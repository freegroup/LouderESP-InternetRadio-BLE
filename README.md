# ESP32 Internet Radio with Bluetooth

An ESP32-based Internet Radio that can switch between WiFi radio streaming and Bluetooth audio reception.

## ⚠️ Important: ESP32 Limitation

The ESP32 **cannot run WiFi and Bluetooth simultaneously in full operation**. This project solves this by having only **one mode fully active** at any time, switching between modes via automatic restart.

## 🔄 Two Operating States

### State 1: WiFi Mode (Radio Streaming)

**What's Active:**
- ✅ WiFi fully active → streaming Internet radio
- ⚠️ Bluetooth in passive "listen mode" → can detect connection requests
- ❌ Bluetooth audio disabled

**What Happens:**
- ESP32 streams MP3 radio from Internet
- Volume controlled locally via rotary encoder
- Bluetooth stack runs passively, waiting for connection requests

### State 2: Bluetooth Mode (Audio Receiver)

**What's Active:**
- ✅ Bluetooth fully active → receiving audio from phone/tablet
- ❌ WiFi completely disabled
- ❌ Radio streaming disabled

**What Happens:**
- ESP32 acts as Bluetooth speaker
- Receives audio from paired device
- Volume controlled via rotary encoder

## 🔀 How Mode Switching Works

### WiFi → Bluetooth

**Trigger:** Connect your phone via Bluetooth while in WiFi mode

**What Happens:**
1. Bluetooth (in listen mode) detects connection request
2. MAC address of your device is saved
3. ESP32 restarts automatically
4. Boots into Bluetooth mode (WiFi disabled)
5. Auto-reconnects to your device

**Result:** Seamless switch to Bluetooth audio

### Bluetooth → WiFi

**Trigger:** Press a station button (Button 2 or 3)

**What Happens:**
1. Selected radio URL is saved
2. ESP32 restarts automatically
3. Boots into WiFi mode (Bluetooth in listen mode only)
4. Starts playing selected radio station

**Result:** Back to radio streaming

### IDLE Mode

**Trigger:** Press Button 1

**What Happens:**
- WiFi disabled
- Bluetooth in listen-only mode
- No audio playback
- Minimal power consumption

## 🎛️ Hardware

- ESP32 board (PSRAM recommended)
- TAS5805M DAC/Amplifier
- Rotary encoder (volume control)
- 3 buttons (station/mode selection)

### Button Functions

- **Button 1**: IDLE mode
- **Button 2**: WiFi mode → Radio Station 1
- **Button 3**: WiFi mode → Radio Station 2

## 📻 Configuration

Edit radio stations in `src/main.cpp`:

```cpp
const char* radio_urls[] = {
    "http://st01.dlf.de/dlf/01/128/mp3/stream.mp3",
    "http://st03.dlf.de/dlf/03/128/mp3/stream.mp3",
    "http://www.radioeins.de/livemp3"
};
```

## 🚀 Getting Started

### First Boot - WiFi Setup

1. Device creates access point: **"FreeGroup Radio Setup"**
2. Password: **"1234"**
3. Connect and configure your WiFi credentials

### Building

```bash
git clone https://github.com/freegroup/LouderESP-InternetRadio-BLE.git
cd LouderESP-InternetRadio-BLE
platformio run --target upload
```

## 💾 Persistent Settings

Settings are stored in ESP32 NVS and survive reboots:
- Current mode (WiFi/Bluetooth/IDLE)
- Last radio station
- Volume level
- Last Bluetooth device MAC address

This enables:
- Auto-reconnect to last Bluetooth device
- Remember last radio station
- Restore volume settings

## 🔧 Why Restart for Mode Switching?

The ESP32 cannot handle WiFi and Bluetooth simultaneously at full capacity. By restarting:
- Clean resource allocation (only one mode active)
- No conflicts between WiFi and Bluetooth
- Reliable operation
- Simple state management

The restart is fast (~2 seconds) and settings are preserved.

## 📁 Project Structure

```
InternetRadio/
├── src/
│   ├── main.cpp        # Main logic & mode switching
│   ├── amplifier.*     # TAS5805M control
│   ├── encoder.*       # Rotary encoder
│   └── buttons.*       # Button input
└── platformio.ini      # Configuration
```

## 🐛 Troubleshooting

**WiFi not connecting?**
- Look for "FreeGroup Radio Setup" access point
- Reconfigure WiFi credentials

**Bluetooth not connecting?**
- Ensure device is in Bluetooth mode (not WiFi mode)
- Look for "FreeGroup Radio" in Bluetooth settings
- Try forgetting and re-pairing

**Mode not switching?**
- Check serial monitor for error messages
- Verify button connections

## 📝 License

Open source - check repository for details.

## 🙏 Credits

- ESP8266Audio library
- ESP32-A2DP library
- TAS5805M library
- WiFiManager library
