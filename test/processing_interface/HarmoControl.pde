/**
 * ╔══════════════════════════════════════════════════════════════════════╗
 * ║        HARMOCONTROL — Interface de test Shapeoko 2                  ║
 * ║                                                                      ║
 * ║  Simule les capteurs via des sliders et envoie les paramètres        ║
 * ║  à l'Arduino Mega via port série USB.                                ║
 * ║  Visualise la trajectoire en temps réel.                             ║
 * ╚══════════════════════════════════════════════════════════════════════╝
 *
 * Prérequis :
 *   - Processing 4.x (https://processing.org)
 *   - Bibliothèque "Serial" incluse dans Processing (aucune installation)
 *
 * Utilisation :
 *   1. Charger le sketch arduino_shapeoko2.ino sur l'Arduino Mega
 *   2. Lancer ce sketch dans Processing
 *   3. Choisir le port COM/ttyUSB et cliquer CONNECT
 *   4. Ajuster les sliders et cliquer HOME pour démarrer
 */

import processing.serial.*;

// ─── Constantes layout ────────────────────────────────────────────────────
final int WIN_W         = 1100;
final int WIN_H         = 700;
final int PANEL_W       = 360;       // Largeur du panneau de contrôle gauche
final int CANVAS_X      = PANEL_W;  // Origine X du canvas de visualisation
final int CANVAS_W      = WIN_W - PANEL_W;
final int CANVAS_H      = WIN_H;

// ─── Couleurs ──────────────────────────────────────────────────────────────
final color BG          = color(20, 22, 30);
final color PANEL_BG    = color(30, 33, 45);
final color ACCENT      = color(80, 160, 255);
final color ACCENT2     = color(255, 140, 60);
final color GREEN_OK    = color(60, 220, 100);
final color RED_ERR     = color(220, 60, 60);
final color TEXT_MAIN   = color(220, 225, 240);
final color TEXT_DIM    = color(110, 115, 135);
final color TRAIL_COLOR = color(80, 160, 255, 180);
final color POS_COLOR   = color(255, 200, 50);
final color GRID_COLOR  = color(45, 48, 60);

// ─── Sliders ──────────────────────────────────────────────────────────────
// Chaque slider : label, valeur [0-1], x, y (centre du track vertical)
float sliderTemp  = 0.50f;
float sliderHum   = 0.50f;
float sliderDist  = 0.80f;
float sliderSnd   = 0.00f;
float sliderFeed  = 0.30f;  // 0=200mm/min, 1=3000mm/min

final int SL_TRACK_H   = 180;
final int SL_Y_TOP     = 160;
final int SL_Y_BOTTOM  = SL_Y_TOP + SL_TRACK_H;
final int SL_WIDTH     = 16;

// Positions X des 5 sliders dans le panneau
final int[] SL_X = {40, 90, 140, 190, 260};
final String[] SL_LABELS = {"TEMP", "HUM", "DIST", "SON", "VITESSE"};

int activeSlider = -1;  // Index du slider en cours de drag

// ─── Boutons ──────────────────────────────────────────────────────────────
// Définition : label, x, y, w, h, couleur, action
class Button {
    String label;
    int x, y, w, h;
    color col;
    boolean pressed = false;

    Button(String l, int x, int y, int w, int h, color c) {
        this.label = l;
        this.x = x; this.y = y; this.w = w; this.h = h; this.col = c;
    }

    void draw() {
        color bg = pressed ? lerpColor(col, color(255), 0.3f) : col;
        fill(bg);
        noStroke();
        rect(x, y, w, h, 5);
        fill(255);
        textAlign(CENTER, CENTER);
        textSize(12);
        text(label, x + w / 2, y + h / 2);
    }

    boolean over() {
        return mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h;
    }
}

Button btnConnect;
Button btnHome;
Button btnPause;
Button btnCircle;
Button btnReset;

// ─── Port série ───────────────────────────────────────────────────────────
Serial serialPort   = null;
String[] portList;
int selectedPort    = 0;
boolean connected   = false;
String statusMsg    = "Deconnecte";
color statusColor   = RED_ERR;

