# 🖥️ TinyOS v2.0

**A complete PC-style web-based operating system running on a tiny ESP32-S3 device**

Turn your LilyGO T-Dongle S3 into a fully functional pocket computer with games, a terminal, app development studio, file management, and live system monitoring — all accessible through a web interface.


---<img width="1920" height="953" alt="image" src="https://github.com/user-attachments/assets/46152c68-767c-4dba-8857-017dec0d6339" />

## ✨ Features

### 🎨 Desktop Environment
- **Windows-style interface** with taskbar, start menu, and draggable windows
- **Mini desktop terminal** for quick commands
- **Live clock** and WiFi status indicator
- **Multi-window management** — drag, minimize, close

### 🎮 Built-in Games (All with Keyboard + Touch Controls)
| Game | Description | Controls |
|------|-------------|----------|
| 🐍 **Snake** | Classic snake with score tracking | WASD / Arrow keys + on-screen D-pad |
| 🏓 **Pong** | Play against AI with realistic physics | W/S or ↑/↓ + on-screen buttons |
| 🔢 **2048** | Addictive number puzzle | Arrow keys / Swipe + on-screen arrows |

All games include restart buttons and proper game-over detection.

### 💻 Productivity Apps
- **🖥️ Terminal** — Full command-line interface
  - 15+ commands (ls, ping, sysinfo, led, cat, etc.)
  - Command history
  - Color-coded output
- **📝 Notepad** — Write and save notes to device memory
  - Save/load functionality
  - Multiple notes support
- **📁 File Manager** — Browse saved notes and apps
- **💻 App Studio** — Build HTML/CSS/JS apps with live preview!
  - Code editor with syntax highlighting
  - Live iframe preview
  - 4 templates: Blank, Hello World, Calculator, Color Picker
  - Save and load projects

### ⚙️ System Tools
- **Settings**
  - Configure WiFi SSID/password (no reflashing needed!)
  - Change IP address
  - LED brightness control
  - Live device stats
- **📊 System Info** — Real-time monitoring:
  - 🌡️ Temperature (°C/°F) with color-coded warnings
  - 💾 RAM usage with visual progress bar
  - ⚡ CPU frequency
  - ⏱️ Uptime
  - 📶 Connected clients
  - 📦 Flash storage size
- **💡 LED Control**
  - 8 colors (Red, Green, Blue, Yellow, Cyan, Magenta, White, Off)
  - Brightness slider (5-100%)

### 📱 Physical Device Display
- **80x160 TFT screen** shows techy cyan/green status interface
- Smooth boot animation with Windows logo
- Live system status indicators (SYS, MEM, NET, PWR)
- "TINY OS v2.0" branding

---

## 🛠️ Hardware Requirements

### Required Components
- **LilyGO T-Dongle S3** 
  - Or any ESP32-S3 with ST7735 80x160 TFT display

### Specifications
- ESP32-S3 dual-core @ 240MHz
- 16MB Flash / 8MB PSRAM
- WiFi 2.4GHz (AP mode)
- ST7735 TFT Display (80x160 pixels)

### Pin Configuration
```cpp
Display Backlight: GPIO 38
LED Data Pin:      GPIO 40
LED Clock Pin:     GPIO 39
```

---

## 📦 Installation

