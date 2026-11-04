# Project Aether — Technical Specification
**Version:** 1.0 | **Classification:** INTERNAL — TECHNICAL SPECIFICATION  
**Status:** MVP OPERATIONAL (Single ESP32) | **Target:** Hardware-Gated Edge Dissemination System

---

## 1. EXECUTIVE SUMMARY (NO MARKETING)

### What Aether Is
A **single-ESP32 hardware gate** that enforces physical proximity for blockchain-anchored, IPFS-backed file retrieval. The phone provides internet and rendering; the ESP32 provides session authorization and local HTTPS delivery.

### What Aether Is NOT
- An anonymity network (Tor/I2P does that better)
- A secure messenger (Signal does that better)
- A replacement for OPSEC (OPSEC is a discipline, not a tool)
- "CIA-grade" — that phrase means nothing. We specify **exact guarantees**.
- A 2-of-3 threshold system (requires 3 ESP32s; we have 1)
- Zero-forensic-footprint (impossible on standard mobile OS)

### Honest MVP State (September 2025)
| Component | Status | Gap to Production |
|-----------|--------|-------------------|
| ESP32 firmware (Design B) | ✅ Working | No TLS, no secure element, no attestation |
| Smart contract (Base Sepolia) | ✅ Deployed | No policy engine, no revocation, no threshold |
| Host script (Python) | ✅ Working | No encryption, no signing, no key management |
| Frontend (browser) | ✅ Working | HTTP only, no integrity on HTTP, no CSP |
| IPFS retrieval | ✅ Working | Gateway fallback, no private retrieval |

**Bottom line:** MVP proves the *architecture*. Every cryptographic guarantee is **currently absent**. This document specifies how to add them.

---

## 2. THREAT MODEL (BRUTAL HONESTY)

### Adversary Capabilities We Assume
| Capability | Assumed? | Notes |
|------------|----------|-------|
| Passive network sniff (WiFi/cellular) | ✅ YES | Default |
| Active MITM on WiFi/cellular | ✅ YES | Default |
| Compromised phone OS (root/jailbreak) | ✅ YES | Default |
| Physical access to phone (forensics) | ✅ YES | Cellebrite, GrayKey |
| Physical access to ESP32 (clone/replace) | ✅ YES | **Critical gap in MVP** |
| Malicious IPFS gateway | ✅ YES | Pinata/Cloudflare could lie |
| Blockchain reorg (testnet) | ✅ YES | Base Sepolia not finality-guaranteed |
| Insider at Pinata/Alchemy | ✅ YES | Metadata leakage |
| Side-channel on ESP32 | ⚠️ PARTIAL | Power/EM analysis possible |

### Guarantees We **DO NOT** Claim
- ❌ "Untraceable" — carrier sees phone IP
- ❌ "Zero forensic footprint" — browser caches, swap, screenshots exist
- ❌ "Unbreakable" — 4-digit PIN = 10,000 guesses (public chain = offline brute)
- ❌ "Air-gapped" — ESP32 connects to internet via phone hotspot
- ❌ "2-of-3 threshold" — only 1 ESP32 available

### Guarantees We **DO** Claim (Post-Production)
| Guarantee | Mechanism |
|-----------|-----------|
| Retrieval requires physical proximity to authorized node | ESP32 session nonce + hardware button |
| File integrity verifiable end-to-end | AES-GCM tag + on-chain SHA-256 commitment |
| Authorization policy immutable once deployed | Base L2 smart contract |
| Compromised long-term key ≠ past sessions | Double Ratchet per session |
| Node compromise detected | Remote attestation via secure element |

---

## 3. SYSTEM ARCHITECTURE (SINGLE ESP32)

