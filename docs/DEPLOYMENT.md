# Project Aether — Deployment & Operations Guide
**Version:** 1.0 | **Classification:** INTERNAL

---

## 1. ENVIRONMENT SETUP

### 1.1 Required Accounts & Keys

| Service | Purpose | Key Type | Storage |
|---------|---------|----------|---------|
| GitHub | Source control | SSH / PAT | 1Password / Bitwarden |
| Alchemy | Base Sepolia RPC | API Key | Env var / Secret Manager |
| Pinata | IPFS pinning | JWT | Env var / Secret Manager |
| Base Sepolia | Contract deployment | EOA Private Key | Hardware wallet (Ledger) |
| AWS/GCP/Azure | Optional hosting | IAM Keys | Secret Manager |

### 1.2 Local Development Environment

```bash
# Required tools
git --version                    # ≥ 2.30
python --version                 # ≥ 3.11
node --version                   # ≥ 20
pnpm --version                   # ≥ 8
foundry --version                # forge, cast, anvil
platformio --version             # ≥ 6.1
idf.py --version                 # ESP-IDF ≥ 5.0

# Python deps (host)
pip install -r host/requirements.txt

# Node deps (frontend)
cd frontend && pnpm install

# Foundry deps (contracts)
cd contracts && forge install
```

---

## 2. CONTRACT DEPLOYMENT

### 2.1 Networks

| Network | Chain ID | RPC URL | Explorer |
|---------|----------|---------|----------|
| Base Sepolia (Test) | 84532 | `https://base-sepolia.g.alchemy.com/v2/<key>` | `https://sepolia.basescan.org` |
| Base Mainnet (Prod) | 8453 | `https://base-mainnet.g.alchemy.com/v2/<key>` | `https://basescan.org` |

### 2.2 Deployment Script (Foundry)

```bash
# contracts/Deploy.s.sol
// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

import "forge-std/Script.sol";
import "../src/AetherPolicy.sol";

contract DeployScript is Script {
    function run() external {
        vm.startBroadcast(vm.envOr("DEPLOYER_PK"));
        
        AetherPolicy policy = new AetherPolicy();
        
        // Register host as admin (deployer)
        // Authorize gates will be done separately via ProvisionGates.s.sol
        
        vm.stopBroadcast();
        
        console.log("AetherPolicy deployed at:", address(policy));
    }
}
```

### 2.3 Deploy Commands

```bash
# Testnet
cd contracts
forge script Deploy --rpc-url $BASE_SEPOLIA_RPC --broadcast --verify -vvvv

# Mainnet (requires multisig approval)
forge script Deploy --rpc-url $BASE_MAINNET_RPC --broadcast --verify -vvvv
```

### 2.4 Post-Deployment

```bash
# 1. Verify contract on BaseScan
forge verify-contract <ADDRESS> AetherPolicy --chain-id 84532 --etherscan-api-key $BASESCAN_KEY

# 2. Set initial policy (if needed)
cast send <ADDRESS> "setPolicy(bytes32,Policy)" <file_id> <policy_json> --rpc-url $BASE_SEPOLIA_RPC --private-key $ADMIN_PK

# 3. Record deployment
echo "CONTRACT_ADDRESS=<ADDRESS>" >> .env.production
echo "DEPLOY_TX=<TX_HASH>" >> .env.production
echo "DEPLOY_BLOCK=<BLOCK_NUMBER>" >> .env.production
```

---

## 3. GATE PROVISIONING (PRODUCTION)

### 3.1 Provisioning Script

```bash
# contracts/ProvisionGates.s.sol
// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

import "forge-std/Script.sol";
import "../src/AetherPolicy.sol";

contract ProvisionGatesScript is Script {
    function run() external {
        vm.startBroadcast(vm.envOr("ADMIN_PK"));
        
        AetherPolicy policy = AetherPolicy(vm.envOr("CONTRACT_ADDRESS"));
        
        // Device addresses from provisioning ceremony
        address[] memory gates = [
            0x...NodeA,
            0x...NodeB,
            0x...NodeC
        ];
        
        for (uint i = 0; i < gates.length; i++) {
            policy.authorizeGate(gates[i], true);
            console.log("Authorized gate:", gates[i]);
        }
        
        vm.stopBroadcast();
    }
}
```

### 3.2 Execute Provisioning

```bash
cd contracts
forge script ProvisionGates --rpc-url $BASE_SEPOLIA_RPC --broadcast -vvvv
```

---

## 4. HOST SCRIPT DEPLOYMENT

### 4.1 Configuration (`.env`)

