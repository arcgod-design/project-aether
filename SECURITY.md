# Security Policy

## Supported Versions

| Version | Supported |
|---------|-----------|
| 1.x.x (current) | ✅ |
| < 1.0.0 | ❌ |

---

## Reporting a Vulnerability

**DO NOT** open a public issue for security vulnerabilities.

### Report via Email
**security@project-aether.dev**

Include:
- Description of the vulnerability
- Steps to reproduce
- Affected components (firmware/host/contract/frontend)
- Potential impact
- Suggested fix (if any)

### Response Timeline
| Severity | Response | Fix Target |
|----------|----------|------------|
| Critical (RCE, key extraction) | 24 hours | 7 days |
| High (auth bypass, key leak) | 48 hours | 14 days |
| Medium (info leak, DoS) | 5 days | 30 days |
| Low (minor info leak) | 14 days | Next release |

---

## Disclosure Policy

- We follow **coordinated disclosure**
- Credit given in advisory (unless anonymity requested)
- CVE requested for qualifying issues
- No legal action against good-faith researchers

---

## Scope

### In Scope
| Component | Scope |
|-----------|-------|
| Firmware (`firmware/src/`) | ✅ |
| Host script (`host/`) | ✅ |
| Smart contracts (`contracts/`) | ✅ |
| Frontend (embedded HTML/JS) | ✅ |
| Smart contract deployment scripts | ✅ |

### Out of Scope
- Third-party services (Pinata, Alchemy, Base Sepolia)
- ESP32 hardware silicon bugs
- ESP-IDF/Arduino core vulnerabilities (report upstream)
- Physical hardware attacks (requires physical access)

---

## Security Architecture

### Threat Model
See [THREAT_MODEL.md](docs/THREAT_MODEL.md) for full adversary capabilities and mitigations.

### Key Guarantees (Post-Production)
| Guarantee | Mechanism |
|-----------|-----------|
| Physical proximity required | ESP32 session nonce + hardware button |
| File integrity | AES-GCM tag + on-chain SHA-256 commitment |
| Immutable policy | Base L2 smart contract |
| Forward secrecy | Double Ratchet per session |
| Node compromise detection | Remote attestation via ATECC608B |

### Known MVP Limitations
| Limitation | Mitigation |
|------------|------------|
| HTTP (no TLS) | Phase 0: TLS + cert pinning |
| No secure element | Phase 1: ATECC608B integration |
| 4-digit PIN (10k space) | Phase 1: Ed25519 auth tokens |
| No secure boot | ESP32-S3 Secure Boot v2 (Phase 1) |
| No attestation | Phase 1: ATECC608B attestation quotes |

---

## Secure Development Practices

### Code Review Requirements
- All PRs require 2 approvals
- `main` branch protected
- CI must pass (build, tests, static analysis)

### Secrets Management
- **Never commit secrets** — `.env` is gitignored
- Use `.env.example` as template
- Rotate keys quarterly
- Use hardware wallets for contract deployment

### Dependency Management
- Pin exact versions in `platformio.ini`, `requirements.txt`, `package.json`
- Run `pip-audit`, `npm audit`, `cargo audit` in CI
- Pin transitive dependencies where possible

### Code Review Checklist
- [ ] No hardcoded secrets
- [ ] Input validation on all external inputs
- [ ] No unsafe C++ (buffer overflows, use-after-free)
- [ ] Proper error handling (no info leaks in errors)
- [ ] Constant-time crypto operations
- [ ] No debug logs in production builds

---

## Incident Response

### Severity Levels
| Level | Criteria | Example |
|-------|----------|---------|
| **Critical** | RCE, key extraction, auth bypass | Private key extraction, RCE on ESP32 |
| **High** | Auth bypass, key leak, DoS | Auth bypass, key leak in logs |
| **Medium** | Info leak, partial DoS | PIN enumeration, partial file leak |
| **Low** | Minor info leak, minor DoS | Version info leak, minor UI bug |

### Response Process
1. **Acknowledge** — Within SLA (see table above)
2. **Investigate** — Reproduce, assess impact
3. **Mitigate** — Deploy hotfix or workaround
4. **Fix** — Root cause fix + tests
5. **Disclose** — Coordinated disclosure, CVE if applicable
6. **Postmortem** — Public postmortem within 30 days

---

## Secure Coding Guidelines

### C++ (Firmware)
- No `strcpy`/`sprintf` — use `snprintf`
- No `malloc`/`free` — use stack or static allocation
- Check all return codes
- `volatile` for hardware registers
- `IRAM_ATTR` for ISRs only
- `const` by default

### Python (Host)
- Type hints everywhere
- `subprocess` with `shell=False`
- `requests` with timeout
- `secrets` module for randomness
- `pathlib` over `os.path`

### JavaScript (Frontend)
- No `eval`/`Function` constructor
- CSP headers in meta tags
- Subresource integrity for CDN scripts
- No `localStorage` for sensitive data

### Solidity (Contracts)
- OpenZeppelin patterns
- ReentrancyGuard on all external calls
- `require`/`revert` with custom errors
- Events for all state changes
- NatSpec documentation

---

## Dependency Management

| Ecosystem | Tool | Policy |
|-----------|------|--------|
| C++ (PlatformIO) | `platformio.ini` | Pin exact versions |
| Python | `requirements.txt` | Pin exact, `pip-audit` in CI |
| Node.js | `package.json` | Pin exact, `npm audit` in CI |
| Solidity | `foundry.toml` | Pin OpenZeppelin version |

---

## Reporting Security Issues

**Email: security@project-aether.dev**

Include:
1. Description of vulnerability
2. Steps to reproduce
3. Affected component(s)
4. Potential impact
5. Proof of concept (if safe)
6. Suggested fix (optional)

**PGP Key:** [link to key]

---

## Hall of Fame

Thanks to responsible disclosure contributors:

| Researcher | Issue | Date |
|------------|-------|------|
| — | — | — |

*Be the first!*

---

## Legal Safe Harbor

We authorize good-faith security research on Project Aether within scope. We will not pursue legal action for good-faith research that follows this policy.