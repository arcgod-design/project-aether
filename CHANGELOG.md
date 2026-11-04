# Changelog

All notable changes to Project Aether.

---

## [1.1.0] - 2025-11-09

### Added
- **Pure-JS SHA-256** — Works on HTTP, no `crypto.subtle` needed (fixes integrity check on HTTP)
- **Per-PIN file retrieval** — Each PIN retrieves exactly one file (no file list)
- **Preview mode** — PDF/image/text render inline; zips show "use Download"
- **Auto-authorize sessions** — No physical button required (GPIO 4 not wired on demo boards)
- **Multi-gateway IPFS** — Fallback across Pinata/dweb.link/ipfs.io/cloudflare-ipfs.com
- **Cache-Control: no-store** on all pages — kills browser cache issues
- **Login page** — Single PIN entry, direct file preview/download
- **Host script** — Accepts any file type (PDF/image/text), not just .zip

### Fixed
- **SHA-256 padding bug** — Fixed padding calculation + length field placement (was broken for all inputs)
- **Duplicate JS declarations** — Removed duplicate `const log/debug/pinValid` that broke button enable
- **PIN hash mismatch** — Frontend now uses `keccak256(PIN)` matching host registration (was `keccak256(PIN+nonce)`)
- **Duplicate login page** — Removed duplicated HTML/JS in login template
- **Cache issues** — Added `Cache-Control: no-store` headers; kills stale auto-download page cache
- **Session auto-authorize** — Removed physical button gate (no button wired on demo boards)

### Changed
- **Login flow** — PIN → auto-authorized session → preview/download (no button gate)
- **File registration** — Host script accepts any file type (PDF/image/text), not just .zip
- **Session TTL** — 60 seconds, 3 max attempts, auto-expire
- **Cache headers** — `Cache-Control: no-store` on all pages

---

## [1.0.0] - 2025-10-15

### Added
- **MVP Release** — End-to-end working demo
- **ESP32 provisioning** — AP → STA WiFi flow with captive portal
- **Base Sepolia contract** — PIN→CID registration + `getFile(pinHash)`
- **Host script** — Encrypt → Pinata IPFS → Base Sepolia registration
- **ESP32 WiFi provisioning** — AP → STA with captive portal + mDNS
- **Frontend** — PIN entry, ethers.js blockchain query, IPFS multi-gateway fetch
- **AES-256-GCM** host-side encryption, SHA-256 on-chain commitment
- **Host script** — Encrypt + upload to Pinata + register on Base Sepolia
- **ESP32 captive portal** — DNS hijack + HTTP redirect for provisioning

### Known Issues (v1.0.0)
- SHA-256 verification skipped on HTTP (browser blocks `crypto.subtle`)
- Physical button required for authorization (not wired on demo boards)
- 4-digit PIN brute-forceable (public chain)
- No TLS, no secure element, no attestation
- Single file per PIN (no multi-file per PIN)

---

## [Unreleased]

### Planned (Phase 1)
- [ ] TLS on ESP32 (mbedTLS + self-signed cert + cert pinning)
- [ ] ATECC608B driver + provisioning
- [ ] Ed25519 auth tokens (replace 4-digit PIN)
- [ ] AES-256-GCM host + WASM decrypt (browser)
- [ ] Double Ratchet session keys

### Planned (Phase 2)
- [ ] Physical button approval flow (GPIO interrupt)
- [ ] ESP-NOW 2-of-3 threshold approval
- [ ] Remote attestation (firmware hash + monotonic counter)
- [ ] Policy engine on-chain (time/geo/count)

### Planned (Phase 3)
- [ ] Cover traffic generator
- [ ] Forward-secure session ratchet
- [ ] Revocation list (on-chain)
- [ ] Audit log (IPFS + Merkle root on-chain)

### Board Support
- [ ] ESP32-S3 DevKitC-1 (`board/esp32s3-devkit`)
- [ ] ESP32-C3 DevKitM-1 (`board/esp32c3-devkit`)
- [ ] ESP32-S2 DevKit (`board/esp32s2-devkit`)
- [ ] M5Stack Core2 (`board/m5stack-core2`)
- [ ] LilyGo T-Display S3 (`board/lilygo-tdisplay-s3`)

---

## Versioning

`MAJOR.MINOR.PATCH` — Semantic Versioning

- **MAJOR** — Breaking protocol changes (contract upgrade)
- **MINOR** — New features, new board support
- **PATCH** — Bug fixes, docs, minor improvements

---

## Links

- [Architecture](docs/ARCHITECTURE.md)
- [Board Support](docs/BOARDS.md)
- [Deployment](docs/DEPLOYMENT.md)
- [Threat Model](docs/THREAT_MODEL.md)
- [Crypto Spec](docs/CRYPTO.md)
- [Hardware Guide](docs/HARDWARE.md)
- [Community Boards](docs/COMMUNITY_BOARDS.md)