# Board Support — Community Driven

> **Anyone can add a board.** No gatekeeping — if it runs ESP32 Arduino, it can run Aether.

---

## How to Add Your Board

### 1. Fork & Branch
```bash
git fork https://github.com/your-org/project-aether
git clone your-fork
cd project-aether
git checkout -b board/your-board-slug
```

### 2. Add Board Config (5 minutes)

Edit `firmware/src/config.h`:
```cpp
#if defined(BOARD_YOUR_BOARD_SLUG)
  #define BOARD_NAME        "Your Board Name"
  #define BUTTON_PIN        4        // GPIO for physical button
  #define LED_RED_PIN       18
  #define LED_GREEN_PIN     5
  #define LED_BLUE_PIN      19
  #define HAS_PSRAM         0        // 1 if PSRAM present
  #define HAS_USB_CDC       0        // 1 if native USB CDC
#elif defined(BOARD_ESP32_DEVKIT)
  // existing...
#endif
```

### 2b. Add PlatformIO Environment
Edit `firmware/platformio.ini`:
```ini
[env:your-board-slug]
platform = espressif32
board = your-board-id          ; from boards.txt
framework = arduino
build_flags = -DBOARD_YOUR_BOARD_SLUG
board_build.partitions = partitions.csv
```

### 3. Document Your Board
Edit `docs/BOARDS.md` — add pinout table + photo.

### 4. Test & PR
```bash
pio run -e your-board-slug      # Build test
# Flash to hardware, test provisioning flow
git push origin board/your-board-slug
# Open PR to `develop` with:
# - Board name + photos
# - Serial log of provisioning
# - Pinout table
```

---

## No Gatekeeping Rules

| What you need | What you don't need |
|--------------|---------------------|
| ESP32 board that runs Arduino | Permission from maintainers |
| Working provisioning flow | Perfect code on first try |
| Serial log showing success | Perfect documentation |
| Photo of your board | Perfect English |

**If it builds, boots, and provisions — merge it.**

---

## Board Branch Naming

```
board/esp32s3-devkit
board/esp32c3-devkit
board/esp32c3-devkitm
board/esp32s2-devkit
board/esp32c6-devkit
board/m5stack-core2
board/lilygo-tdisplay-s3
board/lilygo-tbeam
board/seeed-xiao-esp32s3
board/seeed-xiao-esp32c3
board/custom-myboard
```

**Slug rules:** lowercase, hyphens only, descriptive.

---

## What You Get

| You Provide | You Get |
|-------------|---------|
| Board config + test log | Your board in `docs/BOARDS.md` |
| Serial log + photo | Your board in `boards/` gallery |
| Working PR | Merged to `develop` → `main` |

---

## Community Gallery

| Board | Contributor | Status |
|-------|-------------|--------|
| ESP32 DevKit (WROOM-32) | @arc | ✅ Primary |
| *Your board here* | *You?* | 🟢 Open |

---

## Need Help?

- **Discord**: `#board-support` channel
- **Issues**: Label `board-support`
- **Discussions**: "Show and Tell" category

**No board is too weird.** If it runs Arduino ESP32, it runs Aether.

---

> *"The best board support is the one you write yourself."*