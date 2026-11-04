# Project Aether — Threat Model & Security Analysis
**Version:** 1.0 | **Classification:** INTERNAL

---

## 1. STRIDE ANALYSIS

| Threat | Vector | Likelihood | Impact | Mitigation |
|--------|--------|------------|--------|------------|
| **Spoofing** | Fake ESP32 node | Medium | High | ATECC608B device identity + attestation |
| **Spoofing** | Fake dashboard (phishing) | Medium | High | Certificate pinning + HTTPS |
| **Tampering** | Firmware modification | Medium | Critical | Secure boot + remote attestation |
| **Tampering** | Ciphertext modification | High | Critical | AES-GCM tag + on-chain commitment |
| **Tampering** | Policy modification | Low | High | Immutable contract |
| **Repudiation** | Deny retrieval occurred | Medium | Medium | On-chain audit log (Merkle root) |
| **Info Disclosure** | PIN brute-force (offline) | **Critical** | High | Remove PIN; use signed auth tokens |
| **Info Disclosure** | DEK in browser memory | High | High | WASM decrypt + zeroize |
| **Info Disclosure** | Traffic analysis | High | Medium | Cover traffic + constant rate |
| **DoS** | ESP-NOW jamming | Medium | Medium | Channel diversity + retry |
| **DoS** | Contract gas griefing | Low | Medium | Gas limits + circuit breakers |
| **Elevation** | Privilege escalation via contract bug | Low | Critical | Formal verification + audit |

---

## 2. ATTACK TREES

### 2.1 Goal: Retrieve File Without Authorization

```
Retrieve File Without Authorization
├── Compromise ESP32 Hardware
│   ├── Extract ATECC608B keys (Side-channel)
│   │   ├── Power analysis (DPA/CPA)
│   │   ├── EM emanation
│   │   └── Glitching (voltage/clock)
│   ├── Replace ESP32 with malicious clone
│   │   ├── Supply chain intercept
│   │   └── Physical swap (evidence: attestation fails)
│   └── Firmware modification
│       ├── JTAG/SWD debug access
│       ├── OTA update compromise
│       └── Bootloader exploit
├── Compromise Session
│   ├── PIN brute-force (online)
│   │   └── Mitigated: 3 attempts + 5min TTL
│   ├── PIN brute-force (offline)
│   │   └── **CURRENT GAP**: Public chain = keccak256(PIN) visible
│   ├── Session hijack
│   │   ├── Replay old session_nonce
│   │   │   └── Mitigated: Monotonic counter + TTL
│   │   └── MITM on local WiFi
│   │       └── Mitigated: TLS + cert pinning
│   └── ESP-NOW injection
│       ├── Replay approval frame
│       │   └── Mitigated: Session-bound MAC
│       └── Forge approval (key extract)
│           └── Mitigated: ATECC608B stores ESP-NOW keys
├── Compromise Contract
│   ├── Reentrancy
│   │   └── Mitigated: Checks-effects-interactions
│   ├── Access control bypass
│   │   └── Mitigated: `onlyAdmin`, `onlyGate`
│   └── Policy logic error
│       └── Mitigated: Formal verification
├── Compromise IPFS
│   ├── Gateway serves wrong data
│   │   └── Mitigated: SHA-256 verification
│   └── Payload substitution (pre-image)
│       └── Mitigated: SHA-256 = 256-bit security
└── Compromise Receiver
    ├── Browser exploit
    │   └── Mitigated: Minimal JS, CSP, sandbox
    ├── Memory scrape (DEK)
    │   └── Mitigated: WASM + zeroize
    └── Forensic recovery
        └── Mitigated: Ephemeral render + no-store
```

### 2.2 Goal: Deny Legitimate Retrieval

