# Blockchain-Assisted Secure Medical Image Sharing Using Lightweight Cryptography and Dynamic Key Management

## Abstract
This project demonstrates a proof-of-concept system for secure medical image sharing using a Laptop (or Raspberry Pi), two ESP32 boards, an SD card, lightweight cryptography, and blockchain.

Unlike a full hospital deployment, the ESP32 is **not** responsible for storing or processing medical images. Instead, it acts as a secure hardware interface that selects which encrypted image folder should be copied to an SD card. The laptop performs encryption, blockchain interaction, and file management.

---

# Objectives

- Secure medical image storage
- Blockchain-based integrity verification
- Folder selection using ESP32 push buttons
- Copy encrypted folders to an SD card
- Verify file integrity before transfer

---

# Hardware Required

- Laptop or Raspberry Pi
- 2 × ESP32
- SD Card Module
- Push Buttons
- Micro SD Card
- Wi-Fi or USB Serial

---

# Software Required

- Arduino IDE
- Python
- Flask (optional)
- SHA-256
- Lightweight Encryption (Ascon/AES)
- Local Blockchain (Ethereum Testnet / Hyperledger / Ganache)

---

# System Architecture

```
Medical Image
      │
      ▼
Laptop / Raspberry Pi
├── Encrypt Image
├── Store Image in Folder
├── Generate SHA-256 Hash
├── Store Hash in Blockchain
└── Connect to ESP32

        │
        ▼
ESP32
(Push Buttons)

Button Press Count
        │
        ▼
Laptop selects corresponding folder
        │
        ▼
Folder copied to SD Card
        │
        ▼
Receiver verifies blockchain hash
```

---

# Working Principle

## Sender Side

1. Upload the medical image.
2. Encrypt the image.
3. Save it in a numbered folder.
4. Generate the SHA-256 hash.
5. Store only the hash and metadata on the blockchain.

## ESP32 Operation

- Push buttons are connected to the ESP32.
- The ESP32 counts button presses.
- Example:
  - Press once → Folder 1
  - Press twice → Folder 2
  - Press four times → Folder 4
- The ESP32 sends the selected folder number to the laptop.

## Laptop Operation

- Receives the folder number.
- Finds the corresponding folder.
- Copies that encrypted folder to the SD card.
- Before copying, verifies the hash against the blockchain.
- If valid, the copy proceeds.

## Receiver Side

- Reads the encrypted folder from the SD card.
- Verifies the file hash with the blockchain.
- Displays:
  - Verified
  - Tampered

---

# Blockchain Role

The blockchain stores only:

- SHA-256 hash
- Timestamp
- Patient ID
- Image ID
- Access log

**The image or folder itself is NOT stored on the blockchain.**

---

# ESP32 Role

The ESP32 only:

- Counts button presses
- Sends folder selection
- Acts as a secure hardware interface

It does **not** process medical images or run blockchain software.

---

# Advantages

- Low-cost implementation
- Secure image verification
- Lightweight IoT design
- Simple hardware
- Easy to demonstrate

---

# Limitations

- Blockchain runs on the laptop, not the ESP32.
- ESP32 cannot process large medical images.
- Prototype intended for demonstration purposes.

---

# Applications

- Hospitals
- Medical laboratories
- Diagnostic centers
- Secure healthcare data transfer

---

# Future Scope

- Fingerprint authentication
- RFID-based access
- Cloud integration
- ESP32-CAM support
- Automated audit logs

---

# Conclusion

This prototype demonstrates a practical approach to secure medical image sharing. The laptop performs encryption, blockchain verification, and storage, while the ESP32 provides a simple and reliable hardware interface for selecting image folders using push buttons. The selected encrypted folder is copied to an SD card only after successful blockchain verification, ensuring data integrity and security.

