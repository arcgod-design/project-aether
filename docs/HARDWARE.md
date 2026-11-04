# Project Aether — Hardware Integration Guide
**Version:** 1.0 | **Classification:** INTERNAL

---

## 1. BILL OF MATERIALS (BOM)

### 1.1 Per Node (×3 for threshold)

| Qty | Component | Part Number | Purpose | Notes |
|-----|-----------|-------------|---------|-------|
| 1 | ESP32 Dev Module | ESP32-WROVER-B / DevKitC V4 | Main MCU | 4MB PSRAM required for TLS |
| 1 | Secure Element | ATECC608B-TNGTLS (SOIC-8) | Root of trust | Pre-provisioned or self-provision |
| 1 | Button | 6×6mm tactile (momentary) | Human approval | GPIO 4 + pull-up |
| 3 | LED | 3mm (R/G/B) or RGB | Status | GPIO 5/18/19 + 220Ω |
| 1 | LDO | MCP1700-3302E | 3.3V regulator | ESP32 has onboard; external for SE |
| 4 | Capacitor | 0.1µF ceramic | Decoupling | VCC/GND near each IC |
| 1 | Capacitor | 10µF electrolytic | Bulk decoupling | Near ESP32 EN pin |
| 1 | Resistor | 10kΩ | I2C pull-up (SDA) | ATECC608B SDA |
| 1 | Resistor | 10kΩ | I2C pull-up (SCL) | ATECC608B SCL |
| 1 | Resistor | 10kΩ | Button pull-up | GPIO 4 to 3V3 |
| 1 | PCB | Custom / Perfboard | Integration | See layout guide |
| 1 | Enclosure | 3D printed / Aluminum | Physical security | Tamper-evident |

### 1.2 Provisioning Station (One-time)

| Qty | Component | Purpose |
|-----|-----------|---------|
| 1 | Air-gapped laptop | Offline key generation |
| 1 | ESP32 + ATECC608B (spare) | Target for provisioning |
| 1 | JTAG debugger | ESP-Prog / FT2232H | Firmware + SE config |
| 1 | HSM / Paper backup | Master key storage |
| 1 | Camera | Provisioning ceremony recording |

---

## 2. SCHEMATIC

```
                    ┌─────────────────────────────────────┐
                    │           ESP32-WROVER-B             │
                    │                                     │
                    │  GPIO 21 ──────────────────┐        │
                    │  GPIO 22 ──────────────────┤        │
                    │  GPIO 4  ──────┐           │        │
                    │  GPIO 5  ──────┤           │        │
                    │  GPIO 18 ──────┤           │        │
                    │  GPIO 19 ──────┤           │        │
                    │                ▼           │        │
                    │         ┌─────────────┐    │        │
                    │         │   I2C BUS   │    │        │
                    │         │  (100kHz)   │    │        │
                    │         └──────┬──────┘    │        │
                    └───────────────┼────────────┘        │
                                    │                    │
                    ┌───────────────┼────────────────────┘
                    │               │
                    ▼               ▼
             ┌─────────────┐ ┌─────────────┐
             │  ATECC608B  │ │  BUTTON     │
             │  (SOIC-8)   │ │  (Momentary)│
             │             │ │             │
             │ SDA ────────┤ │ GPIO 4 ─────┤
             │ SCL ────────┤ │ (pull-up)   │
             │ VCC ────────┤ │             │
             │ GND ────────┤ │             │
             └─────────────┘ └─────────────┘
                    │
                    ▼
             ┌─────────────┐
             │   LEDs      │
             │  (Status)   │
             │             │
             │ GPIO 5 ── R │
             │ GPIO 18 ─ G │
             │ GPIO 19 ── B│
             └─────────────┘
```

---

## 3. PCB LAYOUT GUIDELINES

### 3.1 Critical Rules
| Rule | Specification |
|------|---------------|
| I2C trace length | < 50mm, matched length SDA/SCL |
| I2C trace spacing | > 3× trace width from other signals |
| Ground plane | Solid under ESP32 + ATECC608B |
| Decoupling caps | 0.1µF within 2mm of each VCC pin |
| ATECC608B placement | Center of board, away from edges |
| Button trace | Short, away from RF section |
| Antenna keepout | 5mm radius from ESP32 antenna |

### 3.2 Layer Stack (4-layer)
```
Layer 1: Top (Signals + Components)
Layer 2: GND (Solid)
Layer 3: VCC (3.3V pour)
Layer 4: Bottom (Signals)
```

---

## 4. ATECC608B CONFIGURATION

### 4.1 Zone Configuration (Write Once at Provisioning)

