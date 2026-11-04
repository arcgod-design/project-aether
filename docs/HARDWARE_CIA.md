# Project Aether — Tactical Node Hardware (BRUTAL EDITION)
**Version:** 1.0 | **Classification:** INTERNAL

---

## THE ONLY HARDWARE THAT WORKS

| Board | Chip | Verdict | Reason |
|-------|------|---------|--------|
| **ESP32 DevKit (WROOM-32)** | ESP32 | ✅ **USE THIS** | 520KB RAM, crypto accel, native USB, HTTPS capable |
| **ESP32-S3 DevKitC-1** | ESP32-S3 | ✅ **BETTER** | 512KB SRAM + 8MB PSRAM, USB OTG, faster crypto |
| **NodeMCU** | ESP8266 | ❌ **TRASH** | 80KB RAM, no crypto, no native USB, no HTTPS, different SDK |

**You have 1 usable board (DevKit). The NodeMCU goes in the bin.**

---

## MINIMAL BOM (₹450)

| Item | Cost | Source |
|------|------|--------|
| ESP32 DevKit (WROOM-32) | ₹450 | Amazon.in / Robu.in |
| **TOTAL** | **₹450** | |

**No secure element. No custom PCB. No enclosure. No buttons. No LEDs. Bare board.**

---

## WHY NO SECURE ELEMENT (ATECC608B)?

| Feature | Without SE | With SE |
|---------|-----------|---------|
| Key isolation | ❌ Keys in RAM | ✅ Keys in SE |
| Attestation | ❌ No | ✅ Yes |
| Cost | ₹0 | +₹300 |
| Complexity | Low | High (I2C, provisioning, slots) |
| **MVP Need** | **Not required** | Phase 1 |

**For MVP: Keys in RAM is acceptable. Document the risk. Move on.**

---

## HOST SIDE = LAPTOP (NON-NEGOTIABLE)

```
┌─────────────────────────────────────────────────────────────────┐
│                    HOST SIDE REQUIREMENTS                        │
└─────────────────────────────────────────────────────────────────┘

| Task | ESP32 | Laptop |
|------|-------|--------|
| AES-256-GCM encrypt | ✅ HW accel | ✅ |
| Ed25519 sign | ✅ HW accel | ✅ |
| HKDF key derivation | ✅ HW accel | ✅ |
| Zip large folders | ❌ No FS | ✅ |
| Pinata HTTPS upload | ❌ RAM limit | ✅ |
| web3.py contract call | ❌ Too heavy | ✅ |
| Master key storage | ⚠️ NVS only | ✅ Keyring/Env |
| Large file handling | ❌ 4MB Flash | ✅ SSD |

**ESP32 CANNOT BE THE HOST. END OF DISCUSSION.**
```

---

## YOUR 3 BOARDS — ASSIGNMENT

| Board | Role |
|-------|------|
| **DevKit (ESP32)** | **TACTICAL NODE** — Flash firmware, deploy |
| **Upload Board (ESP32)** | **SPARE** — If DevKit dies |
| **NodeMCU (ESP8266)** | **DRAWER** — Cannot run this firmware |

---

## FIRMWARE: HTTPS IS MANDATORY (FOR SHA-256)

```cpp
// tactical_node_https.ino — Key additions for HTTPS
#include <WiFiServerSecure.h>
#include <esp_crt_bundle.h>

// Generate self-signed cert at boot (or embed)
// Pin SHA-256 in frontend → browser allows crypto.subtle

WiFiServerSecure httpsServer(443);
httpsServer.setCertificates(cert_pem, key_pem);
httpsServer.begin();

// Frontend pinning:
const EXPECTED_CERT_SHA256 = "sha256/AbCdEf...";
// fetch() to https://aether-node.local/ → SHA-256 WORKS
```

**Without HTTPS → No SHA-256 verification. Period.**

---

## DEPLOYMENT CHECKLIST (BRUTAL)

- [ ] **1 ESP32 DevKit** — Flash `tactical_node_https.ino`
- [ ] **1 Laptop** — Run `tactical_upload.py` (host)
- [ ] **1 Phone** — Connect to Node AP → Preview files
- [ ] **NodeMCU** → **In drawer**
- [ ] **HTTPS working** → `https://aether-node.local/` → SHA-256 passes
- [ ] **Demo** → Connect → Preview → Download → Done

---

## COST: ₹450 TOTAL

| Person | Pays |
|--------|------|
| 6 people split 1 node | ₹75 each |
| 3 nodes (spares) | ₹1,350 total = ₹225 each |

**Stop over-engineering. Build the one thing that works.**

---

**END OF BRUTAL HARDWARE SPEC**