// ─── Données reçues ───────────────────────────────────────────────────────
float machineX      = 0.0f;   // mm depuis centre
float machineY      = 0.0f;
String machineMode  = "BOOT";
int    machinePts   = 0;
int    machineCycle = 0;
float  displayFeed  = 800.0f;

// ─── Trail (trajectoire) ──────────────────────────────────────────────────
final int TRAIL_MAX = 2000;
float[] trailX  = new float[TRAIL_MAX];
float[] trailY  = new float[TRAIL_MAX];
int trailHead   = 0;
int trailCount  = 0;

// ─── Envoi paramètres ─────────────────────────────────────────────────────
int lastSendMs  = 0;
final int SEND_INTERVAL = 300;  // ms

// ─── Log console ─────────────────────────────────────────────────────────
final int LOG_MAX = 8;
String[] logLines = new String[LOG_MAX];
int logCount      = 0;

void addLog(String msg) {
    if (logCount < LOG_MAX) {
        logLines[logCount++] = msg;
    } else {
        for (int i = 0; i < LOG_MAX - 1; i++) logLines[i] = logLines[i + 1];
        logLines[LOG_MAX - 1] = msg;
    }
}

// ─── setup() ─────────────────────────────────────────────────────────────

void setup() {
    size(1100, 700);
    smooth(4);

    portList = Serial.list();
    if (portList.length == 0) {
        println("Aucun port serie detecte!");
    }

    // Boutons
    btnConnect = new Button("CONNECT",  10, 390, 160, 34, color(60, 120, 200));
    btnHome    = new Button("HOME",     10, 435, 78,  34, color(60, 180, 100));
    btnPause   = new Button("PAUSE",    96, 435, 78,  34, color(200, 130, 40));
    btnCircle  = new Button("CERCLE",   10, 477, 78,  34, color(90, 90, 180));
    btnReset   = new Button("CENTRE",   96, 477, 78,  34, color(90, 150, 150));

    for (int i = 0; i < LOG_MAX; i++) logLines[i] = "";
    addLog("HarmoControl pret.");
    addLog("Choisir le port et cliquer CONNECT.");
}

// ─── Coordonnées machine → canvas ────────────────────────────────────────
// Zone de travail : -150mm à +150mm (Shapeoko 2 = 300×300mm, centre = 0,0)

float machToCanvasX(float mmX) {
    float cx = CANVAS_X + CANVAS_W / 2.0f;
    float scale = (CANVAS_W * 0.85f) / 300.0f;
    return cx + mmX * scale;
}

float machToCanvasY(float mmY) {
    float cy = CANVAS_H / 2.0f;
    float scale = (CANVAS_H * 0.85f) / 300.0f;
    return cy - mmY * scale;  // Y inversé (canvas vers le bas)
}

// ─── draw() ──────────────────────────────────────────────────────────────

void draw() {
    background(BG);

    drawPanel();
    drawCanvas();

    // Envoi périodique des paramètres
    if (connected && millis() - lastSendMs > SEND_INTERVAL) {
        lastSendMs = millis();
        sendParams();
    }

    // Lecture série
    if (connected && serialPort != null) {
        while (serialPort.available() > 0) {
            String line = serialPort.readStringUntil('\n');
            if (line != null) {
                line = trim(line);
                parseSerialLine(line);
            }
        }
    }
}

// ─── Panneau de contrôle (gauche) ─────────────────────────────────────────

void drawPanel() {
    // Fond panneau
    fill(PANEL_BG);
    noStroke();
    rect(0, 0, PANEL_W, WIN_H);

    // Titre
    fill(TEXT_MAIN);
    textAlign(LEFT, TOP);
    textSize(16);
    text("HARMOCONTROL", 14, 14);
    fill(TEXT_DIM);
    textSize(10);
    text("Shapeoko 2 — Test interface", 14, 35);

    // Séparateur
    stroke(GRID_COLOR);
    line(10, 52, PANEL_W - 10, 52);

    // Sliders capteurs
    drawSliders();

    // Sélecteur port
    drawPortSelector();

    // Boutons
    btnConnect.draw();
    btnHome.draw();
    btnPause.draw();
    btnCircle.draw();
    btnReset.draw();

    // Status connexion
    drawConnectionStatus();

    // Log
    drawLog();

    // Stats machine
    drawStats();
}