| Slot | Purpose | Config | Lock |
|------|---------|--------|------|
| 0 | Device Identity Private Key (ECDSA P-256) | `Private`, `NeverRead` | Yes |
| 1 | Device Identity Public Key | `Public` | Yes |
| 2 | Attestation Private Key (ECDSA P-256) | `Private`, `NeverRead` | Yes |
| 3 | Attestation Public Key | `Public` | Yes |
| 4 | KEK (Key Encryption Key) | `Secret`, `AES`, `NoMac` | Yes |
| 5 | ESP-NOW Session Key | `Secret`, `AES`, `NoMac` | Yes |
| 6 | Unused / Future | - | - |
| 7 | Unused / Future | - | - |

### 4.2 Counter Configuration
| Counter | Purpose | Initial |
|---------|---------|---------|
| 0 | Session monotonic counter | 0 |
| 1 | Firmware update counter | 0 |
| 2 | Provisioning counter | 1 (set at factory) |

### 4.3 Key Generation (At Provisioning)
```
# Slot 0: Device Identity
atecc608b_genkey(slot=0, public_key_out=pk_device)

# Slot 2: Attestation
atecc608b_genkey(slot=2, public_key_out=pk_attest)

# Slot 4: KEK (imported, not generated)
atecc608b_write_key(slot=4, key=KEK_from_MK)

# Slot 5: ESP-NOW Key
atecc608b_write_key(slot=5, key=ENC_NOW_from_MK)
```

---

## 5. FIRMWARE ARCHITECTURE

### 5.1 Core Tasks (FreeRTOS)

```
┌─────────────────────────────────────────────────────────────────┐
│                      FREERTOS TASKS                              │
└─────────────────────────────────────────────────────────────────┘

Task: main_loop (Core 0, Priority 5)
  ├─ WiFi Manager (AP/STA/Provisioning)
  ├─ HTTPS Server (mbedTLS)
  ├─ Session Manager
  ├─ Policy Enforcer
  └─ Button Handler

Task: espnow_mesh (Core 1, Priority 6)
  ├─ ESP-NOW RX Callback
  ├─ ESP-NOW TX Queue
  ├─ Peer Manager
  └─ Approval Aggregator

Task: atecc608b_driver (Core 0, Priority 4)
  ├─ I2C Mutex
  ├─ Command Queue
  └─ Response Handler

Task: watchdog (Core 0, Priority 10)
  ├─ Task health monitoring
  └─ Panic recovery
```

### 5.2 Memory Map (ESP32-WROVER-B, 4MB PSRAM)

| Region | Size | Contents |
|--------|------|----------|
| IRAM | 128KB | ISR handlers, crypto primitives |
| DRAM | 320KB | FreeRTOS, mbedTLS, buffers |
| PSRAM | 4MB | HTTPS buffers, TLS session cache, file chunks |
| Flash | 4MB | Firmware (1.2MB), Certs (256KB), NVS (1MB), OTA (1.5MB) |

### 5.3 Partition Table (partitions.csv)

```
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x4000,
otadata,  data, ota,     0xD000,  0x2000,
phy_init, data, phy,     0xF000,  0x1000,
factory,  app,  factory, 0x10000, 0x140000,
ota_0,    app,  ota_0,   0x150000,0x140000,
ota_1,    app,  ota_1,   0x290000,0x140000,
certs,    data, fat,     0x3D0000,0x40000,  // 256KB for certs
nvs2,     data, nvs,     0x410000,0x20000,  // 128KB extra NVS
```

---

## 6. BOOT SEQUENCE

```
BOOT
  │
  ├─▶ ROM Bootloader
  │     │
  │     ├─▶ Secure Boot v2 (if enabled)
  │     │     │
  │     │     └─▶ Verify bootloader signature (RSA-3072)
  │     │
  │     └─▶ Load 2nd stage bootloader (factory/OTA)
  │
  ├─▶ 2nd Stage Bootloader
  │     │
  │     ├─▶ Initialize Flash, PSRAM
  │     ├─▶ Verify app partition signature (ECDSA P-256)
  │     ├─▶ Compute firmware_hash = SHA256(app_image)
  │     ├─▶ Store firmware_hash in RTC memory
  │     └─▶ Jump to app entry
  │
  ├─▶ App Start (app_main)
  │     │
  │     ├─▶ Initialize NVS, ESP-IDF
  │     ├─▶ Initialize I2C (ATECC608B)
  │     │     │
  │     │     ├─▶ Wake SE
  │     │     ├─▶ Verify SE config lock
  │     │     ├─▶ Read device_id (Slot 1 public)
  │     │     └─▶ Read firmware_hash from RTC
  │     │
  │     ├─▶ Initialize WiFi (AP mode: Aether-Setup)
  │     ├─▶ Start DNS + HTTPS Server
  │     ├─▶ Initialize ESP-NOW
  │     │     │
  │     │     ├─▶ Load peer MACs from NVS
  │     │     ├─▶ Set encryption key (Slot 5)
  │     │     └─▶ Register RX callback
  │     │
  │     ├─▶ Start FreeRTOS tasks
  │     └─▶ Print boot banner + device_id
  │
  └─▶ RUNNING
```

