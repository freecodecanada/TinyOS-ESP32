/*
 * ╔═══════════════════════════════════════════════════════════════╗
 * ║                    TINY OS v2.0                                ║
 * ║          Full PC-Style Web OS for ESP32-S3                ║
 * ╚═══════════════════════════════════════════════════════════════╝
 *
 * HARDWARE REQUIREMENTS:
 * - LilyGO T-Dongle S3 (or ESP32-S3 with similar specs)
 * - ST7735 80x160 TFT display
 * - APA102 RGB LED
 * - Pins: Display BL=38, LED DATA=40, LED CLK=39
 *
 * REQUIRED LIBRARIES (install via Arduino Library Manager):
 * - TFT_eSPI (configured for ST7735 80x160)
 * - FastLED
 * - WiFi (built-in ESP32)
 * - WebServer (built-in ESP32)
 *
 * FEATURES:
 * - 🇨🇦 Canadian flag & 🐻‍❄️ polar bear desktop icons
 * - 🖥️ Mini desktop terminal + full terminal app
 * - 🎮 Games: Snake, Pong, 2048 (keyboard + touch controls)
 * - 📝 Notepad with save/load
 * - 💻 App Studio (HTML/CSS/JS live editor with templates)
 * - ⚙️ Settings: WiFi SSID/password, IP config, live device stats
 * - 📊 System info: temp, RAM, CPU, uptime
 * - 💡 LED control with 8 colors + brightness
 * - 📁 File manager for notes & apps
 *
 * DEFAULT WIFI CREDENTIALS:
 * - SSID: TinyOS-XXXX (XXXX = last 4 chars of MAC, auto-generated)
 * - Password: TinyOS123
 * - IP: 192.168.4.1
 *
 * USAGE:
 * 1. Flash this sketch to your T-Dongle S3
 * 2. Device shows boot animation then "TINY OS v2.0" techy screen
 * 3. Connect to the WiFi network "TinyOS-XXXX"
 * 4. Open browser to http://192.168.4.1
 * 5. Enjoy your pocket computer!
 *
 * CUSTOMIZATION:
 * - Change default WiFi password on line ~32
 * - Change IP address on line ~33
 * - Adjust LED brightness on line ~37
 *
 * DEVICE SCREEN: Boot animation + techy cyan/green status display
 * (Screen is NOT a touch interface — web UI only)
 *
 * Created for the ESP32 community - open source, customize freely!
 */

#include <WiFi.h>
#include <WebServer.h>
#include <TFT_eSPI.h>
#include <FastLED.h>

// ==================== PINS ====================
#define BACKLIGHT_PIN 38
#define LED_DATA_PIN 40
#define LED_CLK_PIN 39
#define NUM_LEDS 1

// ==================== OBJECTS ====================
TFT_eSPI tft = TFT_eSPI();
WebServer server(80);
CRGB leds[NUM_LEDS];

// ==================== WIFI CONFIG (changeable at runtime) ====================
String ap_ssid = "TinyOS";
String ap_password = "TinyOS123";
IPAddress local_IP(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);

// ==================== STATE ====================
unsigned long bootTime = 0;
int ledBrightness = 80;
String currentLedColor = "green";

// Notes storage
struct Note {
  String title;
  String content;
};
Note notes[10];
int noteCount = 0;

// App Studio saved apps
struct SavedApp {
  String name;
  String code;
};
SavedApp savedApps[5];
int savedAppCount = 0;

// Terminal history (last 50 lines)
String termHistory[50];
int termHistoryCount = 0;

// ==================== DRAW WINDOWS LOGO ON DEVICE ====================
void drawWindowsLogo(int x, int y, int size) {
  int gap = 2;
  int paneSize = size / 2 - gap;
  tft.fillRect(x, y, paneSize, paneSize, TFT_RED);
  uint16_t BROWN = tft.color565(139, 69, 19);
  tft.fillRect(x + paneSize + gap, y, paneSize, paneSize, BROWN);
  tft.fillRect(x, y + paneSize + gap, paneSize, paneSize, BROWN);
  tft.fillRect(x + paneSize + gap, y + paneSize + gap, paneSize, paneSize, TFT_RED);
}

// ==================== WEB HANDLERS ====================

void handleRoot();
void handleLED();
void handleStats();
void handleWifiChange();
void handleIPChange();
void handleReboot();

// ==================== THE BIG HTML PAGE ====================
// Due to ESP32 flash limits we serve this as a single string
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Tiny OS v2.0</title>
<style>
/* ============ RESET & BASE ============ */
* { margin:0; padding:0; box-sizing:border-box; }
body {
  font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;
  overflow:hidden; height:100vh; width:100vw;
  background:#fff; user-select:none;
}

/* ============ DESKTOP ============ */
#desktop {
  position:absolute; inset:0; bottom:48px;
  background:#fff;
  /* subtle grid pattern for techy feel */
  background-image:
    linear-gradient(rgba(0,120,212,0.04) 1px, transparent 1px),
    linear-gradient(90deg, rgba(0,120,212,0.04) 1px, transparent 1px);
  background-size:40px 40px;
}

/* ============ WELCOME TEXT (center of desktop) ============ */
#welcomeText {
  position:absolute;
  top:50%; left:50%;
  transform:translate(-50%,-50%);
  text-align:center;
  pointer-events:none;
  z-index:1;
}
#welcomeText h1 {
  font-size:32px; font-weight:700;
  background:linear-gradient(135deg,#0078d4,#00bcf2,#0078d4);
  -webkit-background-clip:text;
  -webkit-text-fill-color:transparent;
  background-clip:text;
  margin-bottom:8px;
  letter-spacing:1px;
}
#welcomeText p {
  font-size:16px; color:#888; font-style:italic;
}
#welcomeText .win-logo-wrap {
  width:64px; height:64px; margin:0 auto 16px;
  display:grid; grid-template-columns:1fr 1fr; gap:4px;
}
#welcomeText .win-logo-wrap div { border-radius:6px; }
.wl-r { background:#e53935; }
.wl-g { background:#8B4513; }
.wl-b { background:#8B4513; }
.wl-y { background:#e53935; }

/* ============ CANADIAN FLAG (top-left) ============ */
#canadianFlag {
  position:absolute; top:8px; left:8px; z-index:50;
  width:80px; height:52px;
  display:flex; overflow:hidden; border-radius:4px;
  box-shadow:0 2px 6px rgba(0,0,0,0.25);
}
.flag-red { width:20px; background:#FF0000; }
.flag-white { flex:1; background:#fff; display:flex; align-items:center; justify-content:center; }
.maple-leaf { width:28px; height:28px; }

/* ============ POLAR BEAR (top-right) ============ */
#polarBear {
  position:absolute; top:6px; right:8px; z-index:50;
  width:52px; height:52px;
}

/* ============ MINI DESKTOP TERMINAL ============ */
#miniTerm {
  position:absolute; top:10px; left:50%; transform:translateX(-50%);
  z-index:50; width:440px; max-width:75vw;
  background:#1a1a2e; border:2px solid #00e676; border-radius:10px;
  box-shadow:0 4px 16px rgba(0,230,118,0.25);
  overflow:hidden;
}
#miniTermBar {
  background:#111; padding:6px 10px; display:flex; align-items:center; gap:6px;
}
#miniTermBar .dot { width:11px; height:11px; border-radius:50%; }
.dot-r{background:#ff5f57;} .dot-y{background:#ffbd2e;} .dot-g{background:#28c840;}
#miniTermBar span { color:#888; font-size:12px; font-family:'Courier New',monospace; margin-left:8px; }
#miniTermOut {
  padding:8px 10px; font-family:'Courier New',monospace; font-size:13px;
  color:#00e676; height:60px; overflow-y:auto; white-space:pre-wrap;
  line-height:1.4;
}
#miniTermOut .mt-cmd { color:#aaa; }
#miniTermOut .mt-out { color:#00e676; }
#miniTermOut .mt-err { color:#ff5252; }
#miniTermOut .mt-info { color:#64b5f6; }
#miniTermRow { display:flex; align-items:center; padding:0 10px 8px; gap:6px; }
#miniTermRow .mt-prompt { color:#00e676; font-family:'Courier New',monospace; font-size:13px; font-weight:bold; }
#miniTermRow input {
  flex:1; background:transparent; border:none; outline:none;
  color:#00e676; font-family:'Courier New',monospace; font-size:13px; caret-color:#00e676;
}
#miniTermRow input::placeholder { color:#2a5e3a; }

/* ============ TASKBAR ============ */
#taskbar {
  position:fixed; bottom:0; left:0; right:0; height:48px;
  background:rgba(240,242,245,0.92);
  backdrop-filter:blur(12px);
  border-top:1px solid #ddd;
  display:flex; align-items:center; padding:0 8px; gap:4px;
  box-shadow:0 -2px 8px rgba(0,0,0,0.08);
  z-index:9999;
}
.tb-btn {
  height:36px; padding:0 12px; border:none; border-radius:6px;
  background:transparent; cursor:pointer; font-size:14px;
  display:flex; align-items:center; gap:6px; color:#222;
  transition:background 0.15s;
  white-space:nowrap;
}
.tb-btn:hover { background:rgba(0,120,212,0.1); }
.tb-btn.active { background:rgba(0,120,212,0.15); border-bottom:3px solid #0078d4; }
.tb-btn .tb-icon { font-size:18px; }
#tbRight {
  margin-left:auto; display:flex; align-items:center; gap:12px;
  font-size:13px; color:#444;
}

/* ============ START MENU ============ */
#startMenu {
  display:none; position:fixed; bottom:48px; left:0;
  width:280px; background:#fff; border-radius:12px 12px 0 0;
  box-shadow:0 -4px 24px rgba(0,0,0,0.18);
  z-index:10000; padding:12px 0;
  border:1px solid #e0e0e0; border-bottom:none;
}
#startMenu.open { display:block; }
.sm-item {
  display:flex; align-items:center; gap:12px;
  padding:10px 20px; cursor:pointer; font-size:15px; color:#222;
  transition:background 0.12s;
}
.sm-item:hover { background:rgba(0,120,212,0.08); }
.sm-item .sm-icon { font-size:22px; width:28px; text-align:center; }

