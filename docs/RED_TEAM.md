# Project Aether — Red Team Guide (Single Device)
**Version:** 1.0 | **Classification:** INTERNAL — RED TEAM ONLY

---

## 1. OBJECTIVE

**Find vulnerabilities before adversaries do.** This guide defines the scope, rules, and methodology for attacking Project Aether's single-device architecture at each phase.

---

## 2. RULES OF ENGAGEMENT

| Rule | Description |
|------|-------------|
| **Scope** | Only attack components you built or have explicit permission for |
| **No Production** | Never attack deployed contracts with real value |
| **No Physical Damage** | Don't brick hardware permanently |
| **Document Everything** | Every finding → GitHub Issue with `red-team` label |
| **Timebox** | Max 4 hours per attack vector per session |
| **Escalation** | Critical findings → immediate team notification |

---

## 3. ATTACK SURFACE MAP (SINGLE DEVICE)

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         ATTACK SURFACE MATRIX                            │
└─────────────────────────────────────────────────────────────────────────┘

LAYER 1: PHYSICAL (Single Node)
├── ESP32 UART/JTAG debug interface
├── Button/LED GPIO
├── Power supply glitching
├── Firmware extraction (SPI flash)
├── Side-channel (power/EM during crypto)
└── Supply chain (device swap)

LAYER 2: NETWORK (LOCAL)
├── ESP32 AP (Aether-Setup) — open WiFi
├── ESP32 STA (phone hotspot client)
├── ESP32 HTTPS server (self-signed cert)
├── mDNS / DNS-SD (aether-node.local)
├── Captive portal HTTP redirect
└── USB CDC (when plugged in)

LAYER 3: NETWORK (INTERNET)
├── Alchemy RPC (Base Sepolia)
├── Pinata API / IPFS Gateways
├── NTP time sync
└── OTA update endpoint

LAYER 4: SMART CONTRACT
├── FileVerification.sol
├── Admin functions
├── Gate authorization
├── Session management
└── Policy evaluation

LAYER 5: HOST SCRIPT
├── Master key handling
├── Encryption implementation
├── Pinata upload
├── Contract registration
└── CLI argument parsing

LAYER 6: FRONTEND
├── Dashboard HTML/JS
├── ethers.js integration
├── Web Crypto API usage
├── Certificate pinning
└── CSP headers
```

---

## 4. ATTACK SCENARIOS BY PHASE

### Phase 0 (MVP) — Current State

| ID | Attack | Method | Success Criteria | Mitigation Status |
|----|--------|--------|------------------|-------------------|
| R0-01 | **PIN Brute-force** | Query contract with all 10,000 PINs | Contract returns CID for valid PIN | ❌ **VULNERABLE** — PIN = keccak256(PIN\|nonce), nonce public |
| R0-02 | **IPFS Gateway MITM** | ARP spoof + fake gateway | Browser downloads wrong file | ⚠️ **PARTIAL** — SHA-256 verify catches, but TLS MITM possible |
| R0-03 | **ESP32 AP Evil Twin** | Stronger signal fake AP | Phone connects to attacker AP | ❌ **VULNERABLE** — No cert pinning on AP |
| R0-04 | **Contract Replay** | Reuse old session_id + PIN | Contract accepts replayed session | ⚠️ **PARTIAL** — 60s TTL + 3 attempts, but no monotonic counter |
| R0-05 | **Firmware Downgrade** | Flash old firmware via UART | Old vulns exploitable | ❌ **VULNERABLE** — No secure boot |
| R0-06 | **Side-channel PIN** | Power analysis on ESP32 during hash | Recover PIN from power trace | ❌ **VULNERABLE** — No countermeasures |
| R0-07 | **MITM Local WiFi** | ARP spoof + fake dashboard | Steal PIN, serve malware | ❌ **VULNERABLE** — HTTP only, no cert pinning |
| R0-08 | **Session Hijack** | Steal session_nonce + PIN | Full file access | ❌ **VULNERABLE** — No mutual auth |
| R0-09 | **Button Bypass** | GPIO short / EM pulse | Session authorized without press | ⚠️ **MVP: NO BUTTON** — Not implemented yet |
| R0-10 | **Supply Chain Swap** | Replace Node with malicious clone | All sessions compromised | ❌ **VULNERABLE** — No attestation |

### Phase 1 (Crypto Foundation)

| ID | Attack | Method | Success Criteria | Target Fix |
|----|--------|--------|------------------|------------|
| R1-01 | **DEK Extraction (Browser)** | Memory dump during WASM decrypt | Recover DEK from heap | WASM + zeroize |
| R1-02 | **KEK Extraction (ESP32)** | DPA on ATECC608B Slot 4 | Recover KEK | SE designed for DPA resistance |
| R1-03 | **Signature Forgery** | Fault injection on Ed25519 | Valid sig without private key | SE hardware protection |
| R1-04 | **HKDF Key Confusion** | Cross-protocol key reuse | Derive MK from DEK | Domain separation via `info` |
| R1-05 | **Nonce Reuse** | Force AES-GCM nonce reuse | Decrypt without key | Deterministic nonce from counter |

### Phase 2 (Hardware Gate)

| ID | Attack | Method | Success Criteria | Target Fix |
|----|--------|--------|------------------|------------|
| R2-01 | **Button Bypass** | GPIO short / EM pulse | Session authorized without press | Hardware debounce + SE confirmation |
| R2-02 | **Attestation Forgery** | Extract attestation key | Fake quote with bad firmware | SE slot 2 never readable |
| R2-03 | **Monotonic Counter Reset** | Power glitch during increment | Replay old session | SE hardware counter |
| R2-04 | **Secure Boot Bypass** | Glitch during boot | Unsigned firmware runs | ESP32-S3 Secure Boot v2 |
| R2-05 | **ESP-NOW Injection** | Forge frame with stolen key | Node accepts fake approval | SE-stored ESP-NOW keys (future) |

---

## 5. SPECIFIC TEST CASES (MVP PRIORITY)

### 6.1 Contract: PIN Brute-Force (R0-01) — CRITICAL

```solidity
// Vulnerability: PIN = 4 digits = 10,000 possibilities
// Attacker knows: file exists, has CID, session_nonce is public
// pinHash = keccak256(PIN || session_nonce)
// Attacker can precompute all 10,000 pinHashes offline
// Then call getFile(sessionId, pinHash) for each until one succeeds