void drawSliders() {
    noStroke();
    fill(TEXT_MAIN);
    textAlign(CENTER, TOP);
    textSize(10);
    text("PARAMETRES CAPTEURS (simules)", PANEL_W / 2, 60);

    // Sliders temp, hum, dist, snd
    float[] vals = {sliderTemp, sliderHum, sliderDist, sliderSnd, sliderFeed};
    for (int i = 0; i < 5; i++) {
        int sx = SL_X[i];
        drawVertSlider(sx, SL_Y_TOP, SL_TRACK_H, vals[i], SL_LABELS[i], i < 4);
    }
}

void drawVertSlider(int cx, int top, int trackH, float val, String lbl, boolean is01) {
    int bottom = top + trackH;

    // Track
    fill(GRID_COLOR);
    noStroke();
    rect(cx - SL_WIDTH / 2, top, SL_WIDTH, trackH, 4);

    // Fill
    int fillH = (int)(trackH * val);
    fill(ACCENT);
    rect(cx - SL_WIDTH / 2, bottom - fillH, SL_WIDTH, fillH, 4);

    // Handle
    int hy = bottom - fillH;
    fill(255);
    stroke(ACCENT);
    strokeWeight(2);
    ellipse(cx, hy, 20, 20);
    noStroke();

    // Label
    fill(TEXT_DIM);
    textAlign(CENTER, TOP);
    textSize(9);
    text(lbl, cx, bottom + 8);

    // Valeur
    fill(TEXT_MAIN);
    textSize(10);
    if (is01) {
        text(nf(val, 1, 2), cx, bottom + 22);
    } else {
        // Feed : convertir en mm/min
        float feed = lerp(200, 3000, val);
        text((int)feed + "", cx, bottom + 22);
    }
}

void drawPortSelector() {
    int y = 355;
    fill(TEXT_DIM);
    textAlign(LEFT, CENTER);
    textSize(10);
    text("PORT SERIE :", 10, y);

    fill(GRID_COLOR);
    noStroke();
    rect(90, y - 12, 170, 24, 4);

    fill(TEXT_MAIN);
    textAlign(LEFT, CENTER);
    textSize(10);
    String portName = (portList.length > 0) ? portList[selectedPort] : "Aucun port";
    if (portName.length() > 22) portName = portName.substring(portName.length() - 22);
    text(portName, 96, y);

    // Flèches
    fill(ACCENT);
    textAlign(CENTER, CENTER);
    textSize(14);
    text("<", 270, y);
    text(">", 286, y);
}

void drawConnectionStatus() {
    int y = 530;
    fill(connected ? GREEN_OK : RED_ERR);
    noStroke();
    ellipse(20, y, 10, 10);
    fill(TEXT_MAIN);
    textAlign(LEFT, CENTER);
    textSize(10);
    text(statusMsg, 30, y);
}

void drawLog() {
    int y = 548;
    stroke(GRID_COLOR);
    line(10, y, PANEL_W - 10, y);

    fill(TEXT_DIM);
    textAlign(LEFT, TOP);
    textSize(9);
    text("LOG", 10, y + 4);

    for (int i = 0; i < LOG_MAX; i++) {
        if (logLines[i] != null && logLines[i].length() > 0) {
            fill(TEXT_DIM);
            text(logLines[i], 10, y + 16 + i * 13);
        }
    }
}

void drawStats() {
    // Rien ici, stats dans le canvas
}

// ─── Canvas de visualisation (droite) ────────────────────────────────────

void drawCanvas() {
    // Fond canvas
    fill(22, 25, 35);
    noStroke();
    rect(CANVAS_X, 0, CANVAS_W, CANVAS_H);

    // Grille
    drawGrid();

    // Trail
    drawTrail();

    // Position courante
    float px = machToCanvasX(machineX);
    float py = machToCanvasY(machineY);

    stroke(POS_COLOR);
    strokeWeight(1.5f);
    line(px - 10, py, px + 10, py);
    line(px, py - 10, px, py + 10);
    noFill();
    ellipse(px, py, 14, 14);

    // Stats overlay
    drawCanvasStats();
}

