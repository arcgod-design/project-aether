# Architecture Overview

> **Project Aether** — Hardware-gated, blockchain-anchored, IPFS-backed file dissemination system.

---

## System Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           PROJECT AETHER — SYSTEM OVERVIEW                   │
└─────────────────────────────────────────────────────────────────────────────┘

     ┌─────────────┐                    ┌─────────────────┐     ┌─────────────────┐
     │   HOST      │                   │    IPFS         │     │    BLOCKCHAIN   │
     │  (Python)   │────── upload ────▶│    (Pinata)     │     │  (Base Sepolia) │
     │             │                   │                 │     │                 │
     │ Encrypt +   │                   │  Store CID +    │     │  Register CID   │
     │ Sign        │                   │  metadata       │     │  + SHA-256      │
     └──────┬──────┘                   └────────┬────────┘     └────────┬────────┘
            │                                  │                       │
            │              ┌───────────────────┼───────────────────────┘
            │              │                   │
            ▼            ▼                   ▼
    ┌─────────────────────────────────────────────────────────────────────────┐
    │                         RECEIVER (Phone Browser)                         │
    │                                                                          │
    │  1. Connect to ESP32 AP (Aether-Setup)                                   │
    │  2. Provision WiFi → ESP32 joins phone hotspot                           │
    │  3. Open http://aether-node.local/                                       │
    │  4. Enter PIN → Frontend computes keccak256(PIN)                         │
    │  5. Query contract.getFile(keccak256(PIN)) → Get CID + SHA-256           │
    │  6. Fetch from IPFS gateways → Verify SHA-256 (pure-JS)                  │
     │  7. Preview/Download                                                   │
     └─────────────────────────────────────────────────────────────────────────┘
```

---

## Core Components

### 1. Host (Python) — `host/`
- **Purpose**: Encrypt, sign, upload to IPFS, register on-chain
- **Key files**:
  - `host_transfer.py` — CLI: encrypt → Pinata → register on-chain
  - `requirements.txt` — Python deps
  - `.env` — Secrets (gitignored)

### 2. Firmware — ESP32 (PlatformIO)
```
firmware/
├── src/
│   ├── main.cpp              # Entry point, WiFi, session, API
│   ├── config.h              # Build-time config
│   ├── network/              # WiFi, captive portal, mDNS
│   ├── session/              # Session management
│   └── ui/                   # Embedded HTML pages (PROGMEM)
├── include/
│   ├── setup_page.h          # Provisioning page
│   ├── login_page.h          # PIN entry + preview (NEW: single-file flow)
│   └── pages.h               # Success/error pages
├── platformio.ini
└── partitions.csv
```

### 3. Smart Contracts (Solidity/Foundry)
```
contracts/
├── FileVerification.sol      # Current: PIN → CID + SHA-256
├── AetherPolicy.sol          # Target: policy engine (time/geo/count)
├── script/
│   ├── Deploy.s.sol
│   └── ProvisionGates.s.sol
└── test/
```

### 4. Frontend (Embedded in Firmware)
- Single HTML page served by ESP32 (`login_page.h`)
- CDN ethers.js + pure-JS SHA-256
- Multi-gateway IPFS fetch with fallback
- Preview (PDF/image/text) + Download button

### 5. Smart Contract (Base Sepolia)
```solidity
contract FileVerification {
    struct FileRecord {
        string cid;
        bytes32 sha256Digest;
        string fileName;
        uint256 fileSize;
        uint256 createdAt;
        bool exists;
    }
    
    mapping(bytes32 => FileRecord) private records;  // key = keccak256(PIN)
    
    function registerFile(bytes32 pinHash, string cid, bytes32 sha256, string name, uint256 size) external onlyAdmin;
    function getFile(bytes32 pinHash) external view returns (cid, sha256, name, size, timestamp, exists);
}
```

---

## Data Flow

```
HOST                          ESP32                         PHONE
  │                              │                              │
  ├─▶ Encrypt (AES-256-GCM)     │                              │
  ├─▶ Upload to Pinata          │                              │
  ├─▶ Register on Base Sepolia  │                              │
  │                              │                              │
  │                              ◀─── AP: Aether-Setup         │
  │                              │                              │
  │                              ◀─── WiFi creds               │
  │                              │                              │
  │                              ├─▶ STA connect to hotspot    │
  │                              │                              │
  │                              ├─▶ mDNS: aether-node.local    │
  │                              │                              │
  │                              │                         ◀─── HTTP login page
  │                              │                              │
  │                              │                         ──▶ PIN + session
  │                              │                              │
  │                              ├─▶ /api/session (create)     │
  │                              │                              │
  │                              ├─▶ blockchain getFile(pinHash)│
  │                              │                              │
  │                              │                         ──▶ CID + SHA-256
  │                              │                              │
  │                              │                         ◀─── IPFS fetch (multi-gateway)
  │                              │                              │
  │                              │                         ──▶ SHA-256 verify (pure-JS)
  │                              │                              │
  │                              │                         ──▶ Preview / Download
