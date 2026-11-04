# Project Aether — Team Onboarding Guide
**Version:** 1.0 | **Classification:** INTERNAL — FOR TEAM DISTRIBUTION

---

## 1. WHAT IS THIS PROJECT? (ELI5 — EXPLAIN LIKE I'M 5)

### The Analogy

Imagine you want to give a secret diary to a friend, but:

1. **You can't meet in person** — you're in different countries
2. **You can't trust the postal service** — they might open it
3. **You can't trust the internet** — hackers might intercept it
4. **Your friend's phone might be hacked** — malware could steal it
5. **You need PROOF they were physically at a specific location** — not just "they have the password"

**Aether solves this with three pieces:**

| Piece | What It Does | Analogy |
|-------|--------------|---------|
| **Tactical Node (ESP32)** | A tiny USB-C device your friend must **physically touch** to authorize retrieval | Like a bank vault that only opens when you're standing in front of it AND press the button |
| **Blockchain (Base Sepolia)** | An unchangeable public record saying "Diary #123 exists, its fingerprint is ABC, only Alice can open it, and only between 9-5 PM" | Like a notary public who writes everything in indelible ink that can never be erased |
| **IPFS (Pinata)** | The actual diary stored across thousands of computers, addressed by its fingerprint | Like a library where every book's location IS its content hash |

### The Flow (Simple)

```
HOST (Laptop)                          TACTICAL NODE (ESP32)           RECEIVER (Phone)
─────────────────────────────────────────────────────────────────────────────────────
1. Zip sensitive files                 │                              │
2. Encrypt (AES-256-GCM)               │                              │
3. Upload to Pinata IPFS → CID         │                              │
4. Register on Base Sepolia:           │                              │
   - CID + SHA-256 + filename          │                              │
   - Policy: time window, max 1 retrieve│                              │
                                        │                              │
                    ┌───────────────────▼───────────────────┐        │
                    │         NODE BOOTS                     │        │
                    │  1. WiFi AP: "Aether-Node"             │        │
                    │  2. Captive portal → dashboard         │        │
                    │  3. mDNS: aether-node.local             │        │
                    │  4. HTTPS server (port 443)             │        │
                    └───────────────────┬───────────────────┘        │
                                        │                            │
                              Receiver connects to AP               │
                                        │                            │
                              Opens https://aether-node.local/       │
                                        │                            │
                              Dashboard loads file list from        │
                              blockchain (via Alchemy)              │
                                        │                            │
                              Shows: filename, size, type           │
                                        │                            │
                              Click file → PREVIEW in browser       │
                              (image/PDF/text — no download)        │
                                        │                            │
                              Optional: "Download" → saves to phone │
                                        │                            │
                              Session ends → log:                   │
                              "2025-01-15 14:32: session_start"     │
                              "2025-01-15 14:35: session_end"       │
                              NO file metadata logged               │
```

---

## 2. KEY CONCEPTS (NO JARGON)

| Term | Simple Definition |
|------|-------------------|
| **ESP32 DevKit** | A ₹450 microcontroller board with WiFi, crypto acceleration, and HTTPS server capability |
| **Base Sepolia** | A test blockchain (free, fast, Ethereum-compatible) — the "public notary" |
| **Smart Contract** | Code on the blockchain that enforces rules automatically — the "rules that can't be changed" |
| **IPFS** | A decentralized file system — no central server — the "distributed library" |
| **Pinata** | A service that pins files to IPFS so they stay available — the "librarian" |
| **CID** | Content IDentifier — a unique fingerprint of a file (like SHA-256) — the "book's DNA" |
| **AES-256-GCM** | Encryption standard used by governments — the "unbreakable lock" |
| **Ed25519** | Digital signature algorithm (proves who sent it) — the "tamper-proof seal" |
| **mDNS** | Lets you use `aether-node.local` instead of IP addresses — the "name instead of number" |
| **HTTPS + Cert Pinning** | Encrypted connection + browser verifies it's YOUR ESP32 — prevents MITM |
| **SHA-256 on HTTPS only** | Browser allows integrity check ONLY on HTTPS — HTTP = skipped (AES-GCM still works) |

---

## 3. YOUR ROLE (3 PEOPLE MINIMUM)