### 3.1 High-Level Data Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           PROJECT AETHER — SINGLE ESP32 OVERVIEW             │
└─────────────────────────────────────────────────────────────────────────────┘

    HOST                          ESP32 (Single Node)                PHONE
    ────                          ─────────────────────             ─────
     │                                │                              │
     │  Encrypt + Sign + Upload       │                              │
     │  ──────────────────────────►   │                              │
     │           (Pinata IPFS)        │                              │
     │                                │                              │
     │  Register on-chain             │                              │
     │  ──────────────────────────►   │                              │
     │     (Base Sepolia)             │                              │
     │                                │                              │
     │                         ┌──────▼──────┐                       │
     │                         │  Aether-Setup│  (Phone connects)   │
     │                         │     AP       │                     │
     │                         └──────┬──────┘                       │
     │                                │                              │
     │                         Provision to                          │
     │                         Phone Hotspot                         │
     │                                │                              │
     │                         ┌──────▼──────┐                       │
     │                         │   STA Mode  │  (Joins phone net)    │
     │                         │  (Internet) │                       │
     │                         └──────┬──────┘                       │
     │                                │                              │
     │                         mDNS: aether-node.local               │
     │                                │                              │
     │                         ┌──────▼──────┐                       │
     │                         │  HTTPS Dash │  (Pinned cert)        │
     │                         │   + Session │                       │
     │                         └──────┬──────┘                       │
     │                                │                              │
     │                    Session nonce + PIN                        │
     │                    ─────────────►                              │
     │                                │                              │
     │                    Contract.getFile()                         │
     │                    ─────────────►                              │
     │                                │                              │
     │                    Returns CID + wrapped DEK                  │
     │                    ◄─────────────                              │
     │                                │                              │
     │                    Fetch from IPFS                            │
     │                    ──────────────────► (HTTPS gateway)        │
     │                                │                              │
     │                    Decrypt (WASM)                             │
     │                    Verify SHA-256                             │
     │                    Render ephemeral                           │
     │                                │                              │
```

### 3.2 Component Responsibilities

| Component | Responsibility | Trust Assumption |
|-----------|----------------|------------------|
| **Host** | Encrypt, sign, upload, register | Trusted (operator) |
| **Pinata IPFS** | Store encrypted blob | Untrusted (confidentiality via crypto) |
| **Base Sepolia** | Immutable policy + commitments | Trusted for finality (L2) |
| **ESP32** | Hardware gate, session auth, local HTTPS | **Root of trust** (physical) |
| **Phone Browser** | UI, fetch, decrypt, render | Untrusted (compromised assumed) |
| **IPFS Gateway** | Serve encrypted blob | Untrusted (integrity via hash) |

---

## 4. EMBEDDED FIRMWARE ARCHITECTURE (CORE)

### 4.1 Firmware Modules (Current Implementation)

```
firmware/src/
├── main.cpp                 # Entry point, task creation
├── network/
│   ├── wifi_manager.cpp     # AP/STA/Provisioning state machine
│   ├── captive_portal.cpp   # DNS hijack + HTTP redirect
│   ├── mdns_responder.cpp   # mDNS for aether-node.local
│   └── https_server.cpp     # mbedTLS HTTPS (planned)
├── session/
│   ├── session_manager.cpp  # Session nonce, PIN hash, TTL, attempts
│   └── pin_verifier.cpp     # keccak256(PIN || nonce) verification
├── crypto/
│   └── (placeholder for ATECC608B driver)
├── ui/
│   └── dashboard_html.h     # Embedded dashboard (PROGMEM)
└── system/
    ├── nvs_storage.cpp      # NVS for WiFi creds, device config
    ├── ota_updater.cpp      # Signed OTA (planned)
    └── watchdog.cpp         # Task health monitoring
```

### 4.2 WiFi Provisioning State Machine (Design B)

```
┌─────────────────────────────────────────────────────────────────┐
│                    WIFI PROVISIONING FLOW                        │
└─────────────────────────────────────────────────────────────────┘