---

## 7. PROVISIONING CEREMONY (DETAILED)

### 7.1 Prerequisites
- Air-gapped laptop (no network ever)
- Clean OS (fresh install, verified checksums)
- Hardware: ESP32 + ATECC608B on jig, JTAG debugger
- Master Key (MK) generated offline: `openssl rand 32 > mk.bin`
- Provisioning log template (paper + digital photo)

### 7.2 Ceremony Steps

```
STEP 1: GENERATE DEVICE IDENTITY
  │
  ├─ On air-gapped laptop:
  │   openssl ecparam -name prime256v1 -genkey -noout -out device_sk.pem
  │   openssl ec -in device_sk.pem -pubout -out device_pk.pem
  │
  ├─ Record:
  │   Device ID: AETHER-NODE-XXX (sequential)
  │   Device PK: (base64 of device_pk.pem)
  │   Timestamp: ISO8601 UTC
  │   Operator: Name + signature
  │
  └─ Verify: openssl ec -in device_sk.pem -check

STEP 2: PROGRAM ATECC608B (via JTAG + OpenOCD)
  │
  ├─ Connect: ESP-Prog → ESP32 JTAG
  ├─ OpenOCD config: esp32 + atecc608b
  │
  ├─ Write Slot 0 (Device Identity Private):
  │   atecc608b_priv_write(slot=0, key=device_sk)
  │   atecc608b_lock(slot=0)
  │
  ├─ Write Slot 1 (Device Identity Public):
  │   atecc608b_pub_write(slot=1, key=device_pk)
  │   atecc608b_lock(slot=1)
  │
  ├─ Generate Slot 2 (Attestation):
  │   atecc608b_genkey(slot=2) → pk_attest
  │   atecc608b_lock(slot=2)
  │
  ├─ Write Slot 4 (KEK from MK):
  │   KEK = HKDF(MK, salt=device_id, info="aether:kek", L=32)
  │   atecc608b_key_write(slot=4, key=KEK, config=AES_256)
  │   atecc608b_lock(slot=4)
  │
  ├─ Write Slot 5 (ESP-NOW Key):
  │   ENC_NOW = HKDF(MK, salt=device_id, info="aether:espnow", L=16)
  │   atecc608b_key_write(slot=5, key=ENC_NOW, config=AES_128_CCM)
  │   atecc608b_lock(slot=5)
  │
  ├─ Configure Counters:
  │   atecc608b_counter_config(0, mode=monotonic)
  │   atecc608b_counter_config(1, mode=monotonic)
  │   atecc608b_counter_config(2, mode=monotonic)
  │   atecc608b_counter_inc(2)  # Provisioning counter = 1
  │
  └─ Lock Config Zone (permanent)
      atecc608b_lock_config()

STEP 3: GENERATE DEVICE CERTIFICATE
  │
  ├─ On air-gapped laptop (CA root offline):
  │   openssl req -new -key device_sk.pem -subj "/CN=Aether-Node-XXX" -out device.csr
  │   openssl ca -config ca.conf -in device.csr -out device_cert.pem -days 3650
  │
  ├─ Verify chain:
  │   openssl verify -CAfile ca_cert.pem device_cert.pem
  │
  └─ Bundle for firmware:
      device_cert.der + ca_cert.der → embed in firmware certs partition

STEP 4: REGISTER ON-CHAIN
  │
  ├─ On networked machine (separate from air-gapped):
  │   cast send <AetherPolicy> "authorizeGate(address,bool)" <device_address> true
  │   cast send <AetherPolicy> "setDeviceCert(address,bytes)" <device_address> <cert_der>
  │
  ├─ Record transaction hash
  │
  └─ Verify:
      cast call <AetherPolicy> "authorizedGates(address)" <device_address>

STEP 5: SEAL PROVISIONING LOG
  │
  ├─ Digital: SHA256(log.txt) → record on paper + photo
  ├─ Physical: Print log, sign, seal in tamper-evident envelope
  ├─ Store: One copy with operator, one in secure archive
  │
  └─ NEVER store MK, device_sk on any networked system
```

### 7.3 Post-Provisioning Verification