void drawGrid() {
    float cx = CANVAS_X + CANVAS_W / 2.0f;
    float cy = CANVAS_H / 2.0f;
    float scale = (CANVAS_W * 0.85f) / 300.0f;

    // Cercles de référence (50, 100, 150mm)
    noFill();
    stroke(GRID_COLOR);
    strokeWeight(1);
    for (int r = 50; r <= 150; r += 50) {
        float pr = r * scale;
        ellipse(cx, cy, pr * 2, pr * 2);
    }

    // Axes
    stroke(GRID_COLOR);
    line(cx - 160, cy, cx + 160, cy);
    line(cx, cy - 160, cx, cy + 160);

    // Labels axes
    fill(TEXT_DIM);
    textAlign(LEFT, CENTER);
    textSize(9);
    text("150mm", cx + 150 * scale + 4, cy);
    text("-150mm", cx - 150 * scale - 38, cy);
    textAlign(CENTER, TOP);
    text("150mm", cx, cy - 150 * scale - 14);
    textAlign(CENTER, BOTTOM);
    text("-150mm", cx, cy + 150 * scale + 14);

    // Légende
    fill(TEXT_DIM);
    textAlign(LEFT, TOP);
    textSize(9);
    text("Zone 300x300mm — Shapeoko 2", CANVAS_X + 10, 10);
}

void drawTrail() {
    if (trailCount < 2) return;

    strokeWeight(1.5f);
    noFill();

    for (int i = 1; i < trailCount; i++) {
        int idx0 = (trailHead - trailCount + i - 1 + TRAIL_MAX) % TRAIL_MAX;
        int idx1 = (trailHead - trailCount + i   + TRAIL_MAX) % TRAIL_MAX;

        float alpha = map(i, 0, trailCount, 30, 200);
        stroke(80, 160, 255, alpha);
        line(machToCanvasX(trailX[idx0]), machToCanvasY(trailY[idx0]),
             machToCanvasX(trailX[idx1]), machToCanvasY(trailY[idx1]));
    }
}

void drawCanvasStats() {
    int bx = CANVAS_X + 10;
    int by = WIN_H - 115;

    fill(20, 22, 30, 200);
    noStroke();
    rect(bx - 5, by - 5, 210, 110, 6);

    fill(TEXT_MAIN);
    textAlign(LEFT, TOP);
    textSize(11);
    text("MACHINE", bx, by);

    fill(TEXT_DIM);
    textSize(10);
    text("Mode   : " + machineMode, bx, by + 18);
    text("X      : " + nf(machineX, 1, 1) + " mm", bx, by + 32);
    text("Y      : " + nf(machineY, 1, 1) + " mm", bx, by + 46);
    text("Points : " + machinePts, bx, by + 60);
    text("Cycle  : " + machineCycle, bx, by + 74);

    // Indicateur mode
    color modeCol = GREEN_OK;
    if (machineMode.equals("PAUSED")) modeCol = ACCENT2;
    if (machineMode.equals("ERROR"))  modeCol = RED_ERR;
    if (machineMode.equals("HOMING")) modeCol = ACCENT;
    fill(modeCol);
    noStroke();
    ellipse(bx + 192, by + 8, 10, 10);
}

// ─── Envoi des paramètres vers l'Arduino ──────────────────────────────────

void sendParams() {
    if (serialPort == null) return;
    float feed = lerp(200, 3000, sliderFeed);
    serialPort.write("SET temp " + nf(sliderTemp, 1, 3) + "\n");
    serialPort.write("SET hum "  + nf(sliderHum,  1, 3) + "\n");
    serialPort.write("SET dist " + nf(sliderDist, 1, 3) + "\n");
    serialPort.write("SET snd "  + nf(sliderSnd,  1, 3) + "\n");
    serialPort.write("SET feed " + (int)feed + "\n");
}

// ─── Parsing des réponses Arduino ─────────────────────────────────────────

