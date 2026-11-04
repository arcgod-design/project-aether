# Project Aether — Cryptographic Protocol Specification
**Version:** 1.0 | **Classification:** INTERNAL

---

## 1. NOTATION

| Symbol | Meaning |
|--------|---------|
| `||` | Concatenation |
| `HKDF(salt, ikm, info, L)` | RFC 5869 HKDF-SHA256 |
| `AES256-GCM(k, n, pt, aad)` → `(ct, tag)` | AES-256-GCM encryption |
| `AES256-GCM-Dec(k, n, ct, tag, aad)` → `pt / ⊥` | AES-256-GCM decryption |
| `Ed25519.Sign(sk, m)` → `σ` | Ed25519 signature |
| `Ed25519.Verify(pk, m, σ)` → `bool` | Ed25519 verification |
| `X25519(sk, pk)` → `shared` | X25519 ECDH |
| `SHA256(m)` → `h` | SHA-256 hash |
| `keccak256(m)` → `h` | Keccak-256 (Ethereum) |
| `random(n)` | Cryptographically secure random bytes |

---

## 2. KEY HIERARCHY & DERIVATION

### 2.1 Master Key (MK)
- Generated once during provisioning ceremony
- 256 bits, stored offline in HSM / paper backup
- **Never** enters any networked system

### 2.2 Derived Keys

```
MK (256 bits)
│
├── File Encryption Keys (per file)
│   DEK_i = HKDF(MK, salt=file_id, info="aether:dek", L=32)
│
├── Device Key Encryption Keys (per ESP32)
│   KEK_j = HKDF(MK, salt=device_id_j, info="aether:kek", L=32)
│
├── ESP-NOW Mesh Keys (per node pair)
│   ENC_NOW_{a,b} = HKDF(MK, salt=node_a||node_b, info="aether:espnow", L=16)
│
├── Session Keys (per retrieval session)
│   SESSION_K = DoubleRatchet(initiator_sk, responder_pk)
│
└── Attestation Keys (per device)
    ATT_{sk,pk} = ECDSA_P256_GenerateKey()  // In ATECC608B
```

### 2.3 Key Separation Principle
**Every key has a single, explicit purpose.** Domain separation via `info` parameter in HKDF. No key reused across contexts.

---

## 3. FILE ENCRYPTION PROTOCOL (HOST)

### 3.1 Inputs
- `P`: Plaintext file (any size)
- `file_id`: Unique identifier (UUID v4)
- `MK`: Master Key
- `authorized_devices[]`: List of device IDs (ESP32 nodes)

### 3.2 Algorithm

```
ENCRYPT_FILE(P, file_id, MK, authorized_devices):
    
    // 1. Derive per-file DEK
    DEK = HKDF(MK, salt=file_id, info="aether:dek", L=32)
    
    // 2. Encrypt payload
    N = random(12)                              // 96-bit nonce
    (C, T) = AES256-GCM(DEK, N, P, AAD=file_id)
    
    // 3. Compute commitments
    cid_commitment = SHA256(C)                  // For IPFS verification
    content_commitment = SHA256(P)              // For integrity verification
    metadata = file_id || len(P) || mime_type || timestamp
    metadata_hash = SHA256(metadata)
    
    // 4. Sign metadata (host identity)
    sig = Ed25519.Sign(host_sk, metadata_hash)
    
    // 5. Wrap DEK for each authorized device
    wrappers = []
    for device_id in authorized_devices:
        KEK = HKDF(MK, salt=device_id, info="aether:kek", L=32)
        wrapper_nonce = random(12)
        (W, W_tag) = AES256-GCM(KEK, wrapper_nonce, DEK, AAD=file_id)
        wrappers.append({
            device_id: device_id,
            nonce: wrapper_nonce,
            ciphertext: W,
            tag: W_tag
        })
    
    // 6. Output package
    return {
        file_id: file_id,
        ciphertext: C,
        nonce: N,
        tag: T,
        cid_commitment: cid_commitment,
        content_commitment: content_commitment,
        metadata_hash: metadata_hash,
        signature: sig,
        wrappers: wrappers
    }
```

### 3.3 IPFS Upload
```
CID = Pinata_Upload(C)
Assert SHA256(C) == cid_commitment
```

### 3.4 Contract Registration
```
Register on-chain:
  - file_id
  - CID
  - cid_commitment
  - content_commitment
  - metadata_hash
  - signature
  - wrappers (one per device)
  - policy (time, geo, quota, threshold)
```

---

## 4. SESSION PROTOCOL (RETRIEVAL)

### 4.1 Session Creation (ESP32 → Contract)

```
CREATE_SESSION(file_id, PIN, session_nonce):
    
    // 1. ESP32 generates session
    session_id = random(32)
    pin_hash = keccak256(PIN || session_nonce)
    
    // 2. ESP32 stores in RAM (volatile)
    session_state[session_id] = {
        file_id: file_id,
        pin_hash: pin_hash,
        created_at: now(),
        attempts: 0,
        authorized: false,
        approvals: 0
    }
    
    // 3. ESP32 calls contract
    tx = Contract.createSession(file_id, session_id, pin_hash)
    
    // 4. Contract validates
    require(files[file_id].exists)
    require(policies[file_id].validFrom <= block.timestamp <= validUntil)
    sessions[session_id] = Session(...)
    emit SessionCreated(session_id, file_id)
    
    // 5. ESP32 returns session_nonce to browser
    return session_nonce
```