| Check | Method | Expected |
|-------|--------|----------|
| SE Config Locked | `atecc608b_is_locked()` | All slots locked |
| Device PK readable | `atecc608b_read_pubkey(0)` | Matches provisioning |
| Attestation PK readable | `atecc608b_read_pubkey(2)` | New keypair generated |
| KEK unwraps test vector | `atecc608b_aes_gcm_decrypt(4, ...)` | Returns known plaintext |
| ESP-NOW key works | Send encrypted frame to peer | Peer decrypts |
| Counters increment | `atecc608b_counter_inc(0)` | Returns 1, then 2... |
| Firmware hash matches | `SHA256(firmware) == RTC_firmware_hash` | True |

---

## 8. OTA UPDATE PROCEDURE

### 8.1 Signed Firmware
```
# On build machine (not air-gapped):
# 1. Build firmware
idf.py build

# 2. Sign with manufacturer key (offline HSM)
openssl dgst -sha256 -sign mfr_sk.pem -out firmware.sig build/merged.bin

# 3. Package
cat build/merged.bin firmware.sig > firmware_update.bin
```

### 8.2 ESP32 Verification
```
# On ESP32 (bootloader):
verify_signature(firmware_update.bin, mfr_pk)
if valid:
    write to ota_1 partition
    set boot partition = ota_1
    reboot
else:
    abort, stay on factory
```

### 8.3 Counter Increment
```
# After successful OTA boot:
atecc608b_counter_inc(1)  # Firmware update counter
```

---

## 9. DIAGNOSTICS & DEBUGGING

### 9.1 Status LED Codes
| Pattern | Meaning |
|---------|---------|
| Solid Blue | Provisioning AP active (Aether-Setup) |
| Blinking Blue | Connecting to phone hotspot |
| Solid Green | Connected, mDNS ready, awaiting session |
| Blinking Green | Session active, awaiting PIN |
| Solid Green + Blue | Session authorized, file transfer |
| Red Blink (1) | WiFi connection failed |
| Red Blink (2) | ESP-NOW mesh error |
| Red Blink (3) | ATECC608B error |
| Red Solid | Critical fault (check serial) |

### 9.2 Serial Console Commands (UART 115200)
```
> help                    # List commands
> status                  # System status
> session                 # Current session info
> peers                   # ESP-NOW peers
> attest                  # Generate attestation quote
> test_se                 # ATECC608B self-test
> test_espnow             # Send test frame
> reboot                  # Soft reboot
> factory_reset           # WIPE ALL (requires button hold)
```

### 9.3 Common Failure Modes

| Symptom | Cause | Fix |
|---------|-------|-----|
| ATECC608B not found | I2C wiring / address | Check SDA/SCL, 0x60 address |
| SE command fails | Config not locked | Re-provision |
| ESP-NOW send fails | Peer MAC wrong / channel | Verify peer MACs, channel 1 |
| HTTPS cert verify fails | Time not synced | NTP sync before TLS |
| mDNS not resolving | Network blocks multicast | Use IP fallback |
| Button not detected | GPIO config / pull-up | Check GPIO 4 + 10kΩ |

---

## 10. PHYSICAL SECURITY

### 10.1 Tamper Evidence
- **Enclosure**: Ultrasonic weld or security screws
- **Seal**: Tamper-evident holographic sticker over seam
- **PCB**: Conformal coating (opaque) on SE + critical traces
- **Button**: Recessed, requires tool (prevents accidental press)

### 10.2 Anti-Clone Measures
- Device certificate pinned in frontend
- Attestation quote includes firmware hash
- Monotonic counter prevents replay
- SE keys never readable (Slot 0, 2, 4, 5 locked)

---

## 11. TEST PROCEDURES

### 11.1 Factory Test (Per Unit)
```
[ ] Visual inspection (solder, components)
[ ] Power on → boot banner prints
[ ] ATECC608B: all slots locked, keys readable
[ ] WiFi: AP starts, captive portal loads
[ ] ESP-NOW: ping peer, receive ack
[ ] HTTPS: cert valid, handshake completes
[ ] Button: press detected, LED responds
[ ] Counters: increment correctly
[ ] Attestation: quote verifies
[ ] Firmware hash: matches build
```

### 11.2 Integration Test (3-Node Mesh)
```
[ ] All 3 nodes boot, show unique device IDs
[ ] Mesh forms: each sees 2 peers
[ ] Session create → broadcast → 2 approvals
[ ] Contract: session authorized = true
[ ] Browser: full retrieve + decrypt + verify
[ ] Cover traffic: dummy requests observed
```

---

**END OF HARDWARE INTEGRATION GUIDE**