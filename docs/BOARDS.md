# Board Support Matrix

> **Project Aether** — Board-specific firmware branches. Each board gets its own branch: `board/<board-name>`.

---

## Supported Boards

| Board | Branch | MCU | RAM | PSRAM | USB | Status | Maintainer |
|-------|--------|-----|-----|-------|-----|--------|------------|
| ESP32 DevKit (WROOM-32) | `board/esp32-devkit` | ESP32-D0WD | 520KB | ❌ | Micro-USB | ✅ Primary | @arc |
| ESP32-S3 DevKitC-1 | `board/esp32s3-devkit` | ESP32-S3 | 512KB | 8MB | USB-C | 🟡 WIP | — |
| ESP32-C3 DevKitM-1 | `board/esp32c3-devkit` | ESP32-C3 | 400KB | ❌ | USB-C | ⏳ Planned | — |
| ESP32-S2 DevKit | `board/esp32s2-devkit` | ESP32-S2 | 320KB | ❌ | USB-C | ⏳ Planned | — |
| ESP32-C6 DevKit | `board/esp32c6-devkit` | ESP32-C6 | 512KB | ❌ | USB-C | ⏳ Planned | — |
| ESP32-H2 | `board/esp32h2-devkit` | ESP32-H2 | 320KB | ❌ | USB-C | ⏳ Planned | — |
| M5Stack Core2 | `board/m5stack-core2` | ESP32-D0WD | 520KB | 8MB | USB-C | ⏳ Planned | — |
| LilyGo T-Display S3 | `board/lilygo-tdisplay-s3` | ESP32-S3 | 512KB | 8MB | USB-C | ⏳ Planned | — |
| LilyGo T-Beam | `board/lilygo-tbeam` | ESP32-S3 | 512KB | 8MB | USB-C | ⏳ Planned | — |

---

## Adding a New Board

### 1. Create Branch
```bash
git checkout -b board/<board-slug>
# Example: board/esp32s3-devkit
```

### 2. Add Board Config
Edit `firmware/src/config.h`:
```cpp
#if defined(BOARD_ESP32S3_DEVKIT)
  #define BOARD_NAME "ESP32-S3 DevKitC-1"
  #define BUTTON_PIN 0          // BOOT button on S3
  #define LED_RED_PIN 17
  #define LED_GREEN_PIN 16
  #define LED_BLUE_PIN 18
  #define HAS_PSRAM 1
  #define HAS_USB_CDC 1
#elif defined(BOARD_ESP32_DEVKIT)
  // ... existing config
#endif
```

### 3. Update PlatformIO
```ini
; platformio.ini
[env:esp32s3-devkit]
platform = espressif32
board = esp32s3-devkit
framework = arduino
board_build.partitions = partitions.csv
build_flags = -DBOARD_ESP32S3_DEVKIT
```

### 4. Pinout Documentation
Add to `docs/BOARDS.md`:
```markdown
## ESP32-S3 DevKitC-1
| GPIO | Function |
|------|----------|
| 0 | Boot Button (INPUT_PULLUP) |
| 17 | Red LED |
| 16 | Green LED |
| 18 | Blue LED |
| 21 | I2C SDA (ATECC608B) |
| 22 | I2C SCL (ATECC608B) |
```

### 5. Test & Document
- [ ] Firmware builds: `pio run -e <env>`
- [ ] Flash & test provisioning flow
- [ ] Test HTTPS (if implemented)
- [ ] Test button interrupt
- [ ] Document any quirks

### 6. PR Template
```markdown
## Board: ESP32-S3 DevKitC-1
## Hardware
- DevKitC-1 v1.1
- Photos: [link]
## Changes
- `config.h`: Added BOARD_ESP32S3_DEVKIT
- `platformio.ini`: Added env:esp32s3-devkit
## Testing
- [ ] Provisioning AP works
- [ ] STA connects to phone hotspot
- [ ] mDNS resolves
- [ ] Login page loads
- [ ] Button interrupt works (if wired)
- [ ] LEDs indicate state correctly
## Photos
[attach photos]
```

---

## Board-Specific Notes

### ESP32 DevKit (WROOM-32) — `board/esp32-devkit`
- **Primary target** — most tested
- No PSRAM → limited TLS buffer
- Micro-USB, no native USB CDC
- BOOT button = GPIO 0 (used for bootloader, not available for app)
- Use GPIO 4 for button

### ESP32-S3 DevKitC-1 — `board/esp32s3-devkit`
- 8MB PSRAM → full TLS buffers
- USB-C, native USB CDC + JTAG
- BOOT button = GPIO 0 (usable for app)
- 512KB SRAM + 8MB PSRAM
- Supports USB CDC + WebUSB for debugging

### ESP32-C3 DevKitM-1
- RISC-V single core
- No Bluetooth
- 400KB SRAM
- USB-C native

### ESP32-S2 DevKit
- No Bluetooth
- 320KB SRAM
- Native USB OTG
- Single core Xtensa LX7

### M5Stack Core2
- Built-in display, touch, IMU
- 8MB PSRAM
- Built-in battery
- Different pinout (check schematic)

### LilyGo T-Display S3
- Built-in ST7789 display
- ESP32-S3, 8MB PSRAM
- Great for preview mode demo

---

## Board Comparison Matrix

| Feature | ESP32 DevKit | ESP32-S3 DevKit | ESP32-C3 | ESP32-S2 |
|---------|--------------|-----------------|----------|----------|
| **Core** | Xtensa LX6 dual | Xtensa LX7 dual | RISC-V | Xtensa LX7 single |
| **Clock** | 240 MHz | 240 MHz | 160 MHz | 240 MHz |
| **SRAM** | 520 KB | 512 KB | 400 KB | 320 KB |
| **PSRAM** | ❌ | 8 MB | ❌ | ❌ |
| **Flash** | 4 MB | 8/16 MB | 4 MB | 4 MB |
| **WiFi** | 2.4 GHz | 2.4 GHz | 2.4 GHz | 2.4 GHz |
| **Bluetooth** | 4.2 + BLE | 5.0 + BLE | 5.0 + BLE | ❌ |
| **USB** | Micro-USB | USB-C | USB-C | USB-C |
| **Native USB** | ❌ | ✅ CDC/JTAG | ✅ CDC | ✅ CDC |
| **Secure Boot** | v1 | v2 | v2 | v2 |
| **Flash Encryption** | ✅ | ✅ | ✅ | ✅ |
| **Price** | ~$5 | ~$12 | ~$6 | ~$6 |

---

## Branch Protection

| Branch | Protection |
|--------|------------|
| `main` | Required reviews: 2, CI pass, linear history |
| `develop` | CI pass |
| `board/*` | Maintainer review, hardware test logs required |

---

## Adding a New Board Checklist

- [ ] Create `board/<slug>` branch from `develop`
- [ ] Add board config in `config.h`
- [ ] Add PlatformIO env in `platformio.ini`
- [ ] Update `docs/BOARDS.md` with pinout table
- [ ] Test firmware builds: `pio run -e <env>`
- [ ] Flash to hardware, test provisioning flow
- [ ] Document quirks in `docs/BOARDS.md`
- [ ] Add hardware photos to PR
- [ ] Open PR to `develop`

---

**Want to port to a new board?** Open an issue with `board support` label — we'll help you get started!