void parseSerialLine(String line) {
    if (line.startsWith("STATUS ")) {
        // STATUS x=12.3 y=-45.1 mode=RUNNING pts=1234 cycle=2
        String[] tokens = split(line.substring(7), ' ');
        for (String tok : tokens) {
            String[] kv = split(tok, '=');
            if (kv.length == 2) {
                switch (kv[0]) {
                    case "x":     machineX     = float(kv[1]); break;
                    case "y":     machineY     = float(kv[1]); break;
                    case "mode":  machineMode  = kv[1]; break;
                    case "pts":   machinePts   = int(kv[1]); break;
                    case "cycle": machineCycle = int(kv[1]); break;
                }
            }
        }
        // Ajouter au trail
        trailX[trailHead] = machineX;
        trailY[trailHead] = machineY;
        trailHead = (trailHead + 1) % TRAIL_MAX;
        if (trailCount < TRAIL_MAX) trailCount++;

    } else if (line.startsWith("LOG ")) {
        addLog(line.substring(4));
    }
}

// ─── Événements souris ────────────────────────────────────────────────────

void mousePressed() {
    // Sliders
    float[] vals = {sliderTemp, sliderHum, sliderDist, sliderSnd, sliderFeed};
    for (int i = 0; i < 5; i++) {
        int sx = SL_X[i];
        if (abs(mouseX - sx) < 20 &&
            mouseY >= SL_Y_TOP - 10 && mouseY <= SL_Y_BOTTOM + 10) {
            activeSlider = i;
            return;
        }
    }
    activeSlider = -1;

    // Sélection port
    if (mouseY >= 343 && mouseY <= 367) {
        if (mouseX >= 268 && mouseX <= 280 && portList.length > 0) {
            selectedPort = (selectedPort - 1 + portList.length) % portList.length;
        }
        if (mouseX >= 283 && mouseX <= 295 && portList.length > 0) {
            selectedPort = (selectedPort + 1) % portList.length;
        }
    }

    // Bouton CONNECT
    if (btnConnect.over()) {
        if (!connected) {
            connectSerial();
        } else {
            disconnectSerial();
        }
    }

    // Boutons commandes
    if (connected) {
        if (btnHome.over())   sendCmd("CMD home");
        if (btnPause.over())  sendCmd(machineMode.equals("PAUSED") ? "CMD resume" : "CMD pause");
        if (btnCircle.over()) sendCmd("CMD circle");
        if (btnReset.over())  sendCmd("CMD reset");
    }
}

void mouseDragged() {
    if (activeSlider < 0) return;
    float val = map(mouseY, SL_Y_BOTTOM, SL_Y_TOP, 0.0f, 1.0f);
    val = constrain(val, 0.0f, 1.0f);
    switch (activeSlider) {
        case 0: sliderTemp = val; break;
        case 1: sliderHum  = val; break;
        case 2: sliderDist = val; break;
        case 3: sliderSnd  = val; break;
        case 4: sliderFeed = val; break;
    }
}

void mouseReleased() {
    activeSlider = -1;
}

// ─── Connexion série ──────────────────────────────────────────────────────

void connectSerial() {
    if (portList.length == 0) {
        addLog("Aucun port serie disponible!");
        return;
    }
    try {
        serialPort = new Serial(this, portList[selectedPort], 115200);
        serialPort.bufferUntil('\n');
        connected    = true;
        statusMsg    = "Connecte : " + portList[selectedPort];
        statusColor  = GREEN_OK;
        btnConnect.label = "DISCONNECT";
        addLog("Connecte sur " + portList[selectedPort]);
    } catch (Exception e) {
        addLog("Erreur connexion : " + e.getMessage());
        serialPort = null;
    }
}

void disconnectSerial() {
    if (serialPort != null) {
        serialPort.stop();
        serialPort = null;
    }
    connected    = false;
    statusMsg    = "Deconnecte";
    statusColor  = RED_ERR;
    btnConnect.label = "CONNECT";
    addLog("Deconnecte.");
}

void sendCmd(String cmd) {
    if (serialPort == null) return;
    serialPort.write(cmd + "\n");
    addLog("> " + cmd);
}