// Expected: No rate limiting per file in MVP
// Fix Phase 1: Threshold approval required before getFile returns data
// Fix Phase 1: Remove PIN; use Ed25519-signed auth token
```

**Test:**
```bash
# Precompute all pinHashes for known session_nonce
for pin in {0000..9999}; do
    pinHash=$(cast keccak $(printf "%s%s" "$pin" "$session_nonce"))
    result=$(cast call <CONTRACT> "getFile(bytes32,bytes32)" <SESSION_ID> $pinHash)
    if [ "$result" != "0x" ]; then
        echo "FOUND PIN: $pin"
        break
    fi
done
```

### 6.2 ESP32: Evil Twin AP (R0-03)

```c
// Attack: Attacker sets up "Aether-Setup" with stronger signal
// Phone connects to attacker AP instead of real ESP32
// Attacker serves malicious dashboard (steals PIN, serves malware)

// Mitigation:
// 1. Certificate pinning (frontend knows ESP32 cert hash)
// 2. ESP32 AP uses WPA2 (not open) — but breaks captive portal
// 3. User education: verify cert fingerprint
```

### 6.3 Frontend: Certificate Pinning Bypass

```javascript
// Attack: MITM on local network presents fake cert
// If pinning is weak (only checks SPKI), attacker uses same CA

// Test:
// 1. Generate fake cert for aether-node.local
// 2. Serve dashboard with fake cert
// 3. Check if browser rejects (pinning works) or accepts (fail)

// Expected: Browser rejects — pinning checks full cert chain hash
```

### 6.4 Physical: Button Bypass (R0-09)

```c
// Attack: GPIO short / EM pulse triggers button interrupt
// No physical press, but session authorized

// Test:
// 1. Short GPIO 4 to GND with wire
// 2. EM pulse near GPIO trace
// 3. Power glitch during button read

// Mitigation:
// 1. Hardware debounce (RC filter + Schmitt trigger)
// 2. SE confirms button press (future: SE GPIO)
// 3. Require press duration > 500ms
```

### 6.5 Hardware: NodeMCU (ESP8266) — **ATTACK VECTOR**

```
┌─────────────────────────────────────────────────────────────────┐
│                    NODEMCU = CRITICAL RISK                       │
└─────────────────────────────────────────────────────────────────┘

| Property | NodeMCU (ESP8266) | ESP32 DevKit |
|----------|-------------------|--------------|
| HTTPS Server | ❌ No | ✅ Yes (mbedTLS) |
| Crypto Acceleration | ❌ None | ✅ AES/SHA/ECC HW |
| RAM | 80KB | 520KB |
| Cert Pinning | ❌ Impossible | ✅ Possible |
| Cert Generation | ❌ No crypto | ✅ Yes |
| MITM Resistance | ❌ None | ✅ With pinning |

**If NodeMCU is anywhere near the demo, it's an attack vector. Remove it.**
```

---

## 6. TESTING METHODOLOGY

### 5.1 Automated Fuzzing

```bash
# Contract fuzzing (Foundry)
cd contracts
forge test --fuzz-runs 10000

# Host script fuzzing (Python)
cd host
python -m pytest tests/fuzz/ -xvs

# Frontend fuzzing (JS)
cd frontend
pnpm test:fuzz
```

### 5.2 Property-Based Testing

```python
# Example: PIN brute-force resistance
@given(st.integers(min_value=0, max_value=9999))
def test_pin_not_brute_forceable(pin):
    # Contract should rate-limit or require threshold
    assert not contract.can_brute_force(pin)