```
Deny Legitimate Retrieval
├── Jam ESP-NOW channel
│   └── Mitigation: 3-channel rotation, directional antennas
├── DoS contract (gas grief)
│   └── Mitigation: Gas limits, circuit breaker
├── Pinata gateway down
│   └── Mitigation: Multi-gateway + local cache
├── ESP32 power loss
│   └── Mitigation: UPS / battery backup
├── Revocation abuse
│   └── Mitigation: Time-locked revocation + audit log
└── Legal/takedown
    └── Mitigation: Decentralized IPFS + contract immutability
```

---

## 3. TRUST BOUNDARIES

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         TRUST BOUNDARY MAP                               │
└─────────────────────────────────────────────────────────────────────────┘

    TRUSTED ZONE (Operator Controlled)
    ═══════════════════════════════════════
    ┌─────────────────────────────────────────────────────────────────┐
    │  HOST LAPTOP                                                    │
    │  • Master Key (MK) generation & storage                        │
    │  • File encryption & signing                                    │
    │  • Contract deployment & admin actions                          │
    │  • Provisioning ceremony (offline)                              │
    │  TRUST: FULL — Operator is the root of trust                   │
    └─────────────────────────────────────────────────────────────────┘
                              │
                              │ Provisioning (one-time, air-gapped)
                              ▼
    HARDWARE ROOT OF TRUST (Physical)
    ═══════════════════════════════════════
    ┌─────────────────────────────────────────────────────────────────┐
    │  ESP32 + ATECC608B (×3 nodes)                                   │
    │  • Device identity (ECDSA key in SE)                            │
    │  • Key wrap/unwrap (DEK never leaves SE)                        │
    │  • Monotonic counters (replay protection)                       │
    │  • Remote attestation (firmware hash)                           │
    │  • Physical button (human-in-the-loop)                          │
    │  TRUST: HIGH — Physical possession required                     │
    │  ASSUMPTION: SE not extractable; firmware not modified          │
    └─────────────────────────────────────────────────────────────────┘
                              │
                              │ Local WiFi + ESP-NOW (encrypted)
                              ▼
    UNTRUSTED ZONE (Adversary Controlled)
    ═══════════════════════════════════════
    ┌─────────────────────────────────────────────────────────────────┐
    │  RECEIVER PHONE / BROWSER                                       │
    │  • OS may be compromised (root/jailbreak)                       │
    │  • Browser may be malicious                                     │
    │  • Memory may be scraped                                        │
    │  • Network traffic observed                                     │
    │  • Forensic tools may run post-facto                            │
    │  TRUST: NONE — Assume fully compromised                         │
    │  GUARANTEE: File integrity verified; DEK never persists         │
    └─────────────────────────────────────────────────────────────────┘
                              │
                              │ HTTPS (Alchemy, Pinata)
                              ▼
    SEMI-TRUSTED INFRASTRUCTURE
    ═════════════════════════════════
    ┌─────────────────────────────────────────────────────────────────┐
    │  BASE SEPOLIA (L2)                                              │
    │  • Finality: ~2 min (L1 anchoring)                              │
    │  • Censorship resistance: High                                  │
    │  • TRUST: Finality + code immutability                          │
    │  RISK: Reorg (testnet), sequencer centralization                │
    └─────────────────────────────────────────────────────────────────┘
                              │
                              │ HTTPS
                              ▼
    ┌─────────────────────────────────────────────────────────────────┐
    │  PINATA / IPFS GATEWAYS                                         │
    │  • Confidentiality: NONE (encrypted payload)                    │
    │  • Integrity: SHA-256 verification                              │
    │  • Availability: Multi-gateway fallback                         │
    │  • TRUST: NONE for confidentiality; AVAILABILITY only           │
    └─────────────────────────────────────────────────────────────────┘