```bash
# host/.env
PINATA_JWT=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
BASE_SEPOLIA_RPC_URL=https://base-sepolia.g.alchemy.com/v2/<key>
CONTRACT_ADDRESS=0xA3FFBC19730222a2C0a85e65791B890B4ecd6C26
HOST_WALLET_ADDRESS=0x1351Dc0Dd4658E780FbeFD5C5253296De5F0E0cD
HOST_PRIVATE_KEY=de0dd3a0f31d5d8140d22d7e5fd96f5caed72fa80b81644111c9c072260a345f
MASTER_KEY_PATH=./keys/master.key  # 32 bytes, generated offline
```

### 4.2 Master Key Generation (Offline)

```bash
# On air-gapped machine
openssl rand 32 > master.key
sha256sum master.key  # Record for verification
# Transfer via QR code / paper / hardware token ONLY
```

### 4.3 Usage

```bash
cd host
python -m aether_host.cli encrypt \
    --file ./payload.pdf \
    --file-id "550e8400-e29b-41d4-a716-446655440000" \
    --devices AETHER-NODE-001,AETHER-NODE-002,AETHER-NODE-003 \
    --policy '{"validFrom": 1735689600, "validUntil": 1735776000, "maxRetrievals": 1, "requiresThreshold": true}'
```

---

## 5. FIRMWARE BUILD & FLASH

### 5.1 Build Configuration

```ini
# firmware/platformio.ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_speed = 921600

build_flags = 
    -D CORE_DEBUG_LEVEL=0
    -D AETHER_DEVICE_ID=\"AETHER-NODE-001\"
    -D AETHER_FIRMWARE_VERSION=\"1.0.0\"
    -D MBEDTLS_CONFIG_FILE=\"mbedtls_config.h\"

board_build.partitions = partitions.csv
board_build.filesystem_size = 0x40000

lib_deps = 
    knolleary/PubSubClient@^2.8
```

### 5.2 Build Commands

```bash
cd firmware

# Clean build
pio run --target clean

# Build
pio run

# Flash (ESP32 in bootloader mode: hold BOOT, press EN, release BOOT)
pio run --target upload

# Monitor
pio device monitor --baud 115200
```

### 5.3 OTA Update (Production)

```bash
# 1. Build signed firmware
pio run --target build
# Sign with manufacturer key (offline)
# python sign_firmware.py build/merged.bin mfr_sk.pem firmware_update.bin

# 2. Upload via HTTPS (requires OTA endpoint in firmware)
curl -X POST https://aether-node.local/ota \
     -H "Authorization: Bearer <OTA_TOKEN>" \
     -F "firmware=@firmware_update.bin"

# 3. Or via serial (recovery)
pio run --target uploadfs  # Upload filesystem (certs, etc.)
```

---

## 6. FRONTEND BUILD & DEPLOY

### 6.1 Build Configuration

```javascript
// frontend/vite.config.js
import { defineConfig } from 'vite'
import { resolve } from 'path'

export default defineConfig({
  root: 'src',
  build: {
    outDir: '../dist',
    emptyOutDir: true,
    minify: 'esbuild',
    target: 'es2022',
    rollupOptions: {
      input: {
        main: resolve(__dirname, 'src/index.html'),
        sw: resolve(__dirname, 'src/sw.js')
      }
    }
  },
  server: {
    port: 3000,
    https: {
      key: fs.readFileSync('../certs/dev-key.pem'),
      cert: fs.readFileSync('../certs/dev-cert.pem')
    }
  }
})
```

### 6.2 Build Commands

```bash
cd frontend

# Development
pnpm dev

# Production build
pnpm build

# Output: dist/ (embed in firmware as dashboard_html.h)
# Convert: xxd -i dist/index.html > ../firmware/include/dashboard_html.h
```

---

## 7. NETWORK CONFIGURATION