```

---

## Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| **ESP32 + Arduino** | Mature ecosystem, PSRAM for TLS buffers, native USB CDC |
| **Base Sepolia** | Free testnet, L2 finality, EVM compatible |
| **Pinata IPFS** | Reliable gateway, free tier generous |
| **keccak256(PIN)** | Low-entropy PIN → 256-bit domain, matches host registration |
| **Pure-JS SHA-256** | Works on HTTP (no crypto.subtle restriction) |
| **mDNS `.local`** | Zero-config discovery on phone hotspot |
| **HTTP not HTTPS** | MVP simplicity; TLS is Phase 1 |
| **No persistent app** | Browser-only = no forensic app artifacts |

---

## Security Boundaries

```
┌─────────────────────────────────────────────────────────────┐
│                    TRUST BOUNDARIES                          │
├─────────────────────────────────────────────────────────────┤
│  TRUSTED (Operator)                                          │
│  ├─ Host laptop — master key, encryption, registration      │
│  └─ Provisioning station — ATECC608B programming (future)   │
├─────────────────────────────────────────────────────────────┤
│  HARDWARE ROOT OF TRUST (Physical)                          │
│  ├─ ESP32 + ATECC608B (future) — key storage, attestation  │
│  ├─ Physical button — human-in-the-loop approval            │
│  └─ Monotonic counter (NVS) — replay protection             │
├─────────────────────────────────────────────────────────────┤
│  UNTRUSTED (Adversary Controlled)                           │
│  ├─ Phone OS — root/jailbreak, forensics (Cellebrite)       │
│  ├─ Browser — memory, cache, screenshots                    │
│  ├─ Network — WiFi/cellular MITM, traffic analysis          │
│  ├─ IPFS Gateways — Pinata/Cloudflare could lie             │
│  └─ Blockchain — reorg risk (testnet)                       │
└─────────────────────────────────────────────────────────────┘
```

---

## Threat Model Summary

| Threat | Mitigation |
|--------|------------|
| PIN brute-force (online) | 3 attempts/session, 60s TTL |
| PIN brute-force (offline) | Public chain = trivial; **Phase 1: remove PIN, use Ed25519 tokens** |
| ESP32 cloning | ATECC608B device identity + remote attestation (Phase 1) |
| Firmware modification | Secure boot + attestation quote (Phase 1) |
| Replay attack | Session nonce + TTL + monotonic counter (Phase 1) |
| MITM local WiFi | TLS + cert pinning (Phase 0) |
| Malicious IPFS gateway | Multi-gateway + SHA-256 verification |
| Key extraction from phone | DEK never leaves ESP32 (Phase 1: ATECC608B unwrap) |
| Supply chain swap | Device cert + attestation on each session |

---

## Current MVP Gaps (Honest)

| Component | Status | Gap |
|-----------|--------|-----|
| Transport | HTTP | No TLS, no cert pinning |
| Secure Element | None | ATECC608B driver pending |
| Physical Button | GPIO 4 | Not wired on demo board |
| PIN Entropy | 4-digit | 10k guesses; offline brute-forceable |
| HTTPS | None | mbedTLS integration pending |
| Secure Element | None | ATECC608B driver pending |

---

## File Locations Quick Reference

| Component | Path |
|-----------|------|
| Firmware entry | `firmware/src/main.cpp` |
| WiFi provisioning | `firmware/src/network/` |
| Session logic | `firmware/src/session/` |
| Embedded HTML | `firmware/include/*.h` |
| Smart contract | `contracts/FileVerification.sol` |
| Host upload | `host/host_transfer.py` |
| Login page | `firmware/include/login_page.h` |
| Setup page | `firmware/include/setup_page.h` |
| Docs | `docs/*.md` |

---

**Next: [Deployment Guide](DEPLOYMENT.md) | [Threat Model](THREAT_MODEL.md) | [Crypto Spec](CRYPTO.md) | [Hardware Guide](HARDWARE.md)**