```

---

## 4. CRYPTOGRAPHIC ASSUMPTIONS

| Primitive | Assumption | Quantum Risk | Post-Quantum Alternative |
|-----------|------------|--------------|--------------------------|
| AES-256-GCM | IND-CCA2 | Grover: 128-bit → use 256 | AES-256 (same) |
| XChaCha20-Poly1305 | IND-CCA2 | Grover: 128-bit | Same |
| HKDF-SHA256 | PRF | Grover: 128-bit | SHA-384/512 |
| Ed25519 | EUF-CMA | Shor: **BROKEN** | Dilithium / Falcon |
| ECDSA P-256 | EUF-CMA | Shor: **BROKEN** | ECDSA over P-384 + hash |
| X25519 (ECDH) | CDH | Shor: **BROKEN** | Kyber / SIKE |
| SHA-256 | Collision/Preimage | Grover: 128-bit | SHA-384/512 |
| keccak256 (PIN) | Preimage | Grover: 128-bit | **Remove PIN** |

**Quantum Readiness:** Current design uses Ed25519/ECDSA — **not quantum-safe**. Migration path: hybrid signatures (Ed25519 + Dilithium) when standardized.

---

## 5. SIDE-CHANNEL ANALYSIS (ESP32 + ATECC608B)

| Channel | Risk | Countermeasure |
|---------|------|----------------|
| **Power Analysis** | ATECC608B designed for DPA resistance | Constant-time ops, noise injection |
| **EM Emanation** | ESP32 radio + MCU | Physical shielding, spread spectrum |
| **Timing** | AES-GCM, ECDSA in mbedTLS | Constant-time implementations |
| **Cache** | ESP32 no cache (Harvard arch) | N/A |
| **Branch Prediction** | ESP32 no branch predictor | N/A |
| **Glitching** | Voltage/clock fault injection | ATECC608B has glitch detectors |

**Residual Risk:** ATECC608B is FIPS 140-2 Level 3 / Common Criteria EAL4+. Not immune to nation-state lab attacks. Acceptable for this threat model.

---

## 6. FORENSIC RESISTANCE (PHONE)

| Artifact | Risk | Mitigation |
|----------|------|------------|
| Browser Cache | `Cache-Control: no-store` + `Clear-Site-Data` | Enforced by ESP32 headers |
| Downloads Folder | Ephemeral `blob:` URLs — no `download` attribute | Render only |
| LocalStorage/IndexedDB | Never used | CSP blocks |
| Service Workers | Not registered | CSP blocks |
| Memory (RAM) | DEK in WASM memory | Zeroize after decrypt |
| Swap/Pagefile | OS may swap browser memory | **Unavoidable on compromised OS** |
| Screenshots/Photos | User can photograph screen | **Unavoidable** |
| Network Logs | Carrier sees IP + TLS SNI | **Unavoidable** (use Tor if needed) |

**Honest Assessment:** "Zero forensic footprint" is a **lie**. Browser renders plaintext → photons hit retina → forensic recovery possible. We minimize *digital* artifacts; physical OPSEC remains user responsibility.

---

## 7. INCIDENT RESPONSE PROCEDURES

| Incident | Detection | Response |
|----------|-----------|----------|
| ESP32 attestation failure | Session create returns bad quote | **Immediate**: Revoke gate on-chain, rotate device |
| PIN brute-force detected | >3 failed attempts/session | Auto-expire session, alert operator |
| Contract upgrade needed | Vulnerability found | Proxy admin multisig (3-of-5) |
| DEK compromise | Forensic analysis post-incident | Rotate MK, re-encrypt all files, re-register |
| ESP-NOW key extract | Side-channel lab result | Rotate ESP-NOW keys via OTA (signed) |
| IPFS gateway censorship | Gateway returns 403/blocked | Add new gateway to allowlist |

---

## 8. RESIDUAL RISKS (ACCEPTED)

| Risk | Reason for Acceptance |
|------|----------------------|
| Carrier sees phone IP | Tor adds latency/complexity; out of scope |
| Screenshots defeat ephemeral render | Physical OPSEC is user discipline |
| OS swap/pagefile may leak DEK | Requires OS compromise + forensic skill |
| Quantum breaks Ed25519 | 10-15 year horizon; hybrid migration planned |
| Supply chain (ESP32/SE) | Provisioning ceremony mitigates; trust on first use |
| Pinata metadata (CID, size, timing) | Acceptable leakage; use cover traffic |

---

**END OF THREAT MODEL**