BOOT
  │
  ├─▶ WiFi.mode(WIFI_AP)
  │     │
  │     ├─▶ WiFi.softAP("Aether-Setup", "aether1234")
  │     │     │
  │     │     ├─▶ DNSServer.start(53, "*", 192.168.4.1)
  │     │     │     │
  │     │     │     └─▶ Captive portal: all DNS → 192.168.4.1
  │     │     │
  │     │     └─▶ WebServer.on("/", HTTP_GET, handleDashboard)
  │     │           │
  │     │           ├─▶ If STA not connected: serve setup page
  │     │           │     │
  │     │           │     └─▶ Form: SSID + Password → POST /connect
  │     │           │
  │     │           └─▶ If STA connected: serve dashboard (INDEX_HTML)
  │     │
  ├─▶ handleConnect():
  │     │
  │     ├─▶ WiFi.mode(WIFI_AP_STA)  // Keep AP alive for response
  │     ├─▶ WiFi.begin(ssid, pass)
  │     ├─▶ Wait 20s for WL_CONNECTED
  │     │     │
  │     │     ├─▶ SUCCESS: staConnected = true
  │     │     │     │
  │     │     │     ├─▶ MDNS.begin("aether-node")
  │     │     │     ├─▶ Return success page with STA IP + mDNS URL
  │     │     │     └─▶ Start 8s timer → disable AP
  │     │     │
  │     │     └─▶ FAIL: Return error page, retry
  │     │
  ├─▶ Loop(): After provisioning success + 8s
  │     │
  │     └─▶ WiFi.softAPdisconnect(true)  // AP down, STA stays up
  │           │
  │           └─▶ MDNS re-announce on STA interface
  │
  └─▶ RUNNING: STA connected to phone hotspot
        │
        ├─▶ Phone reconnects to its own hotspot
        ├─▶ Browser → http://aether-node.local/ (or STA IP)
        ├─▶ Dashboard served via HTTPS (planned) / HTTP (current)
        └─▶ Session flow begins
```

### 4.3 Session Management (Embedded)

```cpp
// session_manager.h
struct SessionState {
    uint8_t session_id[32];        // Random per session
    uint8_t pin_hash[32];          // keccak256(PIN || session_nonce)
    uint32_t created_at;           // Unix timestamp
    uint8_t attempts;              // 0-3
    bool authorized;               // Set by button press (future)
    bool active;                   // Session valid
    uint32_t monotonic_counter;    // Replay protection (NVS)
};

class SessionManager {
    // NVS-backed monotonic counter (survives reboot)
    uint32_t get_next_counter();
    
    // Session lifecycle
    bool create_session(const char* pin, uint8_t* session_id_out);
    bool verify_pin(const uint8_t* session_id, const char* pin);
    void increment_attempts(const uint8_t* session_id);
    bool is_expired(const uint8_t* session_id);  // 60s TTL
    void expire_session(const uint8_t* session_id);
};
```

### 4.4 Captive Portal Implementation (Embedded Detail)

```cpp
// captive_portal.cpp
class CaptivePortal {
    DNSServer dns_server;
    WebServer http_server;
    IPAddress ap_ip = IPAddress(192, 168, 4, 1);
    
    void begin() {
        // DNS: intercept ALL queries → ap_ip
        dns_server.start(53, "*", ap_ip);
        
        // HTTP: serve setup page on any path
        http_server.onNotFound([this]() {
            if (!wifi_manager.is_sta_connected()) {
                serve_setup_page();
            } else {
                serve_dashboard();
            }
        });
        
        http_server.on("/connect", HTTP_POST, [this]() {
            handle_wifi_provisioning();
        });
        
        http_server.on("/health", HTTP_GET, [this]() {
            serve_json_status();
        });
        
        http_server.begin();
    }
    
    void loop() {
        dns_server.processNextRequest();
        http_server.handleClient();
    }
};
```

### 4.5 mDNS Responder (Embedded Detail)

```cpp
// mdns_responder.cpp
class MdnsResponder {
    const char* hostname = "aether-node";
    
    void begin() {
        if (MDNS.begin(hostname)) {
            MDNS.addService("http", "tcp", 80);   // HTTP
            MDNS.addService("https", "tcp", 443); // HTTPS (planned)
            MDNS.setInstanceName("Project Aether Node");
        }
    }
    
    void reannounce_on_sta() {
        MDNS.end();
        MDNS.begin(hostname);  // Rebind to STA interface IP
    }
};
```

---

## 5. CRYPTOGRAPHIC PROTOCOLS

### 5.1 Key Hierarchy

```
MASTER KEY (MK) ──────────────────────────────────────────────────────┐
  │                                                                     │
  ├─▶ DEK_i = HKDF(MK, "file" || file_id || nonce)     (per-file)     │
  │                                                                     │
  ├─▶ KEK_j = HKDF(MK, "device" || device_id)          (per-ESP32)    │
  │                                                                     │
  └─▶ SESSION_KEY = DoubleRatchet(initiator, responder) (per-session) │
