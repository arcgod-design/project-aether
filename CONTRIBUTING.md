# Contributing to Project Aether

Thank you for contributing! This project thrives on community involvement.

## 🤝 How to Contribute

### 1. Board Support (Primary Need)
We need support for more ESP32 variants! Each board gets its own branch.

```bash
# Create a new board branch
git checkout -b board/esp32c3-devkit
# Add board config, test, document
git push origin board/esp32c3-devkit
# Open PR with: hardware photos, serial logs, pinout diagram
```

### 2. Code Contributions
- **Firmware**: C++ (Arduino), PlatformIO
- **Host**: Python 3.11+, type hints, Black formatted
- **Frontend**: Vanilla ES6+, no build step, CDN ethers.js
- **Contracts**: Solidity 0.8.20, Foundry

### 3. Documentation
- Fix typos, clarify steps, add diagrams
- Translate to other languages
- Add hardware photos for your board

---

## 📋 Pull Request Checklist

- [ ] Tests pass (`forge test`, `pio test`)
- [ ] Code formatted (`black`, `prettier`, `clang-format`)
- [ ] No secrets in diff (run `git diff --check`)
- [ ] Updated docs if behavior changed
- [ ] Hardware tested (for firmware changes)
- [ ] Changelog entry in `CHANGELOG.md`

---

## 🏷 Branch Strategy

| Branch | Purpose | Protection |
|--------|---------|------------|
| `main` | Production releases | Required reviews, CI pass |
| `develop` | Integration branch | CI pass |
| `board/*` | Board-specific ports | Maintainer review |
| `feature/*` | New features | PR review |
| `fix/*` | Bug fixes | CI pass |
| `docs/*` | Documentation only | Quick merge |

### Board Branch Naming
```
board/esp32s3-devkit
board/esp32c3-devkit
board/esp32s2-devkit
board/esp32c6-devkit
```

---

## 🧪 Testing

```bash
# Contracts
cd contracts && forge test -vv

# Firmware
cd firmware && pio test

# Host
cd host && python -m pytest

# Frontend (optional)
cd frontend && npm test
```

---

## 📝 Commit Convention

```
<type>(<scope>): <subject>

<body>

<footer>
```

Types: `feat`, `fix`, `docs`, `refactor`, `test`, `chore`, `board`

Examples:
```
feat(firmware): add ATECC608B driver skeleton
fix(frontend): fix SHA-256 padding bug (fixes #42)
board(esp32s3): add ESP32-S3 DevKitC-1 support
docs(arch): update threat model for MVP
```

---

## 🏷 Versioning

Semantic Versioning: `MAJOR.MINOR.PATCH`

- **MAJOR**: Breaking protocol changes (contract upgrade needed)
- **MINOR**: New features, new board support
- **PATCH**: Bug fixes, doc updates

---

## 🏷 Labels

| Label | Meaning |
|-------|---------|
| `good first issue` | Beginner friendly |
| `board support` | New board port |
| `security` | Security related |
| `hardware` | Firmware/HW changes |
| `docs` | Documentation only |
| `help wanted` | Needs community help |

---

## 📞 Getting Help

- **Discord**: [Project Aether Community](https://discord.gg/placeholder)
- **Discussions**: [GitHub Discussions](https://github.com/your-org/project-aether/discussions)
- **Email**: security@project-aether.dev (for security issues)

---

## 📜 Code of Conduct

Be respectful, inclusive, and constructive. See [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

---

**Thank you for contributing to Project Aether!** 🚀