```

### 5.3 Side-Channel Testing (Lab)

| Equipment | Target | Measurement |
|-----------|--------|-------------|
| ChipWhisperer | ESP32 | Power traces during AES/SHA |
| Langer EMV-RS | ESP32 | EM emanation during crypto |
| NewAE CW308 | ESP32 | Voltage/clock glitching |

---

## 7. REPORTING TEMPLATE

```markdown
## Red Team Finding: [ID]

**Severity:** Critical / High / Medium / Low
**Component:** [Contract / ESP32 / Frontend / Host / Network / Physical]
**Phase:** [0 / 1 / 2]

### Description
[Clear description of the vulnerability]

### Attack Vector
[Step-by-step reproduction]

### Impact
[What an attacker gains: file access, key extraction, DoS, etc.]

### Proof of Concept
[Code, commands, or hardware setup]

### Root Cause
[Why the vulnerability exists]

### Recommended Fix
[Specific code/config change]

### Verification
[How to verify fix works]

### References
[CVE, CWE, related issues]
```

---

## 8. PRIORITIZATION MATRIX

| Severity | Criteria | Response Time |
|----------|----------|---------------|
| **Critical** | Full file access, key extraction, RCE | Immediate (halt demo) |
| **High** | Partial access, DoS, privilege escalation | 24 hours |
| **Medium** | Info disclosure, logic bypass | 1 week |
| **Low** | Minor info leak, UX issue | Next sprint |

---

## 9. CONTINUOUS RED TEAMING

### Daily (Automated)
- [ ] Contract fuzzing (10k runs)
- [ ] Dependency audit (`cargo audit`, `npm audit`, `pip-audit`)
- [ ] Static analysis (Slither, Semgrep, CodeQL)

### Per Sprint (Manual)
- [ ] Side-channel testing (if lab available)
- [ ] Physical attack simulation (button, glitching, evil twin)
- [ ] Supply chain review (BOM, firmware signatures)

### Per Release
- [ ] Full attack surface review
- [ ] Threat model update
- [ ] Red team retrospective

---

## 10. BLUE TEAM COORDINATION (FIX TRACKING)

| Red Finding | Blue Response | Phase |
|-------------|---------------|-------|
| PIN brute-force (R0-01) | Add threshold requirement before `getFile`; remove PIN | 1 |
| Evil twin AP (R0-03) | Cert pinning on frontend; WPA2 on AP (if possible) | 0 |
| MITM local WiFi (R0-07) | HTTPS + cert pinning on ESP32 | 0 |
| Firmware downgrade (R0-05) | ESP32-S3 Secure Boot v2 | 1 |
| ESP32 cloning (R0-10) | ATECC608B device identity + attestation | 1 |
| Button bypass (R0-09) | Hardware debounce + SE confirmation | 2 |
| Side-channel (R0-06) | SE designed for DPA; add noise | 1 |
| NodeMCU near demo (R0-11) | **Remove it. It's an attack vector.** | 0 |

---

## 11. LEGAL & ETHICAL

- **All testing on testnet only** — no mainnet value at risk
- **No testing on third-party infrastructure** (Alchemy, Pinata) without permission
- **Physical attacks only on owned hardware** — no social engineering
- **Findings disclosed to team first** — 90-day responsible disclosure

---

## 12. MVP DEMO ATTACK SCRIPT (FOR PROFESSOR)

```bash
# Live demo attacks (each < 30s)

# 1. Wrong PIN
echo "Attack: Wrong PIN"
# Enter 1234 instead of 4827
# Expected: RED + REJECT

# 2. Replay old session
echo "Attack: Replay session nonce"
# Capture session_nonce from previous run
# Reuse with correct PIN
# Expected: REJECT (TTL expired / attempts exceeded)

# 3. Tampered file
echo "Attack: Tampered file"
# Re-upload modified file → new CID
# Use old session + PIN
# Expected: SHA-256 mismatch → ABORT

# 4. No button press (Phase 1)
echo "Attack: No physical approval"
# Enter correct PIN, don't press button
# Expected: Session expires → REJECT

# 5. Direct contract access (no ESP32)
echo "Attack: Bypass hardware gate"
# Call contract.getFile() directly with correct PIN
# Expected: Works BUT no proximity gate
# Point: "ESP32 adds physical requirement"

# 6. HTTP SHA-256 fail
echo "Attack: HTTP downgrade"
# Force HTTP connection (no HTTPS)
# Expected: SHA-256 skipped, AES-GCM tag still verified
# Point: "HTTPS mandatory for full integrity"
```

---

## 13. PHYSICAL SECURITY CHECKLIST (PRE-DEMO)

- [ ] **NodeMCU removed from demo area** (drawer/box)
- [ ] Only ESP32 DevKit visible
- [ ] USB cable is charge-only or data (whichever demo needs)
- [ ] No debug probes connected
- [ ] Firmware hash recorded and verified
- [ ] Cert SHA-256 pinned in frontend matches ESP32 cert

---

**REMEMBER: The goal is not to break it. The goal is to prove it can't be broken — or fix it until it can't.**

---

**END OF RED TEAM GUIDE**