```

### 5.2 File Encryption (Host → IPFS)

```
Input: plaintext file P, file_id, master_key MK
Output: ciphertext C, nonce N, tag T, encrypted DEK wrapper W

1. DEK = HKDF(MK, salt=file_id, info="aether:dek", L=32)
2. N = random(12 bytes)  // 96-bit nonce
3. C, T = AES256-GCM(DEK, N, P, AAD=file_id)
4. For each authorized device_j:
     KEK_j = HKDF(MK, salt=device_id_j, info="aether:kek", L=32)
     W_j = AES256-GCM(KEK_j, random_nonce, DEK, AAD=file_id)
5. Upload C to Pinata → CID
6. Register on-chain: (CID, SHA256(C), file_id, {W_j}, signature)
```

### 5.3 Session Protocol (Receiver → ESP32 → Blockchain)

```
PREREQUISITES:
- ESP32 generates session_nonce (32 bytes random)
- Contract has file registered with CID + content_commitment
- Receiver has PIN (low-entropy) + out-of-band knowledge

PROTOCOL:
1. Receiver connects to ESP32 AP → provisions WiFi → reconnects to hotspot
2. Receiver opens http://aether-node.local/ → gets dashboard
3. Receiver enters PIN → submits
4. ESP32 computes pin_hash = keccak256(PIN || session_nonce)
5. Browser → Alchemy → Contract.getFile(session_nonce, pin_hash)
   Contract verifies:
     - session_nonce exists, not expired, attempts < 3
     - pin_hash matches
     - Policy satisfied (time, count)
   Returns: (CID, encrypted_DEK_wrapper, signature, policy)
6. Browser → IPFS Gateway → fetches ciphertext C
7. Browser asks ESP32 (local HTTPS) to unwrap DEK:
     ESP32 verifies session_nonce still valid
     ESP32 uses ATECC608B to decrypt wrapper → DEK (future)
     Returns DEK (ephemeral, never leaves ESP32 RAM)
8. Browser decrypts: P = AES256-GCM-Decrypt(DEK, N, C, T, AAD)
9. Browser verifies SHA256(P) == on-chain content_commitment
10. Browser renders ephemerally (blob: URL, no download)
11. ESP32 increments attempt counter, expires session_nonce
```

---

## 6. HARDWARE SPECIFICATION (SINGLE ESP32)

### 6.1 Current BOM

| Qty | Component | Part Number | Purpose |
|-----|-----------|-------------|---------|
| 1 | ESP32 Dev Module | ESP32-WROVER-B / DevKitC V4 | Main MCU (4MB PSRAM) |
| 1 | Secure Element | ATECC608B-TNGTLS (SOIC-8) | **Future: Root of trust** |
| 1 | Button | 6×6mm tactile (momentary) | **Future: Human approval** |
| 3 | LED | 3mm (R/G/B) or RGB | Status indication |
| 1 | LDO | MCP1700-3302E | 3.3V regulator (if not onboard) |
| 4 | Capacitor | 0.1µF ceramic | Decoupling |
| 1 | Capacitor | 10µF electrolytic | Bulk decoupling |
| 2 | Resistor | 10kΩ | I2C pull-up (SDA/SCL) |
| 1 | Resistor | 10kΩ | Button pull-up |
| 1 | PCB | Custom / Perfboard | Integration |

### 6.2 Pinout (ESP32 DevKit V1)

| ESP32 Pin | Current Function | Future Function |
|-----------|------------------|-----------------|
| GPIO 21 | — | I2C SDA → ATECC608B |
| GPIO 22 | — | I2C SCL → ATECC608B |
| GPIO 4 | — | Button Input (approval) |
| GPIO 5 | Status LED | Green (authorized) |
| GPIO 18 | Status LED | Red (denied/error) |
| GPIO 19 | Status LED | Blue (session active) |
| 3V3 | Power | ATECC608B VCC |
| GND | Ground | ATECC608B GND |

### 6.3 ATECC608B Configuration (Future Provisioning)

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

---

## 7. NETWORK PROTOCOLS

### 7.1 ESP32 HTTPS Server (mbedTLS) — PLANNED

```
Server Certificate: Self-signed, pinned in frontend
  - Subject: CN=aether-node.local
  - SAN: DNS:aether-node.local, IP:10.21.3.249, IP:192.168.4.1
  - Key: ECDSA P-256 (generated on first boot, stored in ATECC608B)
  - Validity: 10 years (rotated via OTA)