/* ============ WINDOWS ============ */
.win {
  position:absolute; min-width:380px; max-width:92vw; max-height:85vh;
  background:#fff; border-radius:10px;
  box-shadow:0 8px 32px rgba(0,0,0,0.22);
  display:none; flex-direction:column; overflow:hidden;
  border:1px solid #e0e0e0;
}
.win.open { display:flex; }
.win-tb {
  background:linear-gradient(135deg,#0078d4,#00a4ef);
  color:#fff; padding:10px 12px;
  display:flex; align-items:center; justify-content:space-between;
  cursor:move; flex-shrink:0;
}
.win-tb-title { font-size:14px; font-weight:600; display:flex; align-items:center; gap:8px; }
.win-tb-btns { display:flex; gap:6px; }
.win-tb-btns button {
  background:rgba(255,255,255,0.2); border:none; color:#fff;
  width:28px; height:28px; border-radius:5px; cursor:pointer;
  font-size:16px; line-height:28px; text-align:center;
  transition:background 0.15s;
}
.win-tb-btns button:hover { background:rgba(255,255,255,0.35); }
.win-tb-btns .close-btn:hover { background:#e04343; }
.win-body { padding:18px; overflow-y:auto; flex:1; }

/* ============ LED CONTROL ============ */
.led-grid { display:grid; grid-template-columns:repeat(4,1fr); gap:8px; margin-bottom:16px; }
.led-btn {
  border:2px solid #eee; border-radius:10px; background:#fff;
  padding:12px 4px; cursor:pointer; text-align:center;
  transition:all 0.18s; font-size:13px;
}
.led-btn:hover { border-color:#0078d4; transform:translateY(-2px); box-shadow:0 4px 10px rgba(0,120,212,0.2); }
.led-swatch { width:36px; height:36px; border-radius:50%; margin:0 auto 6px; border:2px solid #ddd; }
.brightness-wrap { margin-top:12px; }
.brightness-wrap label { font-size:13px; font-weight:600; color:#444; }
.brightness-wrap input[type=range] { width:100%; margin-top:4px; }

/* ============ GAMES ============ */
.game-tabs { display:flex; gap:8px; margin-bottom:14px; }
.game-tab {
  flex:1; padding:10px; border:2px solid #eee; border-radius:8px;
  background:#fff; cursor:pointer; text-align:center; font-size:14px; font-weight:600;
  transition:all 0.15s;
}
.game-tab:hover { border-color:#0078d4; }
.game-tab.active { border-color:#0078d4; background:#e8f0fe; }
#gameScore { text-align:center; font-size:20px; font-weight:700; color:#0078d4; margin-bottom:8px; }
#gameCanvas { display:block; margin:0 auto; border:2px solid #0078d4; border-radius:8px; background:#000; }
.game-dpad { display:grid; grid-template-columns:repeat(3,44px); grid-template-rows:repeat(3,44px); gap:3px; margin:10px auto 0; justify-content:center; }
.game-dpad button { background:#0078d4; color:#fff; border:none; border-radius:6px; cursor:pointer; font-size:18px; }
.game-dpad button:active { background:#005a9e; }
.dpad-center { background:transparent !important; }
/* 2048 board */
#board2048 { display:grid; grid-template-columns:repeat(4,1fr); gap:6px; width:280px; margin:0 auto; }
.cell2048 { height:64px; border-radius:8px; background:#cdc1b4; display:flex; align-items:center; justify-content:center; font-size:22px; font-weight:700; color:#776e65; }

/* ============ NOTEPAD ============ */
.notepad-toolbar { display:flex; gap:8px; margin-bottom:12px; padding-bottom:12px; border-bottom:1px solid #eee; }
.notepad-toolbar button { padding:7px 14px; background:#0078d4; color:#fff; border:none; border-radius:6px; cursor:pointer; font-size:13px; }
.notepad-toolbar button:hover { background:#005a9e; }
.note-title-input { width:100%; padding:9px 12px; border:2px solid #ddd; border-radius:6px; font-size:15px; margin-bottom:8px; }
.note-textarea { width:100%; min-height:200px; padding:10px 12px; border:2px solid #ddd; border-radius:6px; font-family:'Courier New',monospace; font-size:14px; resize:vertical; }
.saved-notes { margin-top:16px; }
.saved-notes h4 { font-size:14px; color:#555; margin-bottom:8px; }
.note-row { background:#f5f5f5; padding:10px 12px; border-radius:6px; margin-bottom:6px; display:flex; justify-content:space-between; align-items:center; }
.note-row button { padding:5px 10px; border:none; border-radius:4px; cursor:pointer; font-size:12px; }
.note-row .btn-view { background:#0078d4; color:#fff; margin-right:4px; }
.note-row .btn-del { background:#e04343; color:#fff; }

/* ============ TERMINAL ============ */
#termWindow .win { min-width:480px; }
#termOutput {
  background:#1a1a2e; color:#00e676; font-family:'Courier New',monospace;
  font-size:14px; padding:12px; border-radius:6px;
  height:320px; overflow-y:auto; white-space:pre-wrap; word-wrap:break-word;
  border:1px solid #333;
}
#termOutput .t-cmd { color:#aaa; }
#termOutput .t-out { color:#00e676; }
#termOutput .t-err { color:#ff5252; }
#termOutput .t-info { color:#64b5f6; }
#termInput {
  display:flex; margin-top:8px; gap:8px;
}
#termInput input {
  flex:1; padding:9px 12px; background:#1a1a2e; color:#00e676;
  border:1px solid #333; border-radius:6px; font-family:'Courier New',monospace; font-size:14px;
  outline:none;
}
#termInput input:focus { border-color:#00e676; }
#termInput button { padding:9px 16px; background:#00e676; color:#1a1a2e; border:none; border-radius:6px; font-weight:700; cursor:pointer; }

/* ============ SYSTEM INFO ============ */
.info-grid { display:grid; grid-template-columns:1fr 1fr; gap:10px; }
.info-card { background:#f5f7fa; padding:14px; border-radius:8px; border-left:4px solid #0078d4; }
.info-card .ic-label { font-size:12px; color:#777; margin-bottom:3px; }
.info-card .ic-val { font-size:17px; font-weight:700; color:#222; }
.ic-val.ok { color:#28a745; }

/* ============ SETTINGS ============ */
.set-section { margin-bottom:20px; }
.set-section h4 { font-size:14px; color:#0078d4; margin-bottom:10px; border-bottom:1px solid #eee; padding-bottom:6px; }
.set-row { display:flex; justify-content:space-between; align-items:center; background:#f5f7fa; padding:12px; border-radius:8px; margin-bottom:8px; }
.set-row-left strong { font-size:14px; color:#222; }
.set-row-left small { font-size:12px; color:#777; }
.set-row input[type=text], .set-row input[type=password] {
  padding:7px 10px; border:2px solid #ddd; border-radius:6px; font-size:14px; width:180px;
}
.set-row select { padding:7px 10px; border:2px solid #ddd; border-radius:6px; font-size:14px; }
.set-btn { padding:7px 14px; background:#0078d4; color:#fff; border:none; border-radius:6px; cursor:pointer; font-size:13px; }
.set-btn:hover { background:#005a9e; }
.set-btn.danger { background:#e04343; }
.set-btn.danger:hover { background:#c62828; }

/* ============ FILES ============ */
.files-toolbar { display:flex; gap:8px; margin-bottom:14px; }
.files-toolbar button { padding:7px 14px; background:#0078d4; color:#fff; border:none; border-radius:6px; cursor:pointer; font-size:13px; }
.file-row { background:#f5f7fa; padding:12px 14px; border-radius:8px; margin-bottom:6px; display:flex; align-items:center; gap:10px; }
.file-row .f-icon { font-size:22px; }
.file-row .f-name { flex:1; font-size:14px; font-weight:600; color:#222; }
.file-row .f-meta { font-size:11px; color:#999; }
.file-row button { padding:5px 10px; background:#0078d4; color:#fff; border:none; border-radius:4px; cursor:pointer; font-size:12px; }

/* ============ APP STUDIO ============ */
#studioWin .win { min-width:700px; }
.studio-layout { display:flex; gap:12px; height:380px; }
.studio-editor { flex:1; display:flex; flex-direction:column; }
.studio-editor textarea {
  flex:1; padding:12px; border:2px solid #ddd; border-radius:6px;
  font-family:'Courier New',monospace; font-size:13px; resize:none; outline:none;
}
.studio-editor textarea:focus { border-color:#0078d4; }
.studio-preview { flex:1; border:2px solid #ddd; border-radius:6px; overflow:hidden; }
.studio-preview iframe { width:100%; height:100%; border:none; }
.studio-toolbar { display:flex; gap:8px; margin-bottom:10px; flex-wrap:wrap; align-items:center; }
.studio-toolbar button { padding:7px 14px; background:#0078d4; color:#fff; border:none; border-radius:6px; cursor:pointer; font-size:13px; }
.studio-toolbar button:hover { background:#005a9e; }
.studio-toolbar select { padding:7px 10px; border:2px solid #ddd; border-radius:6px; font-size:13px; }
.studio-toolbar label { font-size:13px; color:#555; font-weight:600; }
.studio-toolbar input[type=text] { padding:7px 10px; border:2px solid #ddd; border-radius:6px; font-size:13px; width:140px; }
.studio-labels { display:flex; gap:12px; margin-bottom:4px; }
.studio-labels span { flex:1; font-size:12px; font-weight:600; color:#0078d4; text-transform:uppercase; letter-spacing:0.5px; }

/* ============ APP MAIN HUB ============ */
.app-hub-grid { display:grid; grid-template-columns:repeat(3,1fr); gap:12px; }
.app-hub-item {
  background:#f5f7fa; border:2px solid #eee; border-radius:12px;
  padding:18px 10px; text-align:center; cursor:pointer;
  transition:all 0.18s;
}
.app-hub-item:hover { border-color:#0078d4; background:#e8f0fe; transform:translateY(-2px); box-shadow:0 4px 12px rgba(0,120,212,0.15); }
.app-hub-item .ah-icon { font-size:34px; margin-bottom:8px; }
.app-hub-item .ah-label { font-size:13px; font-weight:600; color:#333; }

/* ============ UTILITY ============ */
.hidden { display:none !important; }
</style>
</head>
<body>

<!-- ======= DESKTOP ======= -->
<div id="desktop">

  <!-- Canadian Flag top-left -->
  <div id="canadianFlag">
    <div class="flag-red"></div>
    <div class="flag-white">
      <svg class="maple-leaf" viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg">
        <polygon points="50,10 58,30 78,25 68,42 85,50 68,55 78,75 58,68 50,90 42,68 22,75 32,55 15,50 32,42 22,25 42,30" fill="#FF0000"/>
        <rect x="46" y="70" width="8" height="20" fill="#FF0000"/>
      </svg>
    </div>
    <div class="flag-red"></div>
  </div>

  <!-- Polar Bear top-right -->
  <div id="polarBear">
    <svg viewBox="0 0 100 100" width="52" height="52" xmlns="http://www.w3.org/2000/svg">
      <!-- ears -->
      <ellipse cx="25" cy="18" rx="10" ry="12" fill="#fff" stroke="#ccc" stroke-width="1.5"/>
      <ellipse cx="25" cy="18" rx="5" ry="7" fill="#f5d0c0"/>
      <ellipse cx="75" cy="18" rx="10" ry="12" fill="#fff" stroke="#ccc" stroke-width="1.5"/>
      <ellipse cx="75" cy="18" rx="5" ry="7" fill="#f5d0c0"/>
      <!-- head -->
      <ellipse cx="50" cy="45" rx="33" ry="30" fill="#fff" stroke="#ccc" stroke-width="1.5"/>
      <!-- eyes -->
      <circle cx="37" cy="38" r="4" fill="#222"/>
      <circle cx="63" cy="38" r="4" fill="#222"/>
      <!-- eye shine -->
      <circle cx="38.5" cy="36.5" r="1.5" fill="#fff"/>
      <circle cx="64.5" cy="36.5" r="1.5" fill="#fff"/>
      <!-- muzzle -->
      <ellipse cx="50" cy="52" rx="12" ry="8" fill="#f5d0c0"/>
      <!-- nose -->
      <ellipse cx="50" cy="49" rx="4" ry="3" fill="#222"/>
      <!-- mouth -->
      <path d="M50,52 L47,56 M50,52 L53,56" stroke="#222" stroke-width="1.5" fill="none" stroke-linecap="round"/>
      <!-- cheek blush -->
      <ellipse cx="32" cy="46" rx="4" ry="2.5" fill="#ffb0b0" opacity="0.6"/>
      <ellipse cx="68" cy="46" rx="4" ry="2.5" fill="#ffb0b0" opacity="0.6"/>
      <!-- body hint -->
      <ellipse cx="50" cy="88" rx="28" ry="14" fill="#fff" stroke="#ccc" stroke-width="1.5"/>
    </svg>
  </div>

  <!-- Mini Desktop Terminal -->
  <div id="miniTerm">
    <div id="miniTermBar">
      <div class="dot dot-r"></div><div class="dot dot-y"></div><div class="dot dot-g"></div>
      <span>TinyOS Terminal</span>
    </div>
    <div id="miniTermOut"><span class="mt-info">TinyOS v2.0 — type help</span>
</div>
    <div id="miniTermRow">
      <span class="mt-prompt">></span>
      <input type="text" id="miniTermInput" placeholder="type a command..." autocomplete="off" onkeydown="if(event.key==='Enter')miniTermRun()">
    </div>
  </div>

  <!-- Welcome text center -->
  <div id="welcomeText">
    <div class="win-logo-wrap">
      <div class="wl-r"></div><div class="wl-g"></div>
      <div class="wl-b"></div><div class="wl-y"></div>
    </div>
    <h1>Welcome to Tiny OS</h1>
    <p>Your Portable PC</p>
  </div>

</div><!-- end desktop -->

<!-- ======= TASKBAR ======= -->
<div id="taskbar">
  <button class="tb-btn" id="tbStart" onclick="toggleStart()"><span class="tb-icon">⊞</span> Start</button>
  <button class="tb-btn" onclick="openApp('games')"><span class="tb-icon">🎮</span> Games</button>
  <button class="tb-btn" onclick="openApp('files')"><span class="tb-icon">📁</span> Files</button>
  <button class="tb-btn" onclick="openApp('system')"><span class="tb-icon">📊</span> System</button>
  <button class="tb-btn" onclick="openApp('settings')"><span class="tb-icon">⚙️</span> Settings</button>
  <button class="tb-btn" onclick="openApp('appmain')"><span class="tb-icon">📦</span> Apps</button>
  <!-- open-window indicators injected here -->
  <div id="tbOpen" style="display:flex;gap:4px;margin-left:8px;"></div>
  <div id="tbRight">
    <span>📶 WiFi</span>
    <span id="tbClock">--:--</span>
  </div>
</div>

<!-- ======= START MENU ======= -->
<div id="startMenu">
  <div class="sm-item" onclick="openApp('appmain');toggleStart()"><span class="sm-icon">📦</span> All Apps</div>
  <div class="sm-item" onclick="openApp('led');toggleStart()"><span class="sm-icon">💡</span> LED Control</div>
  <div class="sm-item" onclick="openApp('games');toggleStart()"><span class="sm-icon">🎮</span> Games</div>
  <div class="sm-item" onclick="openApp('notepad');toggleStart()"><span class="sm-icon">📝</span> Notepad</div>
  <div class="sm-item" onclick="openApp('files');toggleStart()"><span class="sm-icon">📁</span> Files</div>
  <div class="sm-item" onclick="openApp('terminal');toggleStart()"><span class="sm-icon">🖥️</span> Terminal</div>
  <div class="sm-item" onclick="openApp('studio');toggleStart()"><span class="sm-icon">💻</span> App Studio</div>
  <div class="sm-item" onclick="openApp('system');toggleStart()"><span class="sm-icon">📊</span> System Info</div>
  <div class="sm-item" onclick="openApp('settings');toggleStart()"><span class="sm-icon">⚙️</span> Settings</div>
</div>

<!-- ======= APP MAIN HUB ======= -->
<div id="appmainWin" class="win" style="top:60px;left:50%;transform:translateX(-50%);">
  <div class="win-tb" onmousedown="dragStart(event,'appmainWin')">
    <div class="win-tb-title"><span>📦</span> All Apps</div>
    <div class="win-tb-btns">
      <button onclick="minimizeApp('appmain')">−</button>
      <button class="close-btn" onclick="closeApp('appmain')">×</button>
    </div>
  </div>
  <div class="win-body">
    <div class="app-hub-grid">
      <div class="app-hub-item" onclick="openApp('led')"><div class="ah-icon">💡</div><div class="ah-label">LED Control</div></div>
      <div class="app-hub-item" onclick="openApp('games')"><div class="ah-icon">🎮</div><div class="ah-label">Games</div></div>
      <div class="app-hub-item" onclick="openApp('notepad')"><div class="ah-icon">📝</div><div class="ah-label">Notepad</div></div>
      <div class="app-hub-item" onclick="openApp('files')"><div class="ah-icon">📁</div><div class="ah-label">Files</div></div>
      <div class="app-hub-item" onclick="openApp('terminal')"><div class="ah-icon">🖥️</div><div class="ah-label">Terminal</div></div>
      <div class="app-hub-item" onclick="openApp('studio')"><div class="ah-icon">💻</div><div class="ah-label">App Studio</div></div>
      <div class="app-hub-item" onclick="openApp('system')"><div class="ah-icon">📊</div><div class="ah-label">System Info</div></div>
      <div class="app-hub-item" onclick="openApp('settings')"><div class="ah-icon">⚙️</div><div class="ah-label">Settings</div></div>
    </div>
  </div>
</div>

<!-- ======= LED CONTROL ======= -->
<div id="ledWin" class="win" style="top:80px;left:60px;">
  <div class="win-tb" onmousedown="dragStart(event,'ledWin')">
    <div class="win-tb-title"><span>💡</span> LED Control</div>
    <div class="win-tb-btns">
      <button onclick="minimizeApp('led')">−</button>
      <button class="close-btn" onclick="closeApp('led')">×</button>
    </div>
  </div>
  <div class="win-body">
    <div class="led-grid">
      <div class="led-btn" onclick="setLED('red')"><div class="led-swatch" style="background:#e53935;"></div>Red</div>
      <div class="led-btn" onclick="setLED('green')"><div class="led-swatch" style="background:#43a047;"></div>Green</div>
      <div class="led-btn" onclick="setLED('blue')"><div class="led-swatch" style="background:#1e88e5;"></div>Blue</div>
      <div class="led-btn" onclick="setLED('yellow')"><div class="led-swatch" style="background:#fdd835;"></div>Yellow</div>
      <div class="led-btn" onclick="setLED('cyan')"><div class="led-swatch" style="background:#00e5ff;"></div>Cyan</div>
      <div class="led-btn" onclick="setLED('magenta')"><div class="led-swatch" style="background:#d500f9;"></div>Magenta</div>
      <div class="led-btn" onclick="setLED('white')"><div class="led-swatch" style="background:#fff;"></div>White</div>
      <div class="led-btn" onclick="setLED('off')"><div class="led-swatch" style="background:#222;"></div>Off</div>
    </div>
    <div class="brightness-wrap">
      <label>Brightness: <span id="brightVal">80</span>%</label>
      <input type="range" min="5" max="100" value="80" oninput="setBrightness(this.value)">
    </div>
  </div>
</div>

<!-- ======= GAMES ======= -->
<div id="gamesWin" class="win" style="top:70px;left:50%;transform:translateX(-50%);min-width:440px;">
  <div class="win-tb" onmousedown="dragStart(event,'gamesWin')">
    <div class="win-tb-title"><span>🎮</span> Games</div>
    <div class="win-tb-btns">
      <button onclick="minimizeApp('games')">−</button>
      <button class="close-btn" onclick="closeApp('games')">×</button>
    </div>
  </div>
  <div class="win-body">
    <div class="game-tabs">
      <div class="game-tab active" id="tab-snake" onclick="switchGame('snake')">🐍 Snake</div>
      <div class="game-tab" id="tab-pong" onclick="switchGame('pong')">🏓 Pong</div>
      <div class="game-tab" id="tab-2048" onclick="switchGame('2048')">🔢 2048</div>
    </div>
    <div id="gameScore">Press a game to start!</div>
    <div id="snakeArea">
      <canvas id="gameCanvas" width="360" height="360"></canvas>
      <div class="game-dpad">
        <div></div><button onclick="gameKey('up')">▲</button><div></div>
        <button onclick="gameKey('left')">◀</button><div class="dpad-center"></div><button onclick="gameKey('right')">▶</button>
        <div></div><button onclick="gameKey('down')">▼</button><div></div>
      </div>
      <div style="text-align:center;margin-top:6px;">
        <button onclick="startSnake()" style="padding:8px 18px;background:#28a745;color:#fff;border:none;border-radius:6px;cursor:pointer;font-size:14px;">🔄 New Game</button>
      </div>
    </div>
    <div id="pongArea" class="hidden">
      <canvas id="pongCanvas" width="360" height="360"></canvas>
      <div style="display:flex;justify-content:center;gap:10px;margin-top:10px;">
        <button onclick="gameKey('up')" style="padding:10px 24px;background:#0078d4;color:#fff;border:none;border-radius:6px;cursor:pointer;font-size:18px;">▲</button>
        <button onclick="gameKey('down')" style="padding:10px 24px;background:#0078d4;color:#fff;border:none;border-radius:6px;cursor:pointer;font-size:18px;">▼</button>
      </div>
      <div style="text-align:center;margin-top:6px;">
        <button onclick="startPong()" style="padding:8px 18px;background:#28a745;color:#fff;border:none;border-radius:6px;cursor:pointer;font-size:14px;">🔄 New Game</button>
      </div>
    </div>
    <div id="game2048Area" class="hidden">
      <div id="board2048"></div>
      <div style="display:flex;justify-content:center;gap:8px;margin-top:14px;">
        <button onclick="gameKey('left')" style="padding:10px 20px;background:#0078d4;color:#fff;border:none;border-radius:6px;cursor:pointer;font-size:18px;">◀</button>
        <button onclick="gameKey('up')" style="padding:10px 20px;background:#0078d4;color:#fff;border:none;border-radius:6px;cursor:pointer;font-size:18px;">▲</button>
        <button onclick="gameKey('down')" style="padding:10px 20px;background:#0078d4;color:#fff;border:none;border-radius:6px;cursor:pointer;font-size:18px;">▼</button>
        <button onclick="gameKey('right')" style="padding:10px 20px;background:#0078d4;color:#fff;border:none;border-radius:6px;cursor:pointer;font-size:18px;">▶</button>
      </div>
      <div style="text-align:center;margin-top:10px;">
        <button onclick="start2048()" style="padding:8px 18px;background:#28a745;color:#fff;border:none;border-radius:6px;cursor:pointer;font-size:14px;">🔄 New Game</button>
      </div>
    </div>
  </div>
</div>

<!-- ======= NOTEPAD ======= -->
<div id="notepadWin" class="win" style="top:80px;left:120px;min-width:480px;">
  <div class="win-tb" onmousedown="dragStart(event,'notepadWin')">
    <div class="win-tb-title"><span>📝</span> Notepad</div>
    <div class="win-tb-btns">
      <button onclick="minimizeApp('notepad')">−</button>
      <button class="close-btn" onclick="closeApp('notepad')">×</button>
    </div>
  </div>
  <div class="win-body">
    <div class="notepad-toolbar">
      <button onclick="saveNote()">💾 Save</button>
      <button onclick="clearNote()">🗑️ Clear</button>
    </div>
    <input type="text" class="note-title-input" id="noteTitle" placeholder="Note title...">
    <textarea class="note-textarea" id="noteContent" placeholder="Start typing your note..."></textarea>
    <div class="saved-notes">
      <h4>📂 Saved Notes</h4>
      <div id="notesList"><p style="color:#999;font-size:14px;">No notes yet.</p></div>
    </div>
  </div>
</div>

<!-- ======= FILES ======= -->
<div id="filesWin" class="win" style="top:80px;left:180px;">
  <div class="win-tb" onmousedown="dragStart(event,'filesWin')">
    <div class="win-tb-title"><span>📁</span> Files</div>
    <div class="win-tb-btns">
      <button onclick="minimizeApp('files')">−</button>
      <button class="close-btn" onclick="closeApp('files')">×</button>
    </div>
  </div>
  <div class="win-body">
    <div class="files-toolbar">
      <button onclick="openApp('notepad')">📝 New Note</button>
      <button onclick="refreshFiles()">🔄 Refresh</button>
    </div>
    <div id="filesList"><p style="color:#999;font-size:14px;">No files saved.</p></div>
  </div>
</div>

<!-- ======= TERMINAL ======= -->
<div id="termWin" class="win" style="top:70px;left:50%;transform:translateX(-50%);min-width:520px;">
  <div class="win-tb" onmousedown="dragStart(event,'termWin')">
    <div class="win-tb-title"><span>🖥️</span> Terminal — TinyOS v2.0 <small style="opacity:0.7;font-size:11px;">(Press / to focus)</small></div>
    <div class="win-tb-btns">
      <button onclick="minimizeApp('terminal')">−</button>
      <button class="close-btn" onclick="closeApp('terminal')">×</button>
    </div>
  </div>
  <div class="win-body" style="padding:12px;">
    <div id="termOutput"><span class="t-info">TinyOS v2.0 Terminal — type 'help' for commands</span>\n</div>
    <div id="termInput">
      <input type="text" id="termCmd" placeholder="type a command..." autocomplete="off" onkeydown="if(event.key==='Enter')termRun()">
      <button onclick="termRun()">Run</button>
    </div>
  </div>
</div>

<!-- ======= APP STUDIO ======= -->
<div id="studioWin" class="win" style="top:50px;left:50%;transform:translateX(-50%);min-width:720px;">
  <div class="win-tb" onmousedown="dragStart(event,'studioWin')">
    <div class="win-tb-title"><span>💻</span> App Studio</div>
    <div class="win-tb-btns">
      <button onclick="minimizeApp('studio')">−</button>
      <button class="close-btn" onclick="closeApp('studio')">×</button>
    </div>
  </div>
  <div class="win-body" style="padding:12px;">
    <div class="studio-toolbar">
      <label>Template:</label>
      <select id="studioTemplate" onchange="loadTemplate()">
        <option value="blank">Blank</option>
        <option value="hello">Hello World</option>
        <option value="calc">Calculator</option>
        <option value="colors">Color Changer</option>
      </select>
      <label>Name:</label>
      <input type="text" id="studioName" value="MyApp">
      <button onclick="studioRun()">▶ Run</button>
      <button onclick="studioSave()">💾 Save</button>
      <button onclick="studioLoad()">📂 Load</button>
    </div>
    <div class="studio-labels"><span>📝 Code Editor</span><span>👁️ Preview</span></div>
    <div class="studio-layout">
      <div class="studio-editor"><textarea id="studioCode" spellcheck="false"></textarea></div>
      <div class="studio-preview"><iframe id="studioPreview" srcdoc=""></iframe></div>
    </div>
  </div>
</div>

<!-- ======= SYSTEM INFO ======= -->
<div id="systemWin" class="win" style="top:80px;left:140px;">
  <div class="win-tb" onmousedown="dragStart(event,'systemWin')">
    <div class="win-tb-title"><span>📊</span> System Information</div>
    <div class="win-tb-btns">
      <button onclick="minimizeApp('system')">−</button>
      <button class="close-btn" onclick="closeApp('system')">×</button>
    </div>
  </div>
  <div class="win-body">
    <div class="info-grid">
      <div class="info-card"><div class="ic-label">Device</div><div class="ic-val">T-Dongle S3</div></div>
      <div class="info-card"><div class="ic-label">OS</div><div class="ic-val">Tiny OS v2.0</div></div>
      <div class="info-card"><div class="ic-label">WiFi</div><div class="ic-val" id="sysWifi">TinyOS</div></div>
      <div class="info-card"><div class="ic-label">IP Address</div><div class="ic-val" id="sysIP">192.168.4.1</div></div>
      <div class="info-card"><div class="ic-label">Uptime</div><div class="ic-val" id="sysUptime">--</div></div>
      <div class="info-card"><div class="ic-label">Free RAM</div><div class="ic-val" id="sysRAM">--</div></div>
      <div class="info-card"><div class="ic-label">Status</div><div class="ic-val ok">● Online</div></div>
      <div class="info-card"><div class="ic-label">Clients</div><div class="ic-val" id="sysClients">--</div></div>
    </div>
  </div>
</div>

<!-- ======= SETTINGS ======= -->
<div id="settingsWin" class="win" style="top:80px;left:100px;min-width:440px;">
  <div class="win-tb" onmousedown="dragStart(event,'settingsWin')">
    <div class="win-tb-title"><span>⚙️</span> Settings</div>
    <div class="win-tb-btns">
      <button onclick="minimizeApp('settings')">−</button>
      <button class="close-btn" onclick="closeApp('settings')">×</button>
    </div>
  </div>
  <div class="win-body">
    <!-- WiFi -->
    <div class="set-section">
      <h4>📶 WiFi Access Point</h4>
      <div class="set-row">
        <div class="set-row-left"><strong>Network Name</strong><br><small>Current AP SSID</small></div>
        <input type="text" id="setSSID" value="TinyOS">
      </div>
      <div class="set-row">
        <div class="set-row-left"><strong>Password</strong><br><small>Min 8 characters</small></div>
        <input type="password" id="setPass" value="TinyOS123">
      </div>
      <div class="set-row" style="background:transparent;padding:4px 0;">
        <button class="set-btn" onclick="saveWifi()">💾 Save WiFi</button>
      </div>
    </div>
    <!-- IP -->
    <div class="set-section">
      <h4>🌐 IP Address</h4>
      <div class="set-row">
        <div class="set-row-left"><strong>Local IP</strong><br><small>e.g. 192.168.4.1</small></div>
        <input type="text" id="setIP" value="192.168.4.1">
      </div>
      <div class="set-row" style="background:transparent;padding:4px 0;">
        <button class="set-btn" onclick="saveIP()">💾 Save IP</button>
      </div>
    </div>
    <!-- LED brightness -->
    <div class="set-section">
      <h4>💡 LED Brightness</h4>
      <div class="set-row">
        <div class="set-row-left"><strong>Brightness</strong><br><small>LED intensity</small></div>
        <select id="setBright" onchange="setBrightness(this.value)">
          <option value="20">20%</option>
          <option value="40">40%</option>
          <option value="60">60%</option>
          <option value="80" selected>80%</option>
          <option value="100">100%</option>
        </select>
      </div>
    </div>
    <!-- Live Device Stats -->
    <div class="set-section">
      <h4>📊 Live Device Stats</h4>
      <div class="info-grid" id="settingsStats">
        <div class="info-card"><div class="ic-label">Temperature</div><div class="ic-val" id="setTemp">--</div></div>
        <div class="info-card"><div class="ic-label">CPU Frequency</div><div class="ic-val" id="setCpu">--</div></div>
        <div class="info-card"><div class="ic-label">RAM Used / Total</div><div class="ic-val" id="setRamUsed">--</div></div>
        <div class="info-card"><div class="ic-label">RAM Free</div><div class="ic-val" id="setRamFree">--</div></div>
        <div class="info-card"><div class="ic-label">Flash Size</div><div class="ic-val" id="setFlash">--</div></div>
        <div class="info-card"><div class="ic-label">Min Free RAM</div><div class="ic-val" id="setMinRam">--</div></div>
      </div>
      <div style="margin-top:10px;">
        <div class="set-row" style="background:#f0fff4;border-left:4px solid #28a745;">
          <div class="set-row-left"><strong>RAM Usage</strong></div>
          <div style="width:140px;background:#e0e0e0;border-radius:8px;height:18px;overflow:hidden;">
            <div id="ramBar" style="height:100%;width:0%;background:linear-gradient(90deg,#28a745,#ffc107,#e04343);transition:width 0.5s;border-radius:8px;"></div>
          </div>
        </div>
      </div>
    </div>
    <!-- Reboot -->
    <div class="set-section">
      <h4>🔧 System</h4>
      <div class="set-row" style="background:transparent;padding:4px 0;">
        <button class="set-btn danger" onclick="reboot()">🔄 Reboot Device</button>
      </div>
    </div>
  </div>
</div>

<!-- ========================================================= -->
<!-- JAVASCRIPT                                                  -->
<!-- ========================================================= -->
<script>
// ========== WINDOW / APP MANAGEMENT ==========
const appMap = {
  led:'ledWin', games:'gamesWin', notepad:'notepadWin',
  files:'filesWin', terminal:'termWin', studio:'studioWin',
  system:'systemWin', settings:'settingsWin', appmain:'appmainWin'
};
let zCounter = 100;
let statsIntervalID = null;
let openApps = new Set();
let minimized = new Set();

 const openApp = (name) => {
  const el = document.getElementById(appMap[name]);
  if (!el) return;
  el.classList.add('open');
  el.style.zIndex = ++zCounter;
  // remove any centering transform after first open so dragging works
  if (el.style.transform) {
    const rect = el.getBoundingClientRect();
    el.style.left = rect.left + 'px';
    el.style.top = rect.top + 'px';
    el.style.transform = 'none';
  }
  openApps.add(name);
  minimized.delete(name);
  updateTbOpen();
  // side effects
  if (name === 'system') fetchStats();
  if (name === 'settings') { 
    fetchStats(); 
    if (statsIntervalID) clearInterval(statsIntervalID);
    statsIntervalID = setInterval(fetchStats, 3000); 
  };
  if (name === 'files') refreshFiles();
  if (name === 'terminal' && termHistoryCount === 0) termPrint('info','TinyOS v2.0 Terminal  —  type help for commands');
};
 const closeApp = (name) => {
  const el = document.getElementById(appMap[name]);
  if (el) el.classList.remove('open');
  openApps.delete(name);
  minimized.delete(name);
  updateTbOpen();
  if (name === 'games') stopGame();
  if (name === 'settings' && statsIntervalID) { clearInterval(statsIntervalID); statsIntervalID = null; }
};
 const minimizeApp = (name) => {
  const el = document.getElementById(appMap[name]);
  if (el) el.classList.remove('open');
  minimized.add(name);
  updateTbOpen();
  if (name === 'games') stopGame();
};
 const updateTbOpen = () => {
  const c = document.getElementById('tbOpen');
  c.innerHTML = '';
  openApps.forEach(name => {
    const icons = {led:'💡',games:'🎮',notepad:'📝',files:'📁',terminal:'🖥️',studio:'💻',system:'📊',settings:'⚙️',appmain:'📦'};
    const b = document.createElement('button');
    b.className = 'tb-btn' + (minimized.has(name) ? '' : ' active');
    b.innerHTML = '<span class="tb-icon">' + (icons[name]||'') + '</span> ' + name.charAt(0).toUpperCase()+name.slice(1);
    b.onclick = () => { if(minimized.has(name)){openApp(name);} else { minimizeApp(name); } };
    c.appendChild(b);
  });
};

// ========== START MENU ==========
 const toggleStart = () => {
  document.getElementById('startMenu').classList.toggle('open');
};
document.addEventListener('click', e => {
  if (!e.target.closest('#startMenu') && !e.target.closest('#tbStart'))
    document.getElementById('startMenu').classList.remove('open');
});

// ========== DRAG WINDOWS ==========
let dragEl=null, dragOX=0, dragOY=0;
 const dragStart = (e, id) => {
  e.preventDefault();
  dragEl = document.getElementById(id);
  dragEl.style.zIndex = ++zCounter;
  const rect = dragEl.getBoundingClientRect();
  dragOX = e.clientX - rect.left;
  dragOY = e.clientY - rect.top;
};
document.addEventListener('mousemove', e => {
  if (!dragEl) return;
  dragEl.style.left = (e.clientX - dragOX) + 'px';
  dragEl.style.top  = (e.clientY - dragOY) + 'px';
  dragEl.style.transform = 'none';
});
document.addEventListener('mouseup', () => { dragEl = null; });

// ========== CLOCK ==========
setInterval(() => {
  const n = new Date();
  document.getElementById('tbClock').textContent =
    String(n.getHours()).padStart(2,'0') + ':' + String(n.getMinutes()).padStart(2,'0');
}, 1000);

// ========== MINI DESKTOP TERMINAL ==========
 const miniTermPrint = (type, text) => {
  const el = document.getElementById('miniTermOut');
  el.innerHTML += '<span class="mt-'+type+'">'+text.replace(/</g,'&lt;')+'</span>\n';
  el.scrollTop = el.scrollHeight;
};
 const miniTermRun = () => {
  const input = document.getElementById('miniTermInput');
  const cmd = input.value.trim();
  if (!cmd) return;
  input.value = '';
  miniTermPrint('cmd','> ' + cmd);
  const parts = cmd.split(' ');
  const c = parts[0].toLowerCase();
  const args = parts.slice(1).join(' ');
  switch(c) {
    case 'help':
      miniTermPrint('info','Commands: help ls whoami date led [color] ping sysinfo echo ver open [app]');
      break;
    case 'ls': case 'dir':
      if (!notes.length && !savedApps.length) miniTermPrint('out','(empty)');
      else { notes.forEach(n => miniTermPrint('out','  '+n.title)); savedApps.forEach(a => miniTermPrint('out','  '+a.name+'.html')); }
      break;
    case 'whoami': miniTermPrint('out','TinyOS User  •  T-Dongle S3'); break;
    case 'date': case 'time': miniTermPrint('out', new Date().toLocaleString()); break;
    case 'led':
      if (!args) { miniTermPrint('err','Usage: led [color]'); }
      else { fetch('/led?color='+args); miniTermPrint('out','LED set to '+args); }
      break;
    case 'ping':
      miniTermPrint('out','Pinging 192.168.4.1...');
      var _t0 = Date.now();
      fetch('/stats').then(() => miniTermPrint('out','Reply: time='+(Date.now()-_t0)+'ms')).catch(() => miniTermPrint('err','Request timed out'));
      break;
    case 'sysinfo':
      fetch('/stats').then(r=>r.json()).then(d => {
        miniTermPrint('info','--- Device Stats ---');
        miniTermPrint('out','Uptime : '+d.uptime);
        miniTermPrint('out','Temp   : '+d.tempC+'C / '+d.tempF+'F');
        miniTermPrint('out','RAM    : '+d.usedRam+' used / '+d.totalRam+' total');
        miniTermPrint('out','Free   : '+d.freeRam);
        miniTermPrint('out','Flash  : '+d.flashMB);
        miniTermPrint('out','CPU    : '+d.cpuMHz+' MHz');
      }).catch(() => miniTermPrint('err','Failed to fetch stats'));
      break;
    case 'echo': miniTermPrint('out', args || ''); break;
    case 'ver': miniTermPrint('out','Tiny OS v2.0  •  ESP32-S3  •  T-Dongle S3'); break;
    case 'open':
      if (appMap[args]) { openApp(args); miniTermPrint('out','Opened '+args); }
      else miniTermPrint('err','Unknown app. Try: led games notepad files terminal studio system settings');
      break;
    case 'clear': document.getElementById('miniTermOut').innerHTML = ''; break;
    default: miniTermPrint('err','Unknown command: '+c+'  (type help)');
  }
};

// ========== LED ==========
 const setLED = (color) => { fetch('/led?color=' + color); };

 const setBrightness = (v) => {
  fetch('/led?brightness=' + v);
  document.getElementById('brightVal') && (document.getElementById('brightVal').textContent = v);
};

// ========== SETTINGS — WiFi & IP ==========
 const saveWifi = () => {
  const ssid = document.getElementById('setSSID').value;
  const pass = document.getElementById('setPass').value;
  if (pass.length < 8) { alert('Password must be at least 8 characters!'); return; }
  fetch('/wifi?ssid=' + encodeURIComponent(ssid) + '&pass=' + encodeURIComponent(pass))
    .then(() => alert('WiFi saved! Device will restart AP.\nNew network: ' + ssid + ' / ' + pass))
    .catch(() => alert('Error saving WiFi.'));
};
 const saveIP = () => {
  const ip = document.getElementById('setIP').value;
  fetch('/config?ip=' + encodeURIComponent(ip))
    .then(() => alert('IP saved! Reconnect to: http://' + ip))
    .catch(() => alert('Error saving IP.'));
};
 const reboot = () => {
  if (confirm('Reboot the T-Dongle S3 now?')) {
    fetch('/reboot');
    alert('Rebooting... reconnect in a few seconds.');
  }
};

// ========== SYSTEM INFO ==========
 const fetchStats = () => {
  fetch('/stats').then(r => r.json()).then(d => {
    // System Info window
    document.getElementById('sysUptime').textContent = d.uptime;
    document.getElementById('sysRAM').textContent = d.freeRam;
    document.getElementById('sysClients').textContent = d.clients;
    document.getElementById('sysWifi').textContent = d.ssid || 'TinyOS';
    document.getElementById('sysIP').textContent = d.ip || '192.168.4.1';
    // Settings window live stats
    document.getElementById('setTemp').textContent = d.tempC + '°C / ' + d.tempF + '°F';
    document.getElementById('setTemp').style.color = parseFloat(d.tempC) > 60 ? '#e04343' : parseFloat(d.tempC) > 40 ? '#ffc107' : '#28a745';
    document.getElementById('setCpu').textContent = d.cpuMHz + ' MHz';
    document.getElementById('setRamUsed').textContent = d.usedRam + ' / ' + d.totalRam;
    document.getElementById('setRamFree').textContent = d.freeRam;
    document.getElementById('setFlash').textContent = d.flashMB;
    document.getElementById('setMinRam').textContent = d.minFreeRam;
    document.getElementById('ramBar').style.width = d.ramPercent + '%';
  }).catch(()=>{});
};

// ========== NOTEPAD ==========
let notes = [];
 const saveNote = () => {
  const t = document.getElementById('noteTitle').value || 'Untitled';
  const c = document.getElementById('noteContent').value;
  if (!c) { alert('Write something first!'); return; }
  // check if editing existing
  const idx = notes.findIndex(n => n.title === t);
  if (idx >= 0) notes[idx].content = c;
  else notes.push({title:t, content:c});
  renderNotes();
  clearNote();
};
 const clearNote = () => {
  document.getElementById('noteTitle').value = '';
  document.getElementById('noteContent').value = '';
};
 const renderNotes = () => {
  const el = document.getElementById('notesList');
  if (!notes.length) { el.innerHTML = '<p style="color:#999;font-size:14px;">No notes yet.</p>'; return; }
  el.innerHTML = notes.map((n,i) => `<div class="note-row">
    <span class="f-icon">📄</span>
    <span class="f-name">${n.title}</span>
    <button class="btn-view" onclick="viewNote(${i})">Open</button>
    <button class="btn-del" onclick="deleteNote(${i})">🗑️</button>
  </div>`).join('');
};
 const viewNote = (i) => {
  document.getElementById('noteTitle').value = notes[i].title;
  document.getElementById('noteContent').value = notes[i].content;
  openApp('notepad');
};
 const deleteNote = (i) => {
  if (confirm('Delete "' + notes[i].title + '"?')) { notes.splice(i,1); renderNotes(); refreshFiles(); }
};

// ========== FILES ==========
 const refreshFiles = () => {
  const el = document.getElementById('filesList');
  if (!notes.length && !savedApps.length) { el.innerHTML = '<p style="color:#999;font-size:14px;">No files saved.</p>'; return; }
  let html = '';
  notes.forEach((n,i) => {
    html += `<div class="file-row">
      <span class="f-icon">📄</span>
      <div><div class="f-name">${n.title}</div><div class="f-meta">Note • ${n.content.length} chars</div></div>
      <button onclick="viewNote(${i})">Open</button>
    </div>`;
  });
  savedApps.forEach((a,i) => {
    html += `<div class="file-row">
      <span class="f-icon">💻</span>
      <div><div class="f-name">${a.name}.html</div><div class="f-meta">App Studio project</div></div>
      <button onclick="loadSavedApp(${i})">Open</button>
    </div>`;
  });
  el.innerHTML = html;
};
let savedApps = [];

// ========== TERMINAL ==========
let termHistoryCount = 0;
 const termPrint = (type, text) => {
  const el = document.getElementById('termOutput');
  el.innerHTML += '<span class="t-' + type + '">' + text.replace(/</g,'&lt;') + '</span>\n';
  el.scrollTop = el.scrollHeight;
  termHistoryCount++;
};
 const termRun = () => {
  const input = document.getElementById('termCmd');
  const cmd = input.value.trim();
  if (!cmd) return;
  input.value = '';
  termPrint('cmd', '> ' + cmd);
  const parts = cmd.split(' ');
  const c = parts[0].toLowerCase();
  const args = parts.slice(1).join(' ');

  switch(c) {
    case 'help':
      termPrint('info','Available commands:');
      termPrint('out','  help          — show this help');
      termPrint('out','  ls / dir      — list files');
      termPrint('out','  whoami        — who are you');
      termPrint('out','  date / time   — current date & time');
      termPrint('out','  led [color]   — change LED (red green blue yellow cyan magenta white off)');
      termPrint('out','  reboot        — restart device');
      termPrint('out','  sysinfo       — live device stats');
      termPrint('out','  echo [text]   — print text');
      termPrint('out','  clear         — clear terminal');
      termPrint('out','  ping          — ping the device');
      termPrint('out','  cat [name]    — read a note');
      termPrint('out','  color         — show current LED color');
      termPrint('out','  apps          — list saved apps');
      termPrint('out','  ver           — OS version');
      break;
    case 'ls': case 'dir':
      if (!notes.length && !savedApps.length) { termPrint('out','(empty)'); }
      else {
        notes.forEach(n => termPrint('out','📄 ' + n.title));
        savedApps.forEach(a => termPrint('out','💻 ' + a.name + '.html'));
      }
      break;
    case 'whoami':
      termPrint('out','TinyOS User  •  T-Dongle S3'); break;
    case 'date': case 'time':
      termPrint('out', new Date().toString()); break;
    case 'led':
      if (!args) { termPrint('err','Usage: led [color]'); }
      else { fetch('/led?color=' + args); termPrint('out','LED set to ' + args); }
      break;
    case 'reboot':
      fetch('/reboot'); termPrint('info','Rebooting device...'); break;
    case 'sysinfo':
      fetch('/stats').then(r=>r.json()).then(d => {
        termPrint('info','--- Device Stats ---');
        termPrint('out','Uptime  : ' + d.uptime);
        termPrint('out','Temp    : ' + d.tempC + '°C / ' + d.tempF + '°F');
        termPrint('out','RAM     : ' + d.usedRam + ' used / ' + d.totalRam + ' total');
        termPrint('out','Free RAM: ' + d.freeRam + ' (min: ' + d.minFreeRam + ')');
        termPrint('out','Flash   : ' + d.flashMB);
        termPrint('out','CPU     : ' + d.cpuMHz + ' MHz');
        termPrint('out','Clients : ' + d.clients);
        termPrint('out','SSID    : ' + (d.ssid||'TinyOS'));
        termPrint('out','IP      : ' + (d.ip||'192.168.4.1'));
      }).catch(()=>termPrint('err','Failed to fetch stats.'));
      break;
    case 'echo':
      termPrint('out', args || ''); break;
    case 'clear':
      document.getElementById('termOutput').innerHTML = ''; break;
    case 'ping':
      termPrint('out','Pinging 192.168.4.1...');
      const t0 = Date.now();
      fetch('/stats').then(()=>{
        termPrint('out','Reply from 192.168.4.1: time=' + (Date.now()-t0) + 'ms');
      }).catch(()=>termPrint('err','Request timed out.'));
      break;
    case 'cat':
      if (!args) { termPrint('err','Usage: cat [note name]'); break; }
      const note = notes.find(n => n.title.toLowerCase() === args.toLowerCase());
      if (note) { termPrint('info','--- ' + note.title + ' ---'); termPrint('out', note.content); }
      else termPrint('err','Note not found: ' + args);
      break;
    case 'color':
      termPrint('out','Current LED: (check device)'); break;
    case 'apps':
      if (!savedApps.length) termPrint('out','No saved apps.');
      else savedApps.forEach(a => termPrint('out','💻 ' + a.name));
      break;
    case 'ver':
      termPrint('out','Tiny OS v2.0  •  ESP32-S3  •  T-Dongle S3'); break;
    default:
      termPrint('err','Unknown command: ' + c + '  (type help)');
  }
};
// keyboard shortcut: focus terminal input
document.addEventListener('keydown', e => {
  if (e.key === '/' && document.activeElement.tagName !== 'INPUT' && document.activeElement.tagName !== 'TEXTAREA') {
    e.preventDefault();
    document.getElementById('termCmd').focus();
  }
});

// ========== GAMES ==========
let gameInterval = null;
let currentGame = null;

 const stopGame = () => { if (gameInterval) { clearInterval(gameInterval); gameInterval=null; } currentGame=null; };


 const switchGame = (g) => {
  stopGame();
  document.querySelectorAll('.game-tab').forEach(t => t.classList.remove('active'));
  document.getElementById('tab-'+g).classList.add('active');
  document.getElementById('snakeArea').classList.toggle('hidden', g!=='snake');
  document.getElementById('pongArea').classList.toggle('hidden', g!=='pong');
  document.getElementById('game2048Area').classList.toggle('hidden', g!=='2048');
  if (g==='snake') startSnake();
  else if (g==='pong') startPong();
  else if (g==='2048') start2048();
};

 const gameKey = (dir) => {
  if (currentGame === 'snake') {
    const opp = {up:'down',down:'up',left:'right',right:'left'};
    if (dir !== opp[snakeDir]) snakeNextDir = dir;  // reject 180° turns
  }
  else if (currentGame === 'pong') {
    if (dir==='up') pongPaddleY = Math.max(0, pongPaddleY - 35);
    if (dir==='down') pongPaddleY = Math.min(280, pongPaddleY + 35);
  } 
  else if (currentGame === '2048') { move2048(dir); }
};
// Keyboard listener for games — only when no input focused
document.addEventListener('keydown', e => {
  const tag = document.activeElement.tagName;
  if (tag === 'INPUT' || tag === 'TEXTAREA') return;  // don't steal from inputs
  const map = { ArrowUp:'up', ArrowDown:'down', ArrowLeft:'left', ArrowRight:'right', 
                w:'up', W:'up', s:'down', S:'down', a:'left', A:'left', d:'right', D:'right' };
  if (map[e.key] && currentGame) { 
    e.preventDefault(); 
    gameKey(map[e.key]); 
  }
});

// --- SNAKE ---
let snakeBody, snakeDir, snakeNextDir, snakeFood, snakeScore;
 const startSnake = () => {
  currentGame = 'snake';
  snakeBody = [{x:9,y:9},{x:8,y:9},{x:7,y:9}];
  snakeDir = 'right';
  snakeNextDir = 'right';
  snakeScore = 0;
  spawnSnakeFood();
  document.getElementById('gameScore').textContent = '🐍 Snake — Score: 0';
  gameInterval = setInterval(snakeTick, 150);
};
 const spawnSnakeFood = () => {
  const grid = 18;
  let fx, fy;
  do { fx = Math.floor(Math.random()*grid); fy = Math.floor(Math.random()*grid); }
  while (snakeBody.some(s => s.x===fx && s.y===fy));
  snakeFood = {x:fx, y:fy};
};
 const snakeTick = () => {
  snakeDir = snakeNextDir;  // apply queued direction
  const h = {...snakeBody[0]};
  if (snakeDir==='up') h.y--; else if (snakeDir==='down') h.y++;
  else if (snakeDir==='left') h.x--; else h.x++;
  if (h.x<0||h.x>=18||h.y<0||h.y>=18||snakeBody.some(s=>s.x===h.x&&s.y===h.y)) {
    stopGame(); document.getElementById('gameScore').textContent = '💀 Game Over — Score: '+snakeScore; drawSnake(); return;
  }
  snakeBody.unshift(h);
  if (h.x===snakeFood.x && h.y===snakeFood.y) { snakeScore+=10; spawnSnakeFood(); document.getElementById('gameScore').textContent='🐍 Snake — Score: '+snakeScore; }
  else snakeBody.pop();
  drawSnake();
};
 const drawSnake = () => {
  const canvas = document.getElementById('gameCanvas');
  const ctx = canvas.getContext('2d');
  const ts = 20;
  ctx.fillStyle='#111'; ctx.fillRect(0,0,360,360);
  // grid lines
  ctx.strokeStyle='#1a1a2e'; ctx.lineWidth=0.5;
  for(let i=0;i<=18;i++){ctx.beginPath();ctx.moveTo(i*ts,0);ctx.lineTo(i*ts,360);ctx.stroke();ctx.beginPath();ctx.moveTo(0,i*ts);ctx.lineTo(360,i*ts);ctx.stroke();}
  // food
  ctx.fillStyle='#ff5252'; ctx.beginPath(); ctx.arc(snakeFood.x*ts+ts/2,snakeFood.y*ts+ts/2,ts/2-2,0,Math.PI*2); ctx.fill();
  // snake
  snakeBody.forEach((s,i)=>{
    ctx.fillStyle = i===0 ? '#69f0ae' : '#00e676';
    ctx.fillRect(s.x*ts+1, s.y*ts+1, ts-2, ts-2);
  });
};

// --- PONG ---
let pongBallX, pongBallY, pongDX, pongDY, pongPaddleY, pongAIY, pongScore, pongAIScore;
 const startPong = () => {
  currentGame = 'pong';
  pongBallX=180; pongBallY=180; pongDX=3; pongDY=2.5;
  pongPaddleY=140; pongAIY=140;
  pongScore=0; pongAIScore=0;
  document.getElementById('gameScore').textContent = '🏓 Pong — You: 0  |  AI: 0';
  gameInterval = setInterval(pongTick, 20);
};
 const pongTick = () => {
  pongBallX+=pongDX; pongBallY+=pongDY;
  // top/bottom bounce
  if (pongBallY<=5||pongBallY>=355) pongDY=-pongDY;
  // AI paddle smooth tracking
  pongAIY += (pongBallY - pongAIY - 40) * 0.08;
  pongAIY = Math.max(0, Math.min(280, pongAIY));
  // player paddle hit (right side) — just bounce, NO score
  if (pongBallX>=338 && pongBallX<=352 && pongBallY>=pongPaddleY && pongBallY<=pongPaddleY+80) {
    pongDX = -Math.abs(pongDX) * 1.03;  // reverse + tiny speed boost
    var hitPos = (pongBallY - pongPaddleY) / 80 - 0.5;  // -0.5 to 0.5
    pongDY += hitPos * 4;  // add spin based on hit position
  }
  // AI paddle hit (left side) — just bounce, NO score
  if (pongBallX<=22 && pongBallX>=8 && pongBallY>=pongAIY && pongBallY<=pongAIY+80) {
    pongDX = Math.abs(pongDX) * 1.03;
    var hitPos2 = (pongBallY - pongAIY) / 80 - 0.5;
    pongDY += hitPos2 * 4;
  }
  // cap speed so it doesn't go infinite
  if (Math.abs(pongDX) > 8) pongDX = pongDX > 0 ? 8 : -8;
  if (Math.abs(pongDY) > 7) pongDY = pongDY > 0 ? 7 : -7;
  // miss — ONLY place score changes
  if (pongBallX<=0) { 
    pongScore++; pongBallX=180; pongBallY=180; pongDX=3; pongDY=2;
    document.getElementById('gameScore').textContent='🏓 Pong — You: '+pongScore+'  |  AI: '+pongAIScore;
  }
  if (pongBallX>=360) { 
    pongAIScore++; pongBallX=180; pongBallY=180; pongDX=-3; pongDY=-2;
    document.getElementById('gameScore').textContent='🏓 Pong — You: '+pongScore+'  |  AI: '+pongAIScore;
  }
  drawPong();
};
 const drawPong = () => {
  const canvas = document.getElementById('pongCanvas');
  const ctx = canvas.getContext('2d');
  ctx.fillStyle='#111'; ctx.fillRect(0,0,360,360);
  // center line
  ctx.setLineDash([10,10]); ctx.strokeStyle='#333'; ctx.beginPath(); ctx.moveTo(180,0); ctx.lineTo(180,360); ctx.stroke(); ctx.setLineDash([]);
  // paddles
  ctx.fillStyle='#00e676'; ctx.fillRect(340, pongPaddleY, 12, 80);
  ctx.fillStyle='#ff5252'; ctx.fillRect(8, pongAIY, 12, 80);
  // ball
  ctx.fillStyle='#fff'; ctx.beginPath(); ctx.arc(pongBallX,pongBallY,6,0,Math.PI*2); ctx.fill();
  // labels
  ctx.fillStyle='#555'; ctx.font='12px sans-serif'; ctx.fillText('YOU',330,16); ctx.fillText('AI',14,16);
};

// --- 2048 ---
let grid2048, score2048;
const colors2048 = {0:'#cdc1b4',2:'#eee4da',4:'#ede0c8',8:'#f2b179',16:'#f59563',32:'#f67c5f',64:'#f65e3b',128:'#edcf72',256:'#edcc61',512:'#edc850',1024:'#edc53f',2048:'#edc22e'};
 const start2048 = () => {
  currentGame = '2048';
  grid2048 = [[0,0,0,0],[0,0,0,0],[0,0,0,0],[0,0,0,0]];
  score2048 = 0;
  addTile(); addTile();
  render2048();
  document.getElementById('gameScore').textContent = '🔢 2048 — Score: 0';
};
 const addTile = () => {
  let empty = [];
  for(let r=0;r<4;r++) for(let c=0;c<4;c++) if(!grid2048[r][c]) empty.push([r,c]);
  if (!empty.length) return;
  const [r,c] = empty[Math.floor(Math.random()*empty.length)];
  grid2048[r][c] = Math.random()<0.9?2:4;
};
 const move2048 = (dir) => {
  if (currentGame !== '2048') return;
  const old = JSON.stringify(grid2048);
  if (dir==='up') { for(let c=0;c<4;c++) { let col=[]; for(let r=0;r<4;r++) if(grid2048[r][c]) col.push(grid2048[r][c]); col=merge(col); while(col.length<4) col.push(0); for(let r=0;r<4;r++) grid2048[r][c]=col[r]; } }
  if (dir==='down') { for(let c=0;c<4;c++) { let col=[]; for(let r=3;r>=0;r--) if(grid2048[r][c]) col.push(grid2048[r][c]); col=merge(col); while(col.length<4) col.push(0); for(let r=3,i=0;r>=0;r--,i++) grid2048[r][c]=col[i]; } }
  if (dir==='left') { for(let r=0;r<4;r++) { let row=[]; for(let c=0;c<4;c++) if(grid2048[r][c]) row.push(grid2048[r][c]); row=merge(row); while(row.length<4) row.push(0); grid2048[r]=row; } }
  if (dir==='right') { for(let r=0;r<4;r++) { let row=[]; for(let c=3;c>=0;c--) if(grid2048[r][c]) row.push(grid2048[r][c]); row=merge(row); while(row.length<4) row.push(0); for(let c=3,i=0;c>=0;c--,i++) grid2048[r][c]=row[i]; } }
  if (JSON.stringify(grid2048)!==old) { addTile(); render2048(); }
  // check win/loss
  if (grid2048.flat().includes(2048)) document.getElementById('gameScore').textContent='🎉 You WIN! Score: '+score2048;
  else if (!grid2048.flat().includes(0) && !canMove()) document.getElementById('gameScore').textContent='💀 Game Over — Score: '+score2048;
};
 const merge = (arr) => {
  let res=[];
  let i=0;
  while(i<arr.length) {
    if (i+1<arr.length && arr[i]===arr[i+1]) { res.push(arr[i]*2); score2048+=arr[i]*2; document.getElementById('gameScore').textContent='🔢 2048 — Score: '+score2048; i+=2; }
    else { res.push(arr[i]); i++; }
  }
  return res;
};
 const canMove = () => {
  for(let r=0;r<4;r++) for(let c=0;c<4;c++) {
    if (c<3 && grid2048[r][c]===grid2048[r][c+1]) return true;
    if (r<3 && grid2048[r][c]===grid2048[r+1][c]) return true;
  }
  return false;
};
 const render2048 = () => {
  const b = document.getElementById('board2048');
  b.innerHTML='';
  for(let r=0;r<4;r++) for(let c=0;c<4;c++) {
    const v = grid2048[r][c];
    const d = document.createElement('div');
    d.className='cell2048';
    d.style.background = colors2048[v] || '#3c3a32';
    d.style.color = v>4 ? '#fff' : '#776e65';
    d.textContent = v || '';
    b.appendChild(d);
  }
};
// touch swipe for 2048
let touchStartX=0, touchStartY=0;
document.addEventListener('touchstart', e => { touchStartX=e.touches[0].clientX; touchStartY=e.touches[0].clientY; });
document.addEventListener('touchend', e => {
  if (currentGame!=='2048') return;
  const dx=e.changedTouches[0].clientX-touchStartX;
  const dy=e.changedTouches[0].clientY-touchStartY;
  if (Math.abs(dx)>Math.abs(dy)) gameKey(dx>0?'right':'left');
  else gameKey(dy>0?'down':'up');
});

// ========== APP STUDIO ==========
const templates = {
  blank: '<!DOCTYPE html>\n<html>\n<head><style>\n  body { font-family: sans-serif; padding: 20px; }\n</style></head>\n<body>\n  <h1>My App</h1>\n  <p>Start coding here!</p>\n</body>\n</html>',
  hello: '<!DOCTYPE html>\n<html>\n<head><style>\n  body { font-family: sans-serif; background: #1a1a2e; color: #00e676; display:flex; align-items:center; justify-content:center; height:100vh; margin:0; }\n  h1 { font-size: 42px; text-shadow: 0 0 20px #00e676; }\n</style></head>\n<body>\n  <h1>Hello from Tiny OS!</h1>\n</body>\n</html>',
  calc: '',
  colors: ''
};

 const loadTemplate = () => {
  const t = document.getElementById('studioTemplate').value;
  let html = '';
  if (t === 'blank') html = templates.blank;
  else if (t === 'hello') html = templates.hello;
  else if (t === 'calc') {
    html = `<!DOCTYPE html><html><head><style>
body{font-family:sans-serif;display:flex;justify-content:center;align-items:center;height:100vh;margin:0;background:#222;}
.calc{background:#333;border-radius:12px;padding:20px;width:220px;}
.disp{background:#111;color:#0f0;padding:12px;font-size:28px;text-align:right;border-radius:6px;margin-bottom:10px;font-family:monospace;min-height:40px;word-break:break-all;}
.row{display:flex;gap:6px;margin-bottom:6px;}
.btn{flex:1;height:48px;background:#555;color:#fff;border:none;border-radius:6px;cursor:pointer;font-size:20px;}
.btn:hover{background:#777;}.btn.op{background:#0078d4;}.btn.eq{background:#00e676;color:#111;font-weight:700;}.btn.clr{background:#e04343;}
</style></head><body><div class="calc">
<div class="disp" id="d">0</div>
<div class="row"><button class="btn clr" id="bc">C</button><button class="btn op" id="bmod">%</button><button class="btn op" id="bdiv">/</button><button class="btn op" id="bmul">x</button></div>
<div class="row"><button class="btn" id="b7">7</button><button class="btn" id="b8">8</button><button class="btn" id="b9">9</button><button class="btn op" id="bsub">-</button></div>
<div class="row"><button class="btn" id="b4">4</button><button class="btn" id="b5">5</button><button class="btn" id="b6">6</button><button class="btn op" id="badd">+</button></div>
<div class="row"><button class="btn" id="b1">1</button><button class="btn" id="b2">2</button><button class="btn" id="b3">3</button><button class="btn eq" id="beq">=</button></div>
<div class="row"><button class="btn" style="flex:2;" id="b0">0</button><button class="btn" id="bdot">.</button></div>
</div></body></html>`;
  }
  else if (t === 'colors') {
    html = `<!DOCTYPE html><html><head><style>
body{margin:0;display:flex;flex-direction:column;align-items:center;justify-content:center;height:100vh;font-family:sans-serif;background:#111;color:#fff;}
h2{margin-bottom:16px;}
input[type=color]{width:120px;height:80px;border:none;cursor:pointer;border-radius:8px;}
.preview{width:200px;height:120px;border-radius:12px;margin-top:16px;transition:background 0.3s;border:2px solid #333;}
.code{margin-top:12px;font-family:monospace;color:#aaa;font-size:14px;}
</style></head><body>
<h2>Color Picker</h2>
<input type="color" id="cp" value="#0078d4">
<div class="preview" id="prev" style="background:#0078d4;"></div>
<div class="code" id="code">#0078d4</div>
</body></html>`;
  }
  document.getElementById('studioCode').value = html;
  studioRun();
};
 const studioRun = () => {
  let html = document.getElementById('studioCode').value;
  if (html.indexOf('id="bc"') >= 0) {
    html += '<scr' + 'ipt>' +
      'var ex="";var d=document.getElementById("d");' +
      'var map={"bc":"C","bmod":"%","bdiv":"/","bmul":"*",' +
      '"b7":"7","b8":"8","b9":"9","bsub":"-",' +
      '"b4":"4","b5":"5","b6":"6","badd":"+",' +
      '"b1":"1","b2":"2","b3":"3","b0":"0","bdot":"."};' +
      'Object.keys(map).forEach(function(id){' +
      '  if(id==="bc"||id==="beq")return;' +
      '  document.getElementById(id).addEventListener("click",function(){ex+=map[id];d.textContent=ex;});' +
      '});' +
      'document.getElementById("bc").addEventListener("click",function(){ex="";d.textContent="0";});' +
      'document.getElementById("beq").addEventListener("click",function(){' +
      '  try{d.textContent=eval(ex);ex=String(eval(ex));}catch(e){d.textContent="Err";ex="";}' +
      '});' +
      '</scr' + 'ipt>';
  }
  if (html.indexOf('id="cp"') >= 0) {
    html += '<scr' + 'ipt>' +
      'document.getElementById("cp").addEventListener("input",function(){' +
      '  var v=this.value;' +
      '  document.getElementById("prev").style.background=v;' +
      '  document.getElementById("code").textContent=v;' +
      '});' +
      '</scr' + 'ipt>';
  }
  document.getElementById('studioPreview').srcdoc = html;
};
 const studioSave = () => {
  const name = document.getElementById('studioName').value || 'MyApp';
  const code = document.getElementById('studioCode').value;
  const idx = savedApps.findIndex(a => a.name === name);
  if (idx >= 0) savedApps[idx].code = code;
  else savedApps.push({name, code});
  refreshFiles();
  alert('App "' + name + '" saved!');
};
 const studioLoad = () => {
  if (!savedApps.length) { alert('No saved apps yet.'); return; }
  const names = savedApps.map((a,i) => i + ': ' + a.name).join('\n');
  const idx = parseInt(prompt('Pick an app:\n' + names));
  if (!isNaN(idx) && savedApps[idx]) loadSavedApp(idx);
};
 const loadSavedApp = (i) => {
  document.getElementById('studioName').value = savedApps[i].name;
  document.getElementById('studioCode').value = savedApps[i].code;
  studioRun();
  openApp('studio');
};
// load blank template on page load
window.onload = () => { document.getElementById('studioCode').value = templates.blank; };
</script>
</body>
</html>
)rawliteral";

// ==================== HANDLER IMPLEMENTATIONS ====================

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleLED() {
  if (server.hasArg("brightness")) {
    ledBrightness = server.arg("brightness").toInt();
    FastLED.setBrightness(map(ledBrightness, 0, 100, 0, 255));
    FastLED.show();
  }
  if (server.hasArg("color")) {
    String color = server.arg("color");
    currentLedColor = color;
    if(color=="red")     leds[0]=CRGB::Red;
    else if(color=="green")  leds[0]=CRGB::Green;
    else if(color=="blue")   leds[0]=CRGB::Blue;
    else if(color=="yellow") leds[0]=CRGB::Yellow;
    else if(color=="cyan")   leds[0]=CRGB::Cyan;
    else if(color=="magenta") leds[0]=CRGB::Magenta;
    else if(color=="white")  leds[0]=CRGB::White;
    else if(color=="off")    leds[0]=CRGB::Black;
    FastLED.show();
  }
  server.send(200, "text/plain", "OK");
}

void handleStats() {
  unsigned long upSec = millis() / 1000;
  unsigned long upMin = upSec / 60;
  unsigned long upHr  = upMin / 60;
  String uptimeStr = String(upHr) + "h " + String(upMin % 60) + "m " + String(upSec % 60) + "s";

  uint32_t freeRam   = esp_get_free_heap_size();
  uint32_t totalRam  = ESP.getHeapSize();
  uint32_t minRam    = ESP.getMinFreeHeap();
  uint32_t ramUsed   = totalRam - freeRam;
  int ramPercent     = (int)((ramUsed * 100) / totalRam);

  float tempC        = temperatureRead();
  float tempF        = tempC * 9.0 / 5.0 + 32.0;

  uint32_t flashSize = ESP.getFlashChipSize();
  int cpuFreq        = getCpuFrequencyMhz();
  int clients        = WiFi.softAPgetStationNum();

  String json = "{";
  json += "\"uptime\":\"" + uptimeStr + "\",";
  json += "\"freeRam\":\"" + String(freeRam / 1024) + " KB\",";
  json += "\"totalRam\":\"" + String(totalRam / 1024) + " KB\",";
  json += "\"usedRam\":\"" + String(ramUsed / 1024) + " KB\",";
  json += "\"ramPercent\":" + String(ramPercent) + ",";
  json += "\"minFreeRam\":\"" + String(minRam / 1024) + " KB\",";
  json += "\"tempC\":\"" + String(tempC, 1) + "\",";
  json += "\"tempF\":\"" + String(tempF, 1) + "\",";
  json += "\"flashMB\":\"" + String(flashSize / (1024*1024)) + " MB\",";
  json += "\"cpuMHz\":" + String(cpuFreq) + ",";
  json += "\"clients\":" + String(clients) + ",";
  json += "\"ssid\":\"" + ap_ssid + "\",";
  json += "\"ip\":\"" + local_IP.toString() + "\",";
  json += "\"ledColor\":\"" + currentLedColor + "\",";
  json += "\"ledBrightness\":" + String(ledBrightness);
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleWifiChange() {
  if (server.hasArg("ssid") && server.hasArg("pass")) {
    String newSSID = server.arg("ssid");
    String newPass = server.arg("pass");
    if (newPass.length() < 8) {
      server.send(400, "text/plain", "Password too short");
      return;
    }
    ap_ssid = newSSID;
    ap_password = newPass;
    server.send(200, "text/plain", "WiFi will restart");
    delay(500);
    // Restart AP with new credentials
    WiFi.softAPdisconnect(true);
    delay(200);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
    Serial.println("AP restarted: " + ap_ssid);
  } else {
    server.send(400, "text/plain", "Missing ssid or pass");
  }
}

void handleIPChange() {
  if (server.hasArg("ip")) {
    String ipStr = server.arg("ip");
    // Parse IP
    int parts[4]; int idx = 0; String num = "";
    for (char c : ipStr) {
      if (c == '.') { parts[idx++] = num.toInt(); num = ""; }
      else num += c;
    }
    parts[idx] = num.toInt();
    if (idx == 3) {
      local_IP = IPAddress(parts[0], parts[1], parts[2], parts[3]);
      gateway  = local_IP;
      server.send(200, "text/plain", "IP changing to " + ipStr);
      delay(500);
      WiFi.softAPdisconnect(true);
      delay(200);
      WiFi.softAPConfig(local_IP, gateway, subnet);
      WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
      Serial.println("AP restarted at IP: " + local_IP.toString());
    } else {
      server.send(400, "text/plain", "Invalid IP");
    }
  } else {
    server.send(400, "text/plain", "Missing ip");
  }
}

void handleReboot() {
  server.send(200, "text/plain", "Rebooting...");
  delay(500);
  ESP.restart();
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== TINY OS v2.0 ===");
  bootTime = millis();

  // ---- LED ----
  FastLED.addLeds<APA102, LED_DATA_PIN, LED_CLK_PIN, BGR>(leds, NUM_LEDS);
  FastLED.setBrightness(map(ledBrightness, 0, 100, 0, 255));
  leds[0] = CRGB::Green;
  FastLED.show();

  // ---- DISPLAY — backlight LOW first (working pattern) ----
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, LOW);

  tft.init();
  tft.setRotation(1);   // landscape 160x80
  tft.fillScreen(TFT_BLACK);

  // Fade in backlight
  for (int i = 0; i < 255; i += 5) {
    analogWrite(BACKLIGHT_PIN, i);
    delay(10);
  }
  digitalWrite(BACKLIGHT_PIN, HIGH);

  // Draw Windows logo during boot
  drawWindowsLogo((160 - 40) / 2, 15, 40);
  delay(2000);

  // ---- DRAW TECHY SCREEN (unchanged) ----
  tft.fillScreen(TFT_BLACK);

  // Triple cyan border
  tft.drawRect(0, 0, 160, 80, TFT_CYAN);
  tft.drawRect(1, 1, 158, 78, TFT_CYAN);
  tft.drawRect(2, 2, 156, 76, TFT_CYAN);

  // Corner brackets
  tft.drawLine(5,5,15,5,TFT_GREEN);  tft.drawLine(5,5,5,15,TFT_GREEN);
  tft.drawLine(145,5,155,5,TFT_GREEN); tft.drawLine(155,5,155,15,TFT_GREEN);
  tft.drawLine(5,65,5,75,TFT_GREEN);  tft.drawLine(5,75,15,75,TFT_GREEN);
  tft.drawLine(145,75,155,75,TFT_GREEN); tft.drawLine(155,65,155,75,TFT_GREEN);

  // Title
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(35, 12);
  tft.print("TINY OS");

  // Version
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(65, 30);
  tft.print("v2.0");

  // Windows logo small
  drawWindowsLogo(10, 40, 25);

  // Status
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setCursor(40,43); tft.print("SYS:");
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.print("OK");

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setCursor(40,53); tft.print("MEM:");
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.print("16MB");

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setCursor(100,43); tft.print("NET:");
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.print("AP");

  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setCursor(100,53); tft.print("PWR:");
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.print("ON");

  // Bottom bar
  tft.fillRect(5, 65, 150, 1, TFT_CYAN);
  tft.setTextColor(TFT_CYAN, TFT_BLACK); tft.setCursor(8, 68); tft.print("T-DONGLE S3");
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.setCursor(115, 68); tft.print("READY");

  Serial.println("Techy screen drawn.");

  // ---- WIFI AP with unique SSID ----
  // Append last 4 chars of MAC to make each device unique
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char macSuffix[5];
  sprintf(macSuffix, "%02X%02X", mac[4], mac[5]);
  if (ap_ssid == "TinyOS") {  // only append if using default name
    ap_ssid = "TinyOS-" + String(macSuffix);
  }
  
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
  Serial.print("AP started: "); Serial.println(WiFi.softAPIP());

  // ---- WEB SERVER ROUTES ----
  server.on("/",          handleRoot);
  server.on("/led",       handleLED);
  server.on("/stats",     handleStats);
  server.on("/wifi",      handleWifiChange);
  server.on("/config",    handleIPChange);
  server.on("/reboot",    handleReboot);
  server.begin();

  Serial.println("=== TINY OS v2.0 READY ===");
  Serial.println("WiFi: " + ap_ssid + " / " + ap_password);
  Serial.println("URL:  http://" + local_IP.toString());
}

// ==================== LOOP ====================
void loop() {
  server.handleClient();
  digitalWrite(BACKLIGHT_PIN, HIGH); // CRITICAL: keep backlight alive
}
