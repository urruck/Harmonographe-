/**
 * ╔══════════════════════════════════════════════════════════════════════╗
 * ║        HARMOGCODE — Générateur G-code Harmonographe                 ║
 * ║                                                                      ║
 * ║  Génère et exporte un fichier .nc directement depuis Processing.    ║
 * ║  Aucune carte Arduino, aucun port série requis.                      ║
 * ╚══════════════════════════════════════════════════════════════════════╝
 *
 * Utilisation :
 *   1. Ajuster les sliders (capteurs simulés)
 *   2. Cliquer PREVIEW pour voir la courbe
 *   3. Cliquer EXPORT .NC pour sauvegarder le fichier G-code
 *
 * Prérequis : Processing 4.x — aucune bibliothèque supplémentaire
 */

// ─── Constantes machine (Shapeoko 2) ─────────────────────────────────────
final float MACHINE_RADIUS = 120.0;
final float WORK_OFFSET_X  = 150.0;
final float WORK_OFFSET_Y  = 150.0;
final float DT             = 0.06;       // Pas de temps (s)
final float TWO_PI_F       = TWO_PI;
final float OMEGA_BASE     = TWO_PI / 10.0;
final float PHI            = 1.6180339887;

// Tables de fréquences (identiques au firmware Arduino)
final float[] FREQ1 = {1.000, 1.002, PHI, 1.502, sqrt(2), 2.004, PHI*PHI, sqrt(3)};
final float[] FREQ2 = {1.5, PHI, 2.0, sqrt(2)+0.003, 2.503, PHI*PHI, 3.001, sqrt(3)+0.005};
final float[] FREQY = {0.999, 1.333, PHI-0.001, sqrt(2)-0.002, 1.997, 2.501, sqrt(3)-0.001, 1.622};

// ─── Layout ──────────────────────────────────────────────────────────────
final int WIN_W     = 1000;
final int WIN_H     = 680;
final int PANEL_W   = 340;
final int CANVAS_X  = PANEL_W;
final int CANVAS_W  = WIN_W - PANEL_W;

// ─── Couleurs ─────────────────────────────────────────────────────────────
final color BG        = color(20, 22, 30);
final color PANEL_BG  = color(28, 31, 42);
final color ACCENT    = color(80, 160, 255);
final color ACCENT2   = color(255, 140, 60);
final color GREEN_OK  = color(60, 210, 100);
final color TEXT_MAIN = color(220, 225, 240);
final color TEXT_DIM  = color(100, 108, 130);
final color GRID      = color(40, 44, 58);

// ─── Paramètres (sliders) ────────────────────────────────────────────────
float slTemp   = 0.50;
float slHum    = 0.50;
float slDist   = 0.80;
float slSnd    = 0.00;
float slPoints = 0.25;   // 0→500, 0.5→4250, 1→8000 pts

final int SL_COUNT  = 5;
final int SL_TOP    = 165;
final int SL_H      = 185;
final int SL_BOT    = SL_TOP + SL_H;
final int[] SL_X    = {38, 85, 132, 179, 255};
final String[] SL_LBL = {"TEMP", "HUM", "DIST", "SON", "POINTS"};

int activeSlider = -1;

// ─── Courbe courante ──────────────────────────────────────────────────────
float[] curveX, curveY;
boolean hasCurve = false;
String lastExportPath = "";
String statusMsg = "";
color  statusCol = TEXT_DIM;

// ─── Boutons ──────────────────────────────────────────────────────────────
class Btn {
    String lbl; int x, y, w, h; color col;
    Btn(String l, int x, int y, int w, int h, color c) {
        lbl=l; this.x=x; this.y=y; this.w=w; this.h=h; col=c;
    }
    void draw() {
        fill(over() ? lerpColor(col, color(255), 0.25) : col);
        noStroke(); rect(x, y, w, h, 5);
        fill(255); textAlign(CENTER, CENTER); textSize(12);
        text(lbl, x + w/2, y + h/2);
    }
    boolean over() {
        return mouseX>=x && mouseX<=x+w && mouseY>=y && mouseY<=y+h;
    }
}

Btn btnPreview, btnExport;

// ─── setup() ──────────────────────────────────────────────────────────────
void setup() {
    size(1000, 680);
    smooth(4);
    btnPreview = new Btn("PREVIEW",    14, 400, 148, 38, color(60, 130, 200));
    btnExport  = new Btn("EXPORT .NC", 170, 400, 148, 38, color(60, 190, 90));
    computeCurve();   // Aperçu initial
}