### 7.1 Phone Hotspot Requirements
- **Band**: 2.4 GHz only (ESP32 doesn't support 5 GHz)
- **Security**: WPA2-PSK (AES)
- **Channel**: Auto or 1/6/11
- **DHCP**: Enabled
- **Client isolation**: Disabled (AP isolation off)

### 7.2 Hostel/Corporate WiFi (Fallback)
- May block: mDNS, IPFS gateways, non-standard ports
- **Test before demo**: `ping gateway.pinata.cloud`, `nslookup aether-node.local`
- **Always prefer phone hotspot** for demo reliability

### 7.3 Firewall Rules (If Hosting Infra)

| Direction | Port | Protocol | Purpose |
|-----------|------|----------|---------|
| Inbound | 443 | TCP | ESP32 HTTPS (if public) |
| Inbound | 53 | UDP | mDNS (local only) |
| Outbound | 443 | TCP | Alchemy RPC, Pinata, IPFS gateways |
| Outbound | 123 | UDP | NTP |

---

## 8. MONITORING & LOGGING

### 8.1 ESP32 Logs (Serial)
```
# Save logs during demo
pio device monitor --baud 115200 --filter log2file --log-file logs/esp32_$(date +%Y%m%d_%H%M%S).log
```

### 8.2 Structured Log Format (JSON)
```json
{
  "timestamp": "2025-01-15T10:30:45.123Z",
  "device_id": "AETHER-NODE-001",
  "event": "SESSION_CREATED",
  "session_id": "0x...",
  "file_id": "0x...",
  "policy_hash": "0x...",
  "approvals": 0
}
```

### 8.3 Key Metrics to Track
| Metric | Target | Alert If |
|--------|--------|----------|
| Session success rate | > 95% | < 90% |
| ESP-NOW approval latency | < 2s | > 5s |
| IPFS fetch time | < 10s | > 30s |
| SHA-256 verify time | < 500ms | > 2s |
| Firmware crash rate | 0 | > 0 |

---

## 9. DEMO CHECKLIST (PRE-FLIGHT)

### 9.1 24 Hours Before
- [ ] All 3 ESP32s provisioned, tested, in enclosures
- [ ] Contract deployed, gates authorized, policy set
- [ ] Master key accessible (offline backup verified)
- [ ] Payload file encrypted, uploaded, registered
- [ ] Pinata gateway tested: `curl https://gateway.pinata.cloud/ipfs/<CID>`
- [ ] Alchemy RPC responding: `cast block-number --rpc-url $RPC`
- [ ] Phone hotspots configured (2.4 GHz, WPA2, tested)
- [ ] Laptop: host script runs, Python deps installed
- [ ] Serial cables, power banks, USB OTG adapters packed

### 9.2 1 Hour Before
- [ ] ESP32s powered on, serial monitors running
- [ ] Device IDs confirmed on serial output
- [ ] ESP-NOW mesh forms (each node sees 2 peers)
- [ ] Phone connects to Aether-Setup → provisions to hotspot
- [ ] mDNS resolves: `http://aether-node.local/`
- [ ] IP fallback works: `http://<STA_IP>/`
- [ ] PIN test: wrong PIN → rejected, correct PIN → file downloads
- [ ] SHA-256 verify: tampered file → rejected
- [ ] Battery levels > 80% on all power banks

### 9.3 During Demo
- [ ] Narrate each step (threat model, trust boundaries)
- [ ] Show serial monitor for transparency
- [ ] Demonstrate attack: wrong PIN, tampered file, single node
- [ ] Demonstrate success: 2-of-3 buttons → file renders
- [ ] Show zero-disk render (no download folder entry)

### 9.4 Post-Demo
- [ ] Power off ESP32s
- [ ] Collect logs
- [ ] Verify no sensitive data in logs
- [ ] Backup provisioning logs

---

## 10. DISASTER RECOVERY

| Scenario | Recovery |
|----------|----------|
| ESP32 lost/stolen | `authorizeGate(address, false)` on-chain; provision replacement |
| Master key compromised | Rotate MK, re-encrypt all files, re-provision all gates |
| Contract vulnerability | Upgrade via proxy (multisig); emergency pause if needed |
| Pinata down | Gateway fallback handles; re-pin to new provider |
| Alchemy down | Switch RPC endpoint in frontend/firmware config |
| Firmware brick | Serial recovery: `pio run --target upload` with factory firmware |

---

## 11. COST ESTIMATE (PRODUCTION SCALE)

| Component | Unit Cost | Qty | Total |
|-----------|-----------|-----|-------|
| ESP32-WROVER-B | $8 | 3 | $24 |
| ATECC608B | $1.50 | 3 | $4.50 |
| PCB Assembly | $15 | 3 | $45 |
| Enclosure | $10 | 3 | $30 |
| Buttons/LEDs | $2 | 3 | $6 |
| **Hardware Total** | | | **$109.50** |
| Alchemy (Growth) | $49/mo | - | $49/mo |
| Pinata (Picnic) | Free (1GB) | - | $0 |
| Base Sepolia Gas | ~$0.001/tx | 100/day | ~$3/mo |
| **Ops/Month** | | | **~$52/mo** |

---

## 12. SECURITY CONTACTS

| Role | Contact | PGP Key |
|------|---------|---------|
| Security Lead | security@aether.local | `0x...` |
| Hardware Lead | hardware@aether.local | `0x...` |
| Contract Admin | contracts@aether.local | `0x...` |
| Incident Response | incident@aether.local | `0x...` |

**Responsible Disclosure:** Email `security@aether.local` with PGP-encrypted details. 90-day disclosure window.

---

**END OF DEPLOYMENT GUIDE**