### 1. Install Arduino IDE & Board Support
1. Download [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - File → Preferences
   - Additional Board Manager URLs: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board → Boards Manager → Search "ESP32" → Install

### 2. Install Required Libraries
Arduino IDE → Tools → Manage Libraries → Install:
- **TFT_eSPI** (by Bodmer)
- **FastLED**

### 3. Configure TFT_eSPI for T-Dongle S3
Edit `Arduino/libraries/TFT_eSPI/User_Setup.h`:
```cpp
#define ST7735_DRIVER
#define TFT_WIDTH  80
#define TFT_HEIGHT 160
#define ST7735_GREENTAB160x80
#define TFT_RGB_ORDER TFT_BGR

// Pin definitions
#define TFT_MOSI 3
#define TFT_SCLK 5
#define TFT_CS   4
#define TFT_DC   2
#define TFT_RST  1
#define TFT_BL   38
```

### 4. Flash the Code
1. Download `TinyOS_v2.0_RELEASE.ino`
2. Open in Arduino IDE
3. Select:
   - Board: **ESP32S3 Dev Module**
   - USB CDC On Boot: **Enabled**
   - Port: (your device's COM port)
4. Click **Upload** ⬆️
5. Wait ~30 seconds for compile + upload

### 5. Connect 
1. Device boots → Shows Windows logo → "TINY OS v2.0" screen
2. Look for WiFi network: **TinyOS-XXXX** (XXXX = last 4 chars of your device's MAC)
3. Connect with password: **TinyOS123**
4. Open browser → **http://192.168.4.1**

---

## 🎯 Usage Guide

### First-Time Setup
The device creates its own WiFi network (Access Point mode). 

**Default Credentials:**
- SSID: `TinyOS-XXXX` (auto-generated unique ID)
- Password: `TinyOS123`
- Web Interface: `http://192.168.4.1`

### Keyboard Shortcuts
| Key | Action |
|-----|--------|
| `/` | Focus mini terminal (from anywhere) |
| `WASD` / `Arrow Keys` | Control games (when games window open) |
| `Enter` | Run terminal command |

### Terminal Commands
```bash
help        # Show all available commands
ls          # List saved notes and apps
dir         # Same as ls
whoami      # Display user info
date        # Show current date/time
time        # Same as date
sysinfo     # Full device stats (temp, RAM, CPU, etc.)
led red     # Change LED color (red/green/blue/yellow/cyan/magenta/white/off)
ping        # Test connection latency to device
echo hello  # Print text
clear       # Clear terminal output
cat MyNote  # Read a saved note
apps        # List App Studio projects
ver         # Show OS version
open led    # Open an app by name
```

### Customization (No Reflashing Needed!)
Use the **Settings** app in the web interface to change:
- WiFi SSID and password
- IP address
- LED brightness

**Or** edit the code (lines 32-37):
```cpp
String ap_ssid = "TinyOS";        // WiFi name
String ap_password = "TinyOS123";  // Change this for security!
IPAddress local_IP(192,168,4,1);   // Device IP address
int ledBrightness = 80;            // LED brightness (0-100)
```

---

## 💻 App Studio - Supported Languages

The **App Studio** is a live HTML/CSS/JavaScript editor that runs **directly in your browser**. You can create full web apps that run in the preview iframe.

### ✅ Fully Supported Languages:
1. **HTML** — Structure your app
2. **CSS** — Style with any valid CSS (inline or `<style>` tags)
3. **JavaScript** — Full ES6+ support including:
   - Arrow functions
   - Template literals
   - `let`/`const`
   - Promises
   - Async/await
   - Classes
   - Modules (with limitations)

### Limitations:
- ❌ No Node.js or backend code (it's client-side only)
- ❌ No file system access (runs in iframe sandbox)
- ❌ No ES6 modules with `import`/`export` (use script tags instead)
- ✅ You CAN fetch data from external APIs
- ✅ You CAN use localStorage for data persistence
- ✅ Canvas/WebGL works perfectly for games

---

## 🤝 Contributing

Contributions welcome! Here are some ideas:
- 🎮 Add more games (Tetris, Breakout, etc.)
- 🎨 New color themes / skins
- 🔧 More terminal commands
- 📱 Mobile-optimized interface
- 🌐 Multi-language support
- 🔐 Authentication/security features

**To contribute:**
1. Fork this repo
2. Create a feature branch (`git checkout -b feature/NewGame`)
3. Commit your changes (`git commit -m 'Add Tetris game'`)
4. Push to branch (`git push origin feature/NewGame`)
5. Open a Pull Request


- Inspired by Windows UI design
- Built for the maker community ❤️

---

## 📸 Screenshots

<img width="376" height="442" alt="image" src="https://github.com/user-attachments/assets/5260945f-06d0-469c-89f9-4162ccd57980" />

### Games

<img width="435" height="645" alt="image" src="https://github.com/user-attachments/assets/00f9115e-b50c-4d39-a1ce-32d42447ac64" />

### Terminal

<img width="854" height="435" alt="image" src="https://github.com/user-attachments/assets/ce19ca9e-5e53-4f60-b1e7-44b6ba413a33" />


### App Studio
<img width="704" height="502" alt="image" src="https://github.com/user-attachments/assets/cb2c86c2-4065-4939-833d-aa55315a5bf5" />


### Device Screen
![IMG_4438](https://github.com/user-attachments/assets/63fa7821-3d8d-4465-84a4-e17d36a619cc)


### System

<img width="387" height="383" alt="image" src="https://github.com/user-attachments/assets/48297857-f3b4-46aa-a5de-a3fc494b50d8" />

<img width="433" height="597" alt="image" src="https://github.com/user-attachments/assets/527b7816-c120-4a08-9fe8-0224f3560101" />

<img width="423" height="445" alt="image" src="https://github.com/user-attachments/assets/5dfd7cca-56de-4470-92f7-613a2fa1e5ff" />



If you find this project useful, consider giving it a star! 