// ─── Algorithme harmonographe ─────────────────────────────────────────────

float interpTable(float[] t, float v) {
    float idx = v * (t.length - 1);
    int i0 = (int) idx;
    int i1 = min(i0 + 1, t.length - 1);
    float f = idx - i0;
    return t[i0] * (1 - f) + t[i1] * f;
}

// Paramètres courants calculés depuis les sliders
float omega1, omega2, omega3;
float phi1, phi2, phi3;
float amp1, amp2, amp3;
float soundPerturb, pFreq1, pFreq2;
float feedrate;

void calcParams() {
    omega1 = interpTable(FREQ1, slTemp) * OMEGA_BASE;
    omega2 = interpTable(FREQ2, slHum)  * OMEGA_BASE;
    float yIdx = (slTemp + slHum) / 2.0;
    omega3 = interpTable(FREQY, yIdx) * OMEGA_BASE;

    float dn = 1.0 - slDist;
    phi1 = dn * TWO_PI * 0.333;
    phi2 = dn * TWO_PI * 0.500 + 0.78;
    phi3 = dn * TWO_PI * 0.167;

    amp1 = max(0.55 + slSnd * 0.30, 0.10);
    amp2 = max(1.0 - amp1, 0.10);
    amp3 = 0.80 + slSnd * 0.18;

    soundPerturb = slSnd * 0.20;
    pFreq1 = 7.391 * OMEGA_BASE;
    pFreq2 = 5.742 * OMEGA_BASE;

    feedrate = constrain((600.0 + slTemp * 800.0) * (0.5 + slDist * 0.8), 200, 2500);
}

float[] getPoint(float t) {
    float rx = amp1 * sin(omega1 * t + phi1) + amp2 * sin(omega2 * t + phi2);
    float ry = amp3 * sin(omega3 * t + phi3);

    if (soundPerturb > 0.001) {
        rx += soundPerturb * sin(pFreq1 * t + 1.234) * sin(pFreq2 * t * 0.7);
        ry += soundPerturb * sin(pFreq2 * t + 2.718) * sin(pFreq1 * t * 0.6);
    }

    float x = rx * MACHINE_RADIUS;
    float y = ry * MACHINE_RADIUS;
    float r = sqrt(x*x + y*y);
    if (r > MACHINE_RADIUS * 0.97) {
        float s = (MACHINE_RADIUS * 0.97) / r;
        x *= s; y *= s;
    }
    return new float[]{x, y};
}

void computeCurve() {
    calcParams();
    int n = (int) lerp(500, 8000, slPoints);
    curveX = new float[n];
    curveY = new float[n];
    float t = 0;
    for (int i = 0; i < n; i++) {
        float[] p = getPoint(t);
        curveX[i] = p[0];
        curveY[i] = p[1];
        t += DT;
    }
    hasCurve = true;
    statusMsg = curveX.length + " points calculés — cliquer EXPORT .NC pour sauvegarder";
    statusCol = TEXT_DIM;
}

// ─── Export G-code ────────────────────────────────────────────────────────

void exportGcode() {
    if (!hasCurve) return;

    // Boîte de dialogue de sauvegarde (Processing natif)
    selectOutput("Enregistrer le fichier G-code (.nc)", "onFileSaved");
}

void onFileSaved(File selection) {
    if (selection == null) {
        statusMsg = "Export annulé.";
        statusCol = TEXT_DIM;
        return;
    }

    String path = selection.getAbsolutePath();
    if (!path.toLowerCase().endsWith(".nc") && !path.toLowerCase().endsWith(".gcode")) {
        path += ".nc";
    }

    String[] lines = buildGcode();
    saveStrings(path, lines);

    lastExportPath = selection.getName();
    if (!lastExportPath.endsWith(".nc") && !lastExportPath.endsWith(".gcode")) {
        lastExportPath += ".nc";
    }

    statusMsg = "Exporté : " + lastExportPath + "  (" + curveX.length + " lignes)";
    statusCol = GREEN_OK;
}