Cipher Suites (Priority Order):
  1. TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
  2. TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256
  3. TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256

TLS Version: 1.2 minimum, 1.3 preferred

Client Verification: NONE (phone browser has no cert)
  - Instead: Certificate pinning in frontend (SHA-256 of server cert)

HSTS: max-age=31536000; includeSubDomains; preload
CSP: default-src 'none'; script-src 'self'; connect-src 'self' https://base-sepolia.g.alchemy.com https://gateway.pinata.cloud; img-src data: blob:; style-src 'self'
```

### 7.2 Current: HTTP + Multi-Gateway IPFS

| Layer | Current | Target |
|-------|---------|--------|
| ESP32 → Phone | HTTP (port 80) | HTTPS (port 443, mbedTLS) |
| Phone → Alchemy | HTTPS (ethers.js) | HTTPS (ethers.js) |
| Phone → IPFS | HTTPS (multi-gateway) | HTTPS (multi-gateway) |
| Certificate Pinning | ❌ None | ✅ SHA-256 of ESP32 cert |

---

## 8. FRONTEND SECURITY (BROWSER)

### 8.1 Content Security Policy (Target)

```http
Content-Security-Policy:
  default-src 'none';
  script-src 'self' 'wasm-unsafe-eval';
  connect-src 'self' https://base-sepolia.g.alchemy.com https://gateway.pinata.cloud https://dweb.link;
  img-src data: blob:;
  style-src 'self' 'unsafe-inline';
  font-src 'self';
  frame-ancestors 'none';
  base-uri 'none';
  form-action 'none';
  upgrade-insecure-requests;
  block-all-mixed-content;
```

### 8.2 Ephemeral Rendering (Zero-Disk)

```javascript
// For images
const img = new Image();
img.src = URL.createObjectURL(blob);
document.body.appendChild(img);
// On unload: URL.revokeObjectURL(img.src)

// For PDF (PDF.js)
const pdf = await pdfjsLib.getDocument({data: blob}).promise;
// Render to canvas, never create download link