### 🎯 Member A — Node Operator (Hardware + Firmware)
**Mission:** Make the Tactical Node work reliably over HTTPS.
**Deliverable (Week 1):** HTTPS firmware on DevKit, cert pinning verified on phone, button interrupt working.
**Skills:** Arduino/PlatformIO, mbedTLS, WiFi state machine, mDNS, cert generation.
**Docs:** `SPEC.md` §4, `firmware/src/`

### 🔐 Member B — Host Operator (Laptop)
**Mission:** End-to-end encryption and blockchain registration.
**Deliverable (Week 1):** `tactical_upload.py` working — encrypt → Pinata → Base Sepolia, master key managed.
**Skills:** Python (cryptography, requests, web3.py), AES-GCM, Ed25519, HKDF, Pinata API.
**Docs:** `CRYPTO.md`, `host/`

### 🌐 Member C — Receiver + Red Team (Phone)
**Mission:** Connect, preview, attack demos.
**Deliverable (Week 1):** Connect to Node AP → provision WiFi → preview files → run attack demos.
**Attack demos:** wrong PIN, replay nonce, tampered file, HTTP SHA-256 fail, no-button.
**Skills:** Chrome DevTools, network debugging, attack mindset.
**Docs:** `HONEST_CLAIMS.md`, `RED_TEAM.md`

---

## 4. FIRST WEEK CHECKLIST (DAY BY DAY)

### Day 1 (Monday): Hardware & Firmware
- [ ] **Member A:** Flash `tactical_node_https.ino` on DevKit → verify HTTPS + cert pinning on phone
- [ ] **Member B:** Python env → `cryptography` → test AES-GCM + Ed25519 vectors
- [ ] **Member C:** Test NodeMCU → **confirm it fails** (ESP8266 = trash) → put in drawer
- [ ] All: Git repo → README → issue templates

### Day 2 (Tuesday): Host Script & Contract
- [ ] **Member B:** `tactical_upload.py` → encrypt file → upload Pinata → register on Base Sepolia
- [ ] **Member B:** Deploy `FileVerification.sol` (already deployed at `0xA3FFBC19730222a2C0a85e65791B890B4ecd6C26`)
- [ ] **Member A:** Test HTTPS + cert pinning on phone browser
- [ ] **Member C:** Test multi-gateway IPFS fetch (Pinata gateway works on hostel WiFi)

### Day 3 (Wednesday): Integration
- [ ] **Member A:** Button interrupt → session authorization flag
- [ ] **Member B:** Host script: zip folder → encrypt → Pinata → contract register
- [ ] **Member C:** Integration test: Laptop → Contract → Node → Phone → Preview
- [ ] **Member C:** Attack demo: wrong PIN, replay nonce, tampered file, HTTP SHA-256 fail

### Day 4 (Thursday): Hardening
- [ ] **Member A:** Session TTL (60s), max attempts (3), air-gap on button hold
- [ ] **Member B:** Log sanitization — only "session_start", "session_end" in RAM
- [ ] **Member C:** Red Team Phase 0: wrong PIN, replay, tamper, MITM, HTTP SHA-256 fail

### Day 5 (Friday): DEMO DAY
- [ ] 10:00 AM: Final integration test
- [ ] 12:00 PM: Attack demo rehearsal
- [ ] 2:00 PM: Dry run with timer (5 min max)
- [ ] 4:00 PM: **LIVE DEMO** → Professor + class
- [ ] 5:00 PM: Retrospective → celebrate 🍻

---

## 5. COMMON MISTAKES TO AVOID

| Mistake | Why It's Bad | Fix |
|---------|--------------|-----|
| Committing `.env` with real keys | Anyone can steal funds/data | Use `.env.example`, never commit real |
| Using Mainnet for testing | Real money lost | **Always Base Sepolia** (chain ID 84532) |
| Flashing wrong firmware | Brick ESP32 / wrong behavior | Double-check `platformio.ini` board |
| Forgetting bootloader mode | Upload fails | Hold BOOT, press EN, release BOOT |
| Using 5 GHz hotspot | ESP32 can't connect | **Force 2.4 GHz** on phone |
| Skipping SHA-256 verify | Tampered file accepted | **Never skip integrity check** |
| Hardcoding contract address | Can't upgrade | Use env var / config file |
| Not zeroizing keys | Memory forensics recovers them | `volatile` in C, `fill(0)` in JS/WASM |
| Ignoring cert pinning | MITM on local WiFi | Pin SHA-256 of ESP32 cert in frontend |
| Claiming "zero footprint" | Professor will destroy you | Say "minimized persistent artifacts" |
| Using NodeMCU for demo | ESP8266 = no HTTPS, no crypto | **Put in drawer. It's trash for this.** |
| Trying host on ESP32 | No Python, no RAM, no FS | **Host = Laptop. Period.** |