### 4.2 Threshold Approval (ESP-NOW Mesh)

```
APPROVE_SESSION(session_id, file_id, policy_hash):
    
    // 1. Leader node (A) broadcasts to B, C
    frame = {
        version: 1,
        type: REQ_AUTH,
        session_id: session_id,
        file_id: file_id[0:16],
        policy_hash: policy_hash,
        timestamp: now()
    }
    mac = AES-CCM(ENC_NOW, frame)
    ESP-NOW_Send(peer_B, frame || mac)
    ESP-NOW_Send(peer_C, frame || mac)
    
    // 2. Nodes B, C verify
    require(verify_mac(frame, mac))
    require(frame.timestamp within 30s)
    require(frame.policy_hash == local_policy_hash)
    
    // 3. Human approval (physical button)
    wait_for_button_press(30s)
    
    // 4. Send approval
    approval_frame = {
        type: APPROVE,
        session_id: session_id,
        node_id: my_id,
        signature: Ed25519.Sign(node_sk, session_id || "APPROVE")
    }
    ESP-NOW_Send(leader, approval_frame)
    
    // 5. Leader collects 2 approvals
    if approvals >= 2:
        session_state[session_id].authorized = true
        Contract.submitApproval(session_id, threshold_sig)
```

### 4.3 File Retrieval (Browser)

```
RETRIEVE_FILE(session_id, PIN, session_nonce):
    
    // 1. Browser computes pin_hash
    pin_hash = keccak256(PIN || session_nonce)
    
    // 2. Query contract
    (cid, wrapper, content_commitment, metadata_hash, sig, policy) 
        = Contract.getFile(session_id, pin_hash)
    
    // 3. Verify policy
    require(now() within policy.validFrom..validUntil)
    require(policy.retrievalCount < policy.maxRetrievals)
    require(policy.requiresThreshold == false || session.authorized == true)
    
    // 4. Verify host signature
    require(Ed25519.Verify(host_pk, metadata_hash, sig))
    
    // 5. Fetch ciphertext from IPFS (multi-gateway)
    C = IPFS_Fetch(cid)  // Tries gateway.pinata.cloud, dweb.link, etc.
    require(SHA256(C) == cid_commitment)
    
    // 6. Unwrap DEK via ESP32 (local HTTPS)
    DEK = ESP32_Unwrap(session_id, wrapper)
    
    // 7. Decrypt & verify
    P = AES256-GCM-Dec(DEK, N, C, T, AAD=file_id)
    require(P != ⊥)
    require(SHA256(P) == content_commitment)
    
    // 8. Ephemeral render (no download)
    Render_Ephemeral(P, mime_type)
    
    // 9. Zeroize
    Zeroize(DEK, P)
```

### 4.4 ESP32 Unwrap Operation

```
ESP32_UNWRAP(session_id, wrapper):
    
    // 1. Validate session
    session = session_state[session_id]
    require(session != null)
    require(now() - session.created_at < 300s)
    require(session.attempts < 3)
    session.attempts++
    
    // 2. Verify wrapper belongs to this device
    require(wrapper.device_id == my_device_id)
    
    // 3. ATECC608B unwrap (key never leaves SE)
    KEK = SE_Read_Key(slot=KEK_slot)  // Internal only
    DEK = SE_AES256_GCM_Decrypt(
        key_slot=KEK_slot,
        nonce=wrapper.nonce,
        ciphertext=wrapper.ciphertext,
        tag=wrapper.tag,
        aad=session.file_id
    )
    
    // 4. Return DEK (ephemeral, in ESP32 RAM only)
    return DEK
```

---

## 5. DOUBLE RATCHET (SESSION KEYS)

### 5.1 Initialization
```
INIT_RATCHET(initiator_sk, responder_pk):
    shared = X25519(initiator_sk, responder_pk)
    root_key = HKDF(salt=0, ikm=shared, info="aether:ratchet:root", L=32)
    send_chain = HKDF(root_key, info="send", L=32)
    recv_chain = HKDF(root_key, info="recv", L=32)
    return (root_key, send_chain, recv_chain, message_num=0)
```

### 5.2 Sending Message
```
RATCHET_SEND(send_chain, plaintext, aad):
    msg_key = HKDF(send_chain, info="msg", L=32)
    send_chain = HKDF(send_chain, info="next", L=32)
    (ct, tag) = AES256-GCM(msg_key, message_num, plaintext, aad)
    message_num++
    return (message_num, ct, tag)
```