// For text
const text = new TextDecoder().decode(await blob.arrayBuffer());
// Render in <pre>, no localStorage
```

### 8.3 Cache Control Headers (ESP32 Response)

```http
Cache-Control: no-store, no-cache, must-revalidate, max-age=0
Pragma: no-cache
Expires: 0
Clear-Site-Data: "cache", "cookies", "storage", "executionContexts"
```

---

## 9. ATTACK SURFACE & MITIGATIONS

| Attack Vector | MVP Status | Production Mitigation |
|---------------|------------|----------------------|
| **PIN brute-force (online)** | 3 attempts/session | Rate-limited by contract + session expiry |
| **PIN brute-force (offline)** | ❌ Public chain = trivial | Remove PIN; use Ed25519-signed auth token |
| **ESP32 cloning** | ❌ No protection | ATECC608B device identity + remote attestation |
| **Firmware modification** | ❌ No verification | Secure boot + attestation quote on session create |
| **Replay attack** | ❌ No nonce tracking | Monotonic counter in ATECC608B + session TTL |
| **IPFS gateway substitution** | ❌ Single gateway | Multi-gateway + SHA-256 verification |
| **Malicious ciphertext** | ❌ No integrity | AES-GCM tag + on-chain commitment |
| **Key extraction from phone** | ❌ DEK in browser RAM | DEK never leaves ESP32; decrypt in WebAssembly |
| **Traffic analysis** | ❌ Clear patterns | Cover traffic + constant-rate dummy requests |
| **Supply chain (ESP32 swap)** | ❌ No verification | Device certificate + attestation on each session |

---

## 10. DEVELOPMENT ROADMAP (PHASE-GATED, SINGLE ESP32)

### Phase 0: MVP Hardening (Week 1) — CURRENT
- [x] Design B firmware (hotspot provisioning)
- [x] Base Sepolia contract (getFile)
- [x] Host script (Pinata + register)
- [x] Frontend (multi-gateway, HTTP SHA-256 skip)
- [ ] **TLS on ESP32 (self-signed + pinning)**
- [ ] **CSP + cache headers**
- [ ] **Ephemeral render (no download)**

### Phase 1: Cryptographic Foundation (Week 2-3)
- [ ] ATECC608B driver + provisioning (I2C, slot config)
- [ ] Ed25519 signing (host) + verification (contract)
- [ ] AES-256-GCM encryption (host) + decryption (browser WASM)
- [ ] DEK wrapping per-device (ATECC608B unwrap)
- [ ] Double Ratchet session keys

### Phase 2: Hardware Gate (Week 4-5)
- [ ] Physical button approval flow (GPIO interrupt)
- [ ] Session nonce + monotonic counter (NVS)
- [ ] Remote attestation quote (firmware hash + counter)
- [ ] Policy engine on-chain (time, count, geo)
- [ ] ESP-NOW peer discovery (prep for 3-node future)

### Phase 3: Operational Security (Week 6-7)
- [ ] Cover traffic generator (dummy requests)
- [ ] Forward-secure session ratchet
- [ ] Revocation list (on-chain)
- [ ] Audit log (IPFS + Merkle root on-chain)
- [ ] Denial-of-retrieval proof (VDF)

### Phase 4: Red Team / Audit (Week 8)
- [ ] Side-channel analysis (power/EM on ESP32)
- [ ] Firmware extraction attempt
- [ ] Contract formal verification (Certora/Halmos)
- [ ] Penetration test (network, hardware, crypto)

---

## 11. FILE STRUCTURE (CURRENT + TARGET)

```
project-aether/
├── contracts/
│   ├── FileVerification.sol         # Current deployed contract
│   ├── AetherPolicy.sol             # Target policy contract
│   ├── test/
│   └── script/
│       ├── Deploy.s.sol
│       └── ProvisionGate.s.sol
│
├── host/
│   ├── host_transfer.py             # Current: upload + register
│   ├── aether_host/                 # Target: proper package
│   │   ├── crypto.py                # AES-GCM, Ed25519, HKDF
│   │   ├── ipfs.py                  # Pinata upload
│   │   ├── contract.py              # Contract interaction
│   │   └── cli.py                   # Typer CLI
│   ├── requirements.txt
│   └── .env.example
│
├── firmware/
│   ├── src/
│   │   ├── main.cpp                 # Entry point
│   │   ├── config.h                 # Build-time config
│   │   ├── network/
│   │   │   ├── wifi_manager.cpp     # AP/STA/Provisioning
│   │   │   ├── captive_portal.cpp   # DNS + HTTP redirect
│   │   │   ├── mdns_responder.cpp   # mDNS for .local
│   │   │   └── https_server.cpp     # mbedTLS (planned)
│   │   ├── session/
│   │   │   ├── session_manager.cpp  # Session state machine
│   │   │   └── pin_verifier.cpp     # keccak256 verification
│   │   ├── crypto/
│   │   │   └── atecc608b.cpp        # Secure element driver (planned)
│   │   ├── system/
│   │   │   ├── nvs_storage.cpp      # NVS for config
│   │   │   ├── ota_updater.cpp      # Signed OTA (planned)
│   │   │   └── watchdog.cpp         # Task health
│   │   └── ui/
│   │       └── dashboard_html.h     # Embedded dashboard
│   ├── include/
│   │   ├── index_html.h             # Current dashboard
│   │   ├── dashboard_html.h         # Target HTTPS dashboard
│   │   └── certs.h                  # Pinned certs (planned)
│   ├── platformio.ini
│   └── partitions.csv               # NVS, OTA, certs
│
├── frontend/
│   ├── src/
│   │   ├── index.html               # Entry (served by ESP32)
│   │   ├── crypto/
│   │   │   ├── aes-gcm.wasm         # WASM decrypt (planned)
│   │   │   ├── sha256.js            # Web Crypto API
│   │   │   └── ratchet.js           # Double Ratchet (planned)
│   │   ├── ui/
│   │   │   ├── dashboard.js
│   │   │   ├── renderer/
│   │   │   │   ├── image.js
│   │   │   │   ├── pdf.js
│   │   │   │   └── text.js
│   │   │   └── status.js
│   │   └── network/
│   │       ├── blockchain.js        # ethers.js wrapper
│   │       ├── ipfs.js              # Multi-gateway fetch
│   │       └── esp32.js             # Local HTTPS client
│   ├── package.json
│   └── vite.config.js
│
├── docs/
│   ├── SPEC.md                      # This file
│   ├── THREAT_MODEL.md
│   ├── CRYPTO.md
│   ├── HARDWARE.md
│   ├── DEPLOYMENT.md
│   ├── ONBOARDING.md
│   └── RED_TEAM.md
│
├── test/
│   ├── integration/
│   └── vectors/
│
├── .github/workflows/
│   ├── firmware.yml
│   ├── contracts.yml
│   ├── host.yml
│   └── frontend.yml
│
├── Makefile
├── README.md
└── LICENSE
```

---

## 12. EMBEDDED ENGINEERING DEPTH (WHY THIS IS AN EMBEDDED PROJECT)

### 12.1 WiFi Stack Management
- **Dual-mode operation**: AP for provisioning → STA for operation
- **State machine**: Clean transition AP → AP+STA → STA only
- **Captive portal**: DNS hijack (port 53) + HTTP redirect (port 80)
- **mDNS**: Service advertisement on STA interface post-provisioning
- **Power management**: WiFi off during idle, reconnection logic

### 12.2 Memory Management (ESP32-WROVER-B, 4MB PSRAM)
| Region | Size | Usage |
|--------|------|-------|
| IRAM | 128KB | ISR handlers, crypto primitives |
| DRAM | 320KB | FreeRTOS, mbedTLS, HTTP buffers |
| PSRAM | 4MB | HTTPS buffers, TLS session cache, file chunks |
| Flash | 4MB | Firmware (1.2MB), Certs (256KB), NVS (1MB), OTA (1.5MB) |

### 12.3 NVS (Non-Volatile Storage) Usage
```csv
# partitions.csv
nvs,      data, nvs,     0x9000,  0x4000,   # WiFi creds, device config
otadata,  data, ota,     0xD000,  0x2000,   # OTA metadata
factory,  app,  factory, 0x10000, 0x140000, # Firmware
ota_0,    app,  ota_0,   0x150000,0x140000, # OTA slot 0
ota_1,    app,  ota_1,   0x290000,0x140000, # OTA slot 1
certs,    data, fat,     0x3D0000,0x40000,  # 256KB for certs/keys
```

### 12.4 Real-Time Constraints
- **Session TTL**: 60 seconds (hard deadline)
- **PIN verification**: < 10ms on ESP32 (keccak256 in software)
- **HTTPS handshake**: < 2s (mbedTLS + ECDSA)
- **Watchdog**: Task health check every 1s, panic recovery

### 12.5 Secure Element Integration (ATECC608B) — Embedded Detail
```cpp
// atecc608b.cpp - I2C driver for ATECC608B
class Atecc608b {
    TwoWire& i2c;
    uint8_t addr = 0x60;
    
    // Wake sequence (requires 60ms low pulse)
    bool wake();
    
    // Sleep (low power)
    bool sleep();
    
    // Generate ECDSA P-256 keypair in slot
    bool genkey(uint8_t slot, uint8_t* pubkey_out);
    
    // Sign digest with private key in slot
    bool sign(uint8_t slot, const uint8_t* digest, uint8_t* sig_out);
    
    // AES-GCM encrypt/decrypt with key in slot
    bool aes_gcm_encrypt(uint8_t slot, const uint8_t* nonce, 
                         const uint8_t* aad, const uint8_t* plaintext,
                         size_t len, uint8_t* ciphertext, uint8_t* tag);
    bool aes_gcm_decrypt(uint8_t slot, const uint8_t* nonce,
                         const uint8_t* aad, const uint8_t* ciphertext,
                         size_t len, uint8_t* tag, uint8_t* plaintext_out);
    
    // Monotonic counter (anti-replay)
    bool counter_increment(uint8_t counter_id, uint32_t* value_out);
    bool counter_read(uint8_t counter_id, uint32_t* value_out);
    
    // Random number generation (TRNG)
    bool random(uint8_t* buf, size_t len);
};
```

---

## 13. DEPLOYMENT CHECKLIST (SINGLE ESP32)

### Pre-Demo
- [ ] ESP32 flashed with latest firmware (Design B)
- [ ] Serial monitor shows: `Aether-Setup` AP → STA connect → mDNS ready
- [ ] Contract deployed on Base Sepolia: `0xA3FFBC19730222a2C0a85e65791B890B4ecd6C26`
- [ ] Host script uploads test file → Pinata CID registered on-chain
- [ ] Phone hotspot: 2.4 GHz, WPA2, tested
- [ ] Pinata gateway works: `curl https://gateway.pinata.cloud/ipfs/<CID>`
- [ ] Alchemy RPC responding: `cast block-number --rpc-url $RPC`

### Demo Flow
1. **Provision**: Phone → `Aether-Setup` → enter hotspot creds → ESP32 joins
2. **Reconnect**: Phone → own hotspot → `http://aether-node.local/`
3. **Retrieve**: Enter PIN `4827` → file downloads → SHA-256 verified
4. **Attack demo**: Wrong PIN → rejected; Replay nonce → rejected; Tampered file → rejected

### Post-Demo
- [ ] Power off ESP32
- [ ] Collect serial logs
- [ ] Verify no sensitive data in logs

---

## 14. WHAT THIS PROJECT IS (AND ISN'T) — FINAL WORDS

### It Is
- A **reference implementation** of hardware-gated, cryptographically-verifiable file dissemination
- A **teaching tool** for: WiFi provisioning, captive portal, mDNS, blockchain anchoring, ephemeral rendering
- A **foundation** for: dead-drop systems, air-gap bridging, hardware-rooted access control

### It Isn't
- A turnkey "secure comms" product
- A replacement for Signal, Tor, or Tails
- Magic security dust you sprinkle on bad OPSEC
- A 2-of-3 threshold system (needs 3 ESP32s)
- Zero-forensic-footprint (impossible on standard mobile OS)

### The Hard Truth
> **Security is not a feature you add. It's a property you design for, verify continuously, and pay for in complexity, latency, and usability.**

Every line of this spec adds complexity. Every cryptographic primitive adds failure modes. Every hardware dependency adds supply chain risk.

**Build it because you understand the tradeoffs. Not because it sounds cool.**

---

## APPENDIX: QUICK REFERENCE

### Key Sizes & Algorithms
| Primitive | Algorithm | Size | Library |
|-----------|-----------|------|---------|
| Symmetric Encryption | AES-256-GCM | 256-bit key, 96-bit nonce, 128-bit tag | mbedTLS / Web Crypto |
| Alternative Symmetric | XChaCha20-Poly1305 | 256-bit key, 192-bit nonce | libsodium / Web Crypto |
| Key Derivation | HKDF-SHA256 | - | mbedTLS / Web Crypto |
| Signing | Ed25519 | 256-bit key, 512-bit sig | libsodium / noble-ed25519 |
| Alternative Signing | ECDSA P-256 | 256-bit key | mbedTLS / Web Crypto |
| Key Agreement | X25519 (ECDH) | 256-bit | libsodium / Web Crypto |
| Hash | SHA-256 | 256-bit | mbedTLS / Web Crypto |
| Hash (PIN) | keccak256 | 256-bit | ethers.js / Solidity |

### Network Endpoints
| Service | Endpoint | Fallback |
|---------|----------|----------|
| Base Sepolia RPC | `https://base-sepolia.g.alchemy.com/v2/...` | `https://sepolia.base.org` |
| IPFS Gateway | `https://gateway.pinata.cloud/ipfs/` | `https://dweb.link/ipfs/` |
| NTP (ESP32) | `pool.ntp.org` | `time.google.com` |

### Error Codes (Frontend)
| Code | Meaning | User Action |
|------|---------|-------------|
| `AUTH_INVALID_PIN` | PIN hash mismatch | Re-enter PIN |
| `AUTH_SESSION_EXPIRED` | Session TTL exceeded | Restart flow |
| `AUTH_ATTEMPTS_EXCEEDED` | >3 attempts | Wait for reset |
| `POLICY_TIME_INVALID` | Outside valid window | Contact sender |
| `POLICY_QUOTA_EXCEEDED` | Retrieval cap hit | Contact sender |
| `IPFS_FETCH_FAILED` | All gateways failed | Check network / retry |
| `INTEGRITY_FAILED` | SHA-256 mismatch | **ABORT — file tampered** |

---

**END OF SPECIFICATION**

*This document is a living specification. Update on every design change. Version control everything.*