String[] buildGcode() {
    int n = curveX.length;
    String[] lines = new String[n + 12];
    int i = 0;

    lines[i++] = "; Harmonographe — généré par HarmoGcode (Processing)";
    lines[i++] = "; Shapeoko 2 — zone 300x300mm, centre = (150,150)";
    lines[i++] = nf(curveX.length, 0) + " points  |  feedrate " + nf(feedrate, 0, 0) + " mm/min";
    lines[i++] = "; Params: T=" + nf(slTemp,1,2) + " H=" + nf(slHum,1,2)
                    + " D=" + nf(slDist,1,2) + " S=" + nf(slSnd,1,2);
    lines[i++] = ";";
    lines[i++] = "G90 G21 G17";
    lines[i++] = "M5";

    // Point de départ (déplacement rapide)
    float x0 = constrain(curveX[0] + WORK_OFFSET_X, 5, 295);
    float y0 = constrain(curveY[0] + WORK_OFFSET_Y, 5, 295);
    lines[i++] = "G0 X" + nf(x0, 1, 3) + " Y" + nf(y0, 1, 3);

    // Tracé
    for (int j = 0; j < n; j++) {
        float mx = constrain(curveX[j] + WORK_OFFSET_X, 5, 295);
        float my = constrain(curveY[j] + WORK_OFFSET_Y, 5, 295);
        lines[i++] = "G1 X" + nf(mx, 1, 3) + " Y" + nf(my, 1, 3)
                     + " F" + nf(feedrate, 0, 0);
    }

    // Retour centre
    lines[i++] = "G1 X" + nf(WORK_OFFSET_X, 1, 3) + " Y" + nf(WORK_OFFSET_Y, 1, 3)
                 + " F" + nf(feedrate, 0, 0) + "  ; Retour centre";
    lines[i++] = "M2";

    // Tronquer au bon nombre de lignes
    return subset(lines, 0, i);
}

// ─── draw() ──────────────────────────────────────────────────────────────
void draw() {
    background(BG);
    drawPanel();
    drawCanvas();
}

// ─── Panneau gauche ───────────────────────────────────────────────────────
void drawPanel() {
    fill(PANEL_BG); noStroke();
    rect(0, 0, PANEL_W, WIN_H);

    // Titre
    fill(TEXT_MAIN); textAlign(LEFT, TOP); textSize(15);
    text("HARMOGCODE", 14, 14);
    fill(TEXT_DIM); textSize(10);
    text("Générateur G-code — sans carte", 14, 34);

    stroke(GRID); line(10, 50, PANEL_W - 10, 50); noStroke();

    // Sliders
    fill(TEXT_MAIN); textAlign(CENTER, TOP); textSize(10);
    text("CAPTEURS SIMULÉS", PANEL_W/2, 58);

    float[] vals = {slTemp, slHum, slDist, slSnd, slPoints};
    for (int i = 0; i < SL_COUNT; i++) {
        drawSlider(SL_X[i], SL_TOP, SL_H, vals[i], SL_LBL[i], i);
    }

    // Séparateur
    stroke(GRID); line(10, SL_BOT + 50, PANEL_W - 10, SL_BOT + 50); noStroke();

    // Infos paramètres calculés
    drawParamInfo();

    // Boutons
    btnPreview.draw();
    btnExport.draw();

    // Note auto-preview
    fill(TEXT_DIM); textAlign(CENTER, TOP); textSize(9);
    text("(la courbe se recalcule en temps réel)", PANEL_W/2, 447);

    // Status export
    if (statusMsg.length() > 0) {
        fill(statusCol); textAlign(LEFT, TOP); textSize(9);
        // Wrapping manuel sur 2 lignes si trop long
        if (statusMsg.length() > 42) {
            text(statusMsg.substring(0, 42), 14, 470);
            text(statusMsg.substring(42), 14, 483);
        } else {
            text(statusMsg, 14, 470);
        }
    }
}

void drawSlider(int cx, int top, int h, float val, String lbl, int idx) {
    int bot = top + h;

    // Track
    fill(GRID); noStroke();
    rect(cx - 8, top, 16, h, 4);

    // Fill
    int fh = (int)(h * val);
    fill(idx == 4 ? ACCENT2 : ACCENT);
    rect(cx - 8, bot - fh, 16, fh, 4);

    // Handle
    int hy = bot - fh;
    fill(255); stroke(idx == 4 ? ACCENT2 : ACCENT); strokeWeight(2);
    ellipse(cx, hy, 18, 18);
    noStroke();

    // Label
    fill(TEXT_DIM); textAlign(CENTER, TOP); textSize(9);
    text(lbl, cx, bot + 6);

    // Valeur
    fill(TEXT_MAIN); textSize(10);
    if (idx == 4) {
        text((int)lerp(500, 8000, val) + "", cx, bot + 20);
    } else {
        text(nf(val, 1, 2), cx, bot + 20);
    }
}