### 5.3 Receiving Message
```
RATCHET_RECV(recv_chain, message_num, ct, tag, aad):
    // Handle out-of-order via skipped message keys cache
    if message_num > expected:
        // Derive skipped keys, store in cache
        for i in expected..message_num-1:
            skipped[i] = HKDF(recv_chain, info="msg", L=32)
            recv_chain = HKDF(recv_chain, info="next", L=32)
    msg_key = skipped[message_num] or HKDF(recv_chain, info="msg", L=32)
    pt = AES256-GCM-Dec(msg_key, message_num, ct, tag, aad)
    if message_num >= expected:
        recv_chain = HKDF(recv_chain, info="next", L=32)
        expected = message_num + 1
    delete skipped[message_num]
    return pt
```

---

## 6. REMOTE ATTESTATION

### 6.1 Quote Generation (ESP32)
```
GENERATE_QUOTE(session_id):
    firmware_hash = SHA256(firmware_image)  // Computed at boot
    quote_data = {
        device_id: my_device_id,
        firmware_hash: firmware_hash,
        session_id: session_id,
        timestamp: now(),
        monotonic_counter: SE_Read_Counter()
    }
    quote_sig = SE_ECDSA_Sign(attestation_sk, SHA256(quote_data))
    return (quote_data, quote_sig)
```

### 6.2 Quote Verification (Contract/Browser)
```
VERIFY_QUOTE(quote_data, quote_sig, device_pk):
    require(Ed25519.Verify(device_pk, SHA256(quote_data), quote_sig))
    require(quote_data.firmware_hash == EXPECTED_FIRMWARE_HASH)
    require(quote_data.timestamp within 60s)
    require(quote_data.monotonic_counter > last_seen_counter)
    update last_seen_counter = quote_data.monotonic_counter
    return true
```

---

## 7. POLICY EVALUATION (ON-CHAIN)

```solidity
function evaluatePolicy(Policy memory p, bytes32 sessionId) internal view returns (bool) {
    // Time window
    if (block.timestamp < p.validFrom || block.timestamp > p.validUntil) {
        return false;
    }
    
    // Retrieval quota
    if (p.retrievalCount >= p.maxRetrievals) {
        return false;
    }
    
    // Geo-fence (verified by ESP32 signature)
    if (p.geoFenceHash != bytes32(0)) {
        Session memory s = sessions[sessionId];
        if (!s.geoVerified) return false;
    }
    
    // Threshold approval
    if (p.requiresThreshold) {
        if (!sessions[sessionId].authorized) return false;
    }
    
    return true;
}
```

---

## 8. TEST VECTORS

### 8.1 HKDF
```
Input:  salt = "file_123"
        ikm  = 0x000102030405060708090a0b0c0d0e0f
        info = "aether:dek"
Output: 0x... (32 bytes)
```

### 8.2 AES256-GCM
```
Key:     0x000102...1f (32 bytes)
Nonce:   0x000102030405060708090a0b (12 bytes)
Plaintext: "Hello Aether"
AAD:     "file_123"
Ciphertext: 0x...
Tag:       0x... (16 bytes)
```

### 8.3 Ed25519
```
Private: 0x... (32 bytes)
Public:  0x... (32 bytes)
Message: "file_123" + metadata
Sig:     0x... (64 bytes)
```

### 8.4 Full Encryption Round-Trip
```
File:     "test_payload.pdf" (1024 bytes)
file_id:  "550e8400-e29b-41d4-a716-446655440000"
MK:       0x... (32 bytes)

DEK = HKDF(MK, "file_id", "aether:dek", 32)
N = random(12)
C, T = AES256-GCM(DEK, N, file, "file_id")
cid_commitment = SHA256(C)
content_commitment = SHA256(file)

Verify: AES256-GCM-Dec(DEK, N, C, T, "file_id") == file
Verify: SHA256(file) == content_commitment
```

---

## 9. IMPLEMENTATION NOTES

### 9.1 Constant-Time Requirements
- All cryptographic comparisons: `constant_time_equal()`
- No early returns on MAC/tag verification
- AES-GCM implemented in constant-time (mbedTLS / Web Crypto)

### 9.2 Memory Zeroization
```c
// C (ESP32)
void zeroize(void *ptr, size_t len) {
    volatile uint8_t *p = (volatile uint8_t*)ptr;
    while (len--) *p++ = 0;
}

// WASM (Browser)
function zeroize(view) {
    for (let i = 0; i < view.length; i++) view[i] = 0;
}
```

### 9.3 Randomness
- ESP32: Hardware RNG (entropy source) → `esp_fill_random()`
- Browser: `crypto.getRandomValues()`
- Host: `os.urandom()` / `secrets.token_bytes()`

---

## 10. PARAMETER REGISTRY

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| AES-GCM nonce | 96 bits | NIST SP 800-38D |
| AES-GCM tag | 128 bits | Maximum security |
| HKDF output | 256 bits | Match AES-256 |
| Session TTL | 300 seconds | Balance usability/security |
| Max attempts | 3 | Throttle brute-force |
| ESP-NOW retries | 3 | Reliability vs latency |
| Threshold | 2-of-3 | Byzantine fault tolerance |
| Monotonic counter | 32-bit | ~4B sessions per device |

---

**END OF CRYPTO SPECIFICATION**