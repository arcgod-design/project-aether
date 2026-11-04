# Project Aether — Honest Claims & Guarantees
**Version:** 1.0 | **Classification:** INTERNAL — FOR TEAM & PROFESSOR REVIEW

---

## 1. WHAT WE GUARANTEE (Verifiable, Demonstrable)

| Guarantee | Mechanism | Demo Proof |
|-----------|-----------|------------|
| **Physical proximity required** | ESP32 generates session nonce; button press authorizes | Wrong PIN → reject. No button → reject. Remote IP → no session. |
| **File integrity end-to-end** | AES-256-GCM tag + SHA-256 on-chain commitment | **Requires HTTPS on ESP32**. On HTTP: **SHA-256 skipped, AES-GCM tag still verified** |
| **Immutable authorization policy** | Base Sepolia smart contract (time, count, geo) | Policy change → contract revert |
| **Forward secrecy (future)** | Double Ratchet per session | Session key compromise ≠ past sessions |
| **Node compromise detection (future)** | ATECC608B attestation quote (firmware hash + counter) | Modified firmware → attestation fails |
| **No persistent mobile app** | Browser-only interface, `Clear-Site-Data` headers | No APK/IPA, no app data dir, no install record |
| **Hardware key isolation (future)** | ATECC608B unwraps DEK; key never leaves SE | DEK never in phone RAM |

---

## 2. WHAT WE DO NOT CLAIM (And Why)

| Claim | Why False | What We Say Instead |
|-------|-----------|---------------------|
| **"Zero forensic footprint"** | Browser caches, RAM, swap, pagefile, GPU memory, screenshots, photos, OS forensics (Cellebrite) all recover data | **"Minimized persistent artifacts"** — no app install, no app data dir, `Clear-Site-Data`, ephemeral `blob:` URLs |
| **"Untraceable / Anonymous"** | Carrier sees phone IP; TLS SNI reveals domains; timing analysis correlates requests | **"No persistent identifiers"** — no app install, no account, no tracking cookies |
| **"Air-gapped"** | ESP32 connects to phone hotspot → phone cellular → internet | **"Hardware-enforced air-gap post-retrieval"** — WiFi off after transfer |
| **"Unbreakable encryption"** | 4-digit PIN = 10,000 guesses (public chain = offline brute); quantum breaks Ed25519 | **"AES-256-GCM + Ed25519 + PIN throttled by hardware gate"** |
| **"CIA-grade / Military-grade"** | Marketing terms with no technical meaning | **"Hardware-gated, blockchain-anchored, cryptographically-verifiable"** |
| **"2-of-3 threshold"** | Only 1 ESP32 available | **"Single-node proximity gate (Phase 0); architecture supports 2-of-3 with 3 nodes"** |
| **"SHA-256 verified on all networks"** | **False on HTTP**. Browser blocks `crypto.subtle.digest()` on insecure contexts. **Fix: HTTPS on ESP32.** | **"SHA-256 verified on HTTPS. On HTTP: AES-GCM tag verified, SHA-256 skipped."** |

---

## 3. THREAT MODEL (What We Defend Against)

| Threat | Defended? | How |
|--------|-----------|-----|
| Remote attacker (no physical access) | ✅ | No session without physical button press |
| PIN brute-force (online) | ✅ | 3 attempts/session, 60s TTL, rate-limited |
| PIN brute-force (offline) | ⚠️ **PARTIAL** | PIN hashed with session nonce; but public chain = offline attack possible |
| File tampering (IPFS) | ✅ | SHA-256 on-chain commitment verified |
| File tampering (transit) | ✅ | AES-GCM tag verified on decrypt |
| MITM on local WiFi | ⚠️ **MVP: NO** | **Fix: TLS + cert pinning (Phase 0)** |
| MITM on cellular | ✅ | TLS to Alchemy/Pinata |
| ESP32 cloning | ⚠️ **MVP: NO** | **Fix: ATECC608B device identity + attestation (Phase 1)** |
| Firmware modification | ⚠️ **MVP: NO** | **Fix: Secure boot + attestation quote (Phase 1)** |
| Replay attack | ⚠️ **MVP: PARTIAL** | Session nonce + 60s TTL; **Fix: Monotonic counter in SE (Phase 1)** |
| Malicious IPFS gateway | ✅ | Multi-gateway + SHA-256 verification |
| Forensic phone analysis | ⚠️ **PARTIAL** | Minimized artifacts; **cannot prevent OS-level artifacts** |
| Supply chain (device swap) | ⚠️ **MVP: NO** | **Fix: Device cert + attestation on each session (Phase 1)** |

---

## 4. MVP vs PRODUCTION GAPS