void drawParamInfo() {
    int y = SL_BOT + 58;
    fill(TEXT_DIM); textAlign(LEFT, TOP); textSize(9);

    String[] infos = {
        "ω1 = " + nf(omega1, 1, 4) + " rad/s   (pendule X1)",
        "ω2 = " + nf(omega2, 1, 4) + " rad/s   (pendule X2)",
        "ω3 = " + nf(omega3, 1, 4) + " rad/s   (pendule Y)",
        "A1=" + nf(amp1,1,2) + "  A2=" + nf(amp2,1,2) + "  A3=" + nf(amp3,1,2),
        "Feed = " + nf(feedrate, 0, 0) + " mm/min"
    };

    for (int i = 0; i < infos.length; i++) {
        text(infos[i], 14, y + i * 14);
    }
}

// ─── Canvas de visualisation ──────────────────────────────────────────────
void drawCanvas() {
    // Fond
    fill(18, 20, 28); noStroke();
    rect(CANVAS_X, 0, CANVAS_W, WIN_H);

    float cx = CANVAS_X + CANVAS_W / 2.0;
    float cy = WIN_H / 2.0;
    float scale = (min(CANVAS_W, WIN_H) * 0.83) / (MACHINE_RADIUS * 2);

    // Grille
    noFill(); strokeWeight(0.8);
    for (int r = 50; r <= 150; r += 50) {
        stroke(GRID);
        ellipse(cx, cy, r * scale * 2, r * scale * 2);
    }
    stroke(GRID);
    line(cx - 160, cy, cx + 160, cy);
    line(cx, cy - 160, cx, cy + 160);

    fill(TEXT_DIM); textAlign(LEFT, CENTER); textSize(8);
    text("±150mm", cx + 150 * scale + 3, cy);

    // Courbe
    if (hasCurve) {
        int n = curveX.length;
        strokeWeight(1.2);
        for (int i = 1; i < n; i++) {
            float alpha = lerp(25, 210, (float)i / n);
            stroke(80, 160, 255, alpha);
            float x0 = cx + curveX[i-1] * scale;
            float y0 = cy - curveY[i-1] * scale;
            float x1 = cx + curveX[i]   * scale;
            float y1 = cy - curveY[i]   * scale;
            line(x0, y0, x1, y1);
        }

        // Départ (vert) / arrivée (orange)
        noStroke();
        fill(60, 210, 100);
        ellipse(cx + curveX[0] * scale, cy - curveY[0] * scale, 7, 7);
        fill(255, 140, 60);
        ellipse(cx + curveX[n-1] * scale, cy - curveY[n-1] * scale, 7, 7);
    }

    // Légende
    fill(TEXT_DIM); textAlign(LEFT, TOP); textSize(9);
    text("Shapeoko 2 — zone 300×300mm", CANVAS_X + 10, 10);
    fill(60, 210, 100);  text("● départ", CANVAS_X + 10, 24);
    fill(255, 140, 60);  text("● arrivée", CANVAS_X + 60, 24);

    if (hasCurve) {
        fill(TEXT_DIM); textAlign(RIGHT, TOP); textSize(9);
        text(curveX.length + " pts  |  " + nf(curveX.length * DT, 0, 1) + "s simulées",
             WIN_W - 10, 10);
    }
}

// ─── Événements ──────────────────────────────────────────────────────────

void mousePressed() {
    // Sliders
    for (int i = 0; i < SL_COUNT; i++) {
        if (abs(mouseX - SL_X[i]) < 18 && mouseY >= SL_TOP - 12 && mouseY <= SL_BOT + 12) {
            activeSlider = i;
            return;
        }
    }
    activeSlider = -1;

    // Boutons
    if (btnPreview.over()) {
        computeCurve();
    }
    if (btnExport.over()) {
        if (!hasCurve) computeCurve();
        exportGcode();
    }
}

void mouseDragged() {
    if (activeSlider < 0) return;
    float v = constrain(map(mouseY, SL_BOT, SL_TOP, 0, 1), 0, 1);
    switch (activeSlider) {
        case 0: slTemp   = v; break;
        case 1: slHum    = v; break;
        case 2: slDist   = v; break;
        case 3: slSnd    = v; break;
        case 4: slPoints = v; break;
    }
    computeCurve();   // Recalcul en temps réel pendant le drag
}

void mouseReleased() {
    activeSlider = -1;
}
