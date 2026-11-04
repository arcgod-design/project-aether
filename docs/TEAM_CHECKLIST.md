# Team Checklist

## Member A — Hardware
- [ ] ESP32 #1 assigned as development board
- [ ] COM9 visible in Device Manager
- [ ] Firmware uploaded
- [ ] Serial output verified at 115200
- [ ] `Aether-Node` Wi-Fi visible
- [ ] `http://192.168.4.1/` opens

## Member B — Blockchain
- [ ] MetaMask test wallet created
- [ ] Base Sepolia added
- [ ] Test ETH obtained
- [ ] Contract deployed
- [ ] Contract address recorded

## Member C — IPFS / Host
- [ ] Pinata account created
- [ ] Pinata JWT recorded safely
- [ ] `host/.env` created and ignored by git
- [ ] ZIP uploaded
- [ ] CID recorded
- [ ] transaction hash recorded

## Member D — Integration
- [ ] Alchemy Base Sepolia RPC obtained
- [ ] `index_html.h` configured
- [ ] ESP32 reflashed
- [ ] correct PIN retrieves metadata
- [ ] file downloads
- [ ] browser SHA-256 matches blockchain

## Demo pass criteria
- [ ] Wrong PIN fails
- [ ] Correct PIN succeeds
- [ ] Modified ZIP fails integrity check
- [ ] Re-uploaded file produces a different SHA-256/CID
- [ ] No private keys/JWTs are in committed source