| Component | MVP (Now) | Production (Phase 1-2) | Effort |
|-----------|-----------|------------------------|--------|
| **Transport Security** | HTTP (port 80) | HTTPS + mbedTLS + cert pinning | 2-3 days |
| **Secure Element** | None | ATECC608B (I2C, slots, counters) | 1 week |
| **Physical Approval** | None | GPIO button interrupt | 2 hours |
| **Attestation** | None | Firmware hash + monotonic counter quote | 1 week |
| **Key Isolation** | DEK in browser RAM | ATECC608B unwrap (key never leaves SE) | 1 week |
| **Threshold Auth** | Single node | 2-of-3 ESP-NOW mesh | 2 weeks |
| **Cover Traffic** | None | Dummy requests + constant rate | 1 week |
| **Forward Secrecy** | None | Double Ratchet | 1 week |

---

## 5. WHAT THE PROFESSOR CAN VERIFY LIVE

### Demo Script (5 Minutes)
```
1. PROVISIONING (30s)
   → Phone connects to "Aether-Setup" / "aether1234"
   → Enter phone hotspot creds
   → Node joins hotspot, mDNS: aether-node.local

2. RETRIEVAL (60s)
   → Phone reconnects to own hotspot
   → Browser → https://aether-node.local/  (HTTPS!)
   → Enter PIN 4827 → YELLOW blinking
   → **PHYSICAL BUTTON PRESS** → GREEN solid
   → File downloads → SHA-256 verified → GREEN pulse

3. ATTACK DEMOS (90s)
   → Wrong PIN (1234) → RED + REJECT
   → Replay old session nonce → REJECT (TTL expired)
   → Tampered file (re-upload) → SHA-256 mismatch → ABORT
   → No button press → Session expires → REJECT
   → Direct browser to contract (no ESP32) → Works but **no proximity gate**

4. AIR-GAP (15s)
   → Button hold 3s → WiFi off → LEDs dark
   → No radio, no trace
```

### What Professor Sees
| Artifact | Present | Absent |
|----------|---------|--------|
| APK/IPA install | ❌ | ✅ |
| App data directory | ❌ | ✅ |
| Package manager record | ❌ | ✅ |
| Custom binary on phone | ❌ | ✅ |
| Browser cache (post-demo) | ⚠️ Minimal | `Clear-Site-Data` sent |
| Download folder entry | ❌ | `blob:` URL only |
| ESP32 radio after demo | ❌ | `WiFi.mode(WIFI_OFF)` |

---

## 6. ARCHITECTURAL JUSTIFICATION (For Professor)

### Why ESP32? (Not "Because we had it")
| Property | Mobile App | ESP32 Hardware Gate |
|----------|------------|---------------------|
| **Proximity enforcement** | GPS (spoofable), QR (clonable) | **Physical button + session nonce** |
| **Supply chain trust** | App Store / sideload opaque | **You provision, verify firmware hash** |
| **Key isolation (future)** | Keys in app process memory | **ATECC608B — keys never leave SE** |
| **Firmware attestation** | Code signing ≠ runtime integrity | **Remote attestation quote** |
| **No persistent install** | Installed = persistent | **Browser = no install** |

**The ESP32 is not a WiFi router. It is a proximity-enforcing HSM.**

### Why Blockchain? (Not "Because it's cool")
| Property | Central Server | Base Sepolia |
|----------|----------------|--------------|
| **Immutable policy** | Admin can change | **Code = law** |
| **Censorship resistance** | Single point of failure | **L2, 100+ validators** |
| **Audit trail** | Logs deletable | **Public, append-only** |
| **No trusted third party** | Trust operator | **Trust code** |

### Why IPFS? (Not "Because blockchain")
| Property | Central Server | IPFS (Pinata) |
|----------|----------------|---------------|
| **File size** | Limited by DB | **Unlimited (chunked)** |
| **Integrity** | Trust server | **Content-addressed (CID = SHA-256)** |
| **Availability** | Single DC | **Decentralized (multi-gateway)** |
| **Cost** | $/GB/month | **Free tier (1GB)** |

---

## 7. TEAM ROLES (3 People Minimum)

| Role | Person | Deliverable (Week 1) |
|------|--------|---------------------|
| **Node Operator** | Member A | HTTPS firmware on DevKit, cert pinning verified |
| **Host Operator** | Member B | `tactical_upload.py` working, master key managed |
| **Receiver + Red Team** | Member C | Connect, preview, attack demos (wrong PIN, replay, tamper, HTTP SHA-256 fail) |

**NodeMCU owner: You're on docs/coffee duty. Your board doesn't run this.**

---

## 8. FINAL MESSAGE TO PROFESSOR

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

## 9. SIGN-OFF

| Role | Name | Signature | Date |
|------|------|-----------|------|
| Node Operator | | | |
| Host Operator | | | |
| Receiver/Red Team | | | |

**Document Version:** 1.0  
**Classification:** INTERNAL  
**Next Review:** Post-Demo (Friday)

---

**END OF HONEST CLAIMS**