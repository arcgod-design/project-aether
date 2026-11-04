# Project Aether — Tactical File Dissemination System

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Framework: Arduino](https://img.shields.io/badge/Framework-Arduino-00979D.svg)](https://github.com/espressif/arduino-esp32)
[![Smart Contract: Base Sepolia](https://img.shields.io/badge/Contract-Base%20Sepolia-0052FF.svg)](https://sepolia.basescan.org)
[![IPFS: Pinata](https://img.shields.io/badge/IPFS-Pinata-FF6B6B.svg)](https://pinata.cloud)

> **A hardware-gated, blockchain-anchored, IPFS-backed file dissemination system.**  
> Physical proximity + blockchain verification + IPFS storage = tamper-evident file delivery.

---

## 🎯 What Is This?

Project Aether is a **hardware-gated file dissemination system** built on three pillars:

| Pillar | Technology | Purpose |
|--------|------------|---------|
| **Hardware Gate** | ESP32 (Arduino) | Physical proximity enforcement via captive portal + physical button |
| **Verification Layer** | Base Sepolia (L2) | Immutable PIN→CID mapping, tamper-evident file registry |
| **Storage** | IPFS (Pinata) | Decentralized, content-addressed file storage |

**Flow:** `Host uploads file → Pinata IPFS → Base Sepolia registers CID+PIN → Receiver enters PIN on ESP32 → File retrieved & verified`

---

## 🚀 Quick Start

### Hardware Required
| Qty | Component | Notes |
|------|-----------|-------|
| 1 | ESP32 DevKit (WROVER-B / DevKitC V4) | 4MB PSRAM recommended |
| 1 | ATECC608B (optional, future) | Secure element for key isolation |
| 1 | Tactile button (6×6mm) | Physical approval (GPIO 4) |
| 3 | LEDs (R/G/B or RGB) | Status indication |
| 1 | USB-C cable | Power + serial |

### Prerequisites
- [PlatformIO](https://platformio.org/) or Arduino IDE
- [Python 3.11+](https://python.org/) for host script
- [Node.js 20+](https://nodejs.org/) for frontend (optional)
- [Foundry](https://getfoundry.sh/) for contract deployment
- MetaMask + Base Sepolia testnet ETH (from [faucet](https://faucet.quicknode.com/base/sepolia))

### 1. Deploy Smart Contract
```bash
cd contracts
forge script Deploy --rpc-url $BASE_SEPOLIA_RPC --broadcast --verify
# Save deployed contract address
```

### 2. Configure Environment
```bash
cp .env.example host/.env
# Edit host/.env with your values:
# PINATA_JWT, BASE_SEPOLIA_RPC_URL, CONTRACT_ADDRESS, HOST_WALLET_ADDRESS, HOST_PRIVATE_KEY
```

### 3. Register Files (Host)
```bash
cd host
pip install -r requirements.txt
python host_transfer.py ./briefing.pdf      # PIN: 1111
python host_transfer.py ./map.jpg           # PIN 2222
python host_transfer.py ./report.pdf        # PIN 3333
# ... repeat for each file with unique PIN
```

### 4. Flash Firmware
```bash
cd firmware
pio run -t upload
# Hold BOOT, tap EN, release BOOT for bootloader mode
```

### 5. Test Retrieval
1. Phone → WiFi `Aether-Setup` / `aether1234`
2. Browser → `http://192.168.4.1` → Enter your phone hotspot credentials
3. Phone → reconnect to your hotspot → `http://aether-node.local/`
4. Enter PIN → **Preview renders inline** → Download optional

---

## 🏗 Architecture

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   HOST      │     │   IPFS      │     │  BASE       │     │   PHONE     │
│  (Laptop)   │────▶│  (Pinata)   │     │  SEPOLIA    │     │  (Browser)  │
└──────┬──────┘     └──────┬──────┘     └──────┬──────┘     └──────┬──────┘
       │                   │                    │                  │
       ▼                   ▼                    ▼                  ▼
  Encrypt + Sign      Store CID              Register CID         Preview/
  Upload to IPFS      + metadata             + SHA-256            Download
```

### Data Flow
1. **Host**: Encrypt file (AES-256-GCM) → Upload to Pinata → Get CID → Register `CID + SHA-256 + PIN` on Base Sepolia
2. **ESP32**: Boots AP → Phone provisions WiFi → ESP32 joins phone hotspot → mDNS `aether-node.local`
3. **Phone**: Enters PIN → Frontend computes `keccak256(PIN)` → Queries contract → Gets CID
4. **Retrieval**: Fetch from IPFS gateways → SHA-256 verify against on-chain hash → Preview/Download

---

## 🔧 Hardware

### Supported Boards (Branch per Board)
| Board | Branch | Status | Notes |
|-------|--------|--------|-------|
| ESP32 DevKit (WROOM-32) | `board/esp32-devkit` | ✅ Primary | 520KB RAM, no PSRAM |
| ESP32-S3 DevKitC-1 | `board/esp32s3-devkit` | 🟡 WIP | 512KB SRAM + 8MB PSRAM |
| ESP32-C3 DevKit | `board/esp32c3-devkit` | ⏳ Planned | RISC-V, no BT |
| ESP32-S2 | `board/esp32s2-devkit` | ⏳ Planned | No BT, native USB |

> **Create a new board branch:** `git checkout -b board/<board-name>` — see [BOARDS.md](docs/BOARDS.md)

### Pinout (ESP32 DevKit V1)
| GPIO | Function | Notes |
|------|----------|-------|
| 4 | Button (INPUT_PULLUP) | Physical approval |
| 5 | Green LED | Authorized |
| 18 | Red LED | Error/Denied |
| 19 | Blue LED | Session active |
| 21 | I2C SDA | ATECC608B SDA |
| 22 | I2C SCL | ATECC608B SCL |

---

## 🔐 Security Model

### What We Guarantee (Post-Production)
| Guarantee | Mechanism |
|-----------|-----------|
| Physical proximity required | ESP32 session nonce + hardware button |
| File integrity | AES-GCM tag + on-chain SHA-256 commitment |
| Immutable policy | Base L2 smart contract |
| Forward secrecy | Double Ratchet per session |
| Node compromise detection | Remote attestation via ATECC608B |

### What We DON'T Claim
| Claim | Reality |
|-------|---------|
| "Untraceable" | Carrier sees phone IP |
| "Zero forensic footprint" | Browser caches, swap, screenshots exist |
| "Unbreakable" | 4-digit PIN = 10k guesses (offline brute-force) |
| "Air-gapped" | ESP32 connects via phone hotspot |

---

## 📁 Project Structure

```
project-aether/
├── contracts/                    # Solidity smart contracts
│   ├── FileVerification.sol      # Current deployed contract
│   ├── AetherPolicy.sol          # Target policy contract
│   ├── script/                   # Foundry deploy scripts
│   └── test/                     # Forge tests
│
├── host/                         # Python host tooling
│   ├── host_transfer.py          # Upload + register files
│   ├── requirements.txt
│   ├── .env.example
│   └── .env                      # (gitignored)
│
├── firmware/                     # ESP32 firmware (PlatformIO)
│   ├── src/
│   │   ├── main.cpp              # Entry point, WiFi, session, API
│   │   ├── config.h              # Build-time config
│   │   ├── network/              # WiFi, captive portal, mDNS
│   │   ├── session/              # Session management
│   │   └── ui/                   # Embedded HTML pages (PROGMEM)
│   ├── include/
│   │   ├── setup_page.h          # Provisioning page
│   │   ├── login_page.h          # PIN entry + preview
│   │   └── pages.h               # Success/error pages
│   ├── platformio.ini
│   └── partitions.csv
│
├── contracts/                    # Solidity contracts
├── docs/                         # Documentation
│   ├── ARCHITECTURE.md
│   ├── BOARDS.md
│   ├── CRYPTO.md
│   ├── DEPLOYMENT.md
│   ├── HARDWARE.md
│   ├── ONBOARDING.md
│   ├── RED_TEAM.md
│   ├── SPEC.md
│   └── THREAT_MODEL.md
├── .github/workflows/            # CI/CD
├── .github/ISSUE_TEMPLATE/       # Issue templates
├── .gitignore
├── .env.example
├── LICENSE
├── CONTRIBUTING.md
├── SECURITY.md
└── README.md
```

---

## 🛠 Development

### Firmware (PlatformIO)
```bash
cd firmware
pio run          # Build
pio run -t upload    # Flash (bootloader: hold BOOT, tap EN, release BOOT)
pio device monitor   # Serial monitor
```

### Host Script
```bash
cd host
pip install -r requirements.txt
python host_transfer.py ./file.pdf    # Prompts for PIN
```

### Contract (Foundry)
```bash
cd contracts
forge test
forge script Deploy --rpc-url $BASE_SEPOLIA_RPC --broadcast --verify
```

---

## 🤝 Contributing

We welcome contributions! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Board Support
Want to add a new board?
1. `git checkout -b board/<board-name>`
2. Add board config in `firmware/src/config.h`
3. Update pinout in `docs/BOARDS.md`
3. Test on hardware
4. PR with hardware photos + test logs

### Code Style
- C++: Arduino style, 4-space indent
- Python: Black formatted, type hints
- JS: ES6+, Prettier formatted

---

## 🔒 Security

- **No secrets in repo** — `.env` is gitignored, use `.env.example`
- **Report vulnerabilities** → [SECURITY.md](SECURITY.md)
- **Audit scope** — See [RED_TEAM.md](docs/RED_TEAM.md)

---

## 📄 License

MIT License — see [LICENSE](LICENSE)

> **Project Aether** — *Build it because you understand the tradeoffs. Not because it sounds cool.*

---

## 🙏 Acknowledgments

- [Espressif](https://www.espressif.com/) — ESP32 & Arduino core
- [Pinata](https://pinata.cloud/) — IPFS pinning
- [Alchemy](https://alchemy.com/) — Base Sepolia RPC
- [Base](https://base.org/) — L2 scaling
- [ethers.js](https://docs.ethers.org/) — Ethereum library
- [mbedTLS](https://tls.mbed.org/) — TLS library
- [PDF.js](https://mozilla.github.io/pdf.js/) — PDF rendering

---

**Star ⭐ if this helps your project!**  
Questions? [Open an issue](https://github.com/your-org/project-aether/issues) or [start a discussion](https://github.com/your-org/project-aether/discussions).