---

## 6. DEBUGGING CHEAT SHEET

### ESP32 Won't Upload
```
1. Check COM port: pio device list
2. Bootloader mode: Hold BOOT → press EN → release BOOT
3. Close Serial Monitor (locks port)
4. Try: pio run -t upload -v
```

### Phone Can't Reach Dashboard
```
1. Phone on same WiFi as ESP32 STA IP?
2. Try IP instead of .local: https://10.21.3.249/
3. Check Serial Monitor for "STA IP: x.x.x.x"
4. Disable phone VPN/firewall
5. Check mDNS: `dns-sd -B _http._tcp local`
```

### IPFS Fetch Fails
```
1. Try gateway.pinata.cloud first (works on hostel WiFi)
2. Check Serial: "Trying gateway: ..."
3. Test CID in browser: https://gateway.pinata.cloud/ipfs/<CID>
4. Check CORS: gateway must allow Origin: https://aether-node.local
```

### SHA-256 Fails on HTTP
```
**THIS IS EXPECTED.** Browser blocks crypto.subtle.digest() on HTTP.
Fix: Use HTTPS on ESP32 (mbedTLS + self-signed cert + pinning).
On HTTP: AES-GCM tag still verified, SHA-256 skipped.
```

### Contract Call Fails
```
1. Check chain ID: 84532 (Base Sepolia)
2. Check contract address matches .env
3. Check ABI matches deployed contract
4. Use cast call to test: cast call <addr> "getFile(bytes32,bytes32)" <session> <pin>
```

---

## 7. LEARNING RESOURCES (CURATED)

| Topic | Resource | Time |
|-------|----------|------|
| ESP32 HTTPS (mbedTLS) | Espressif mbedTLS port guide | 3 hrs |
| ESP32 WiFi provisioning | Random Nerd Tutorials ESP32 | 2 hrs |
| Solidity + Foundry | Foundry Book (foundry-book.vercel.app) | 4 hrs |
| AES-GCM / HKDF | Crypto 101 (crypto101.io) | 2 hrs |
| Ed25519 / X25519 | RFC 8032 / RFC 7748 | 1 hr |
| Cert pinning in JS | MDN Certificate Pinning | 1 hr |
| Web Crypto API | MDN Web Crypto API | 1 hr |

---

## 8. COMMUNICATION

| Channel | Purpose |
|---------|---------|
| GitHub Issues | Bugs, tasks, design decisions |
| Discord/Slack | Daily sync, quick questions |
| Daily Sync | 10:00 AM — progress + blockers (15 min) |
| Demo Day | Friday 4:00 PM — show working features |

**Rule:** If blocked > 30 min → ask in Discord. If blocked > 2 hrs → escalate to daily sync.

---

## 9. MINDSET

> **"We don't build features. We reduce attack surface."**

Every line of code is a liability. Every dependency is a risk. Every assumption is a future bug.

**Ask before coding:**
1. What happens if this fails?
2. What happens if the adversary controls this?
3. Can we remove this entirely?

**Ship the simplest thing that provides the guarantee. Then audit it.**

---

## 10. CONTACTS

| Name | Role | Handle |
|------|------|--------|
| [Member A] | Node Operator | @node |
| [Member B] | Host Operator | @host |
| [Member C] | Receiver/Red Team | @redteam |

---

## 11. THE PITCH (MEMORIZE FOR PROFESSOR)

> **"We built a hardware-gated file dissemination system.  
>   
> The ESP32 is not a WiFi dongle — it's a proximity-enforcing HSM.  
> The blockchain is not a database — it's an immutable policy engine.  
> The browser is not an app — it's an ephemeral renderer with no persistent footprint.  
>   
> Here's what it guarantees: physical proximity required, file integrity verified, policy immutable.  
> Here's what it doesn't: zero forensic footprint (impossible), untraceable (carrier sees IP), unbreakable (quantum breaks Ed25519), SHA-256 on HTTP (browser blocks it — we use HTTPS).  
>   
> We're honest about the gaps. The architecture closes them in phases.  
>   
> **This is embedded systems meeting cryptography meeting distributed systems. That's the project."**

---

**WELCOME TO PROJECT AETHER. LET'S BUILD THE ONE THING THAT WORKS.**