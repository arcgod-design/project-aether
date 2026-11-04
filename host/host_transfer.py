from __future__ import annotations

import hashlib
import json
import os
import sys
from pathlib import Path

import requests
from dotenv import load_dotenv
from web3 import Web3


PINATA_UPLOAD_URL = "https://api.pinata.cloud/pinning/pinFileToIPFS"
CHAIN_ID = 84532  # Base Sepolia

ABI = [
    {
        "inputs": [
            {"internalType": "bytes32", "name": "pinHash", "type": "bytes32"},
            {"internalType": "string", "name": "cid", "type": "string"},
            {"internalType": "bytes32", "name": "sha256Digest", "type": "bytes32"},
            {"internalType": "string", "name": "fileName", "type": "string"},
            {"internalType": "uint256", "name": "fileSize", "type": "uint256"},
        ],
        "name": "registerFile",
        "outputs": [],
        "stateMutability": "nonpayable",
        "type": "function",
    }
]


def required(name: str) -> str:
    value = os.getenv(name)
    if not value:
        raise RuntimeError(f"Missing {name} in host/.env")
    return value.strip()


def sha256_file(path: Path) -> bytes:
    digest = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.digest()

def upload_to_pinata(path: Path, jwt: str) -> str:
    url = "https://api.pinata.cloud/pinning/pinFileToIPFS"

    headers = {
        "Authorization": f"Bearer {jwt}",
    }

    metadata = json.dumps({
        "name": path.name,
    })

    options = json.dumps({
        "cidVersion": 1,
    })

    with path.open("rb") as f:
        files = {
            "file": (path.name, f, "application/octet-stream"),
        }

        data = {
            "pinataMetadata": metadata,
            "pinataOptions": options,
        }

        response = requests.post(
            url,
            headers=headers,
            files=files,
            data=data,
            timeout=120,
        )

    # Show Pinata's actual response if something goes wrong.
    if not response.ok:
        print("\nPinata response:")
        print(response.text)
        response.raise_for_status()

    result = response.json()

    cid = result.get("IpfsHash")
    if not cid:
        raise RuntimeError(f"Pinata returned no CID: {result}")

    return cid

def register_on_chain(pin: str, cid: str, sha256_digest: bytes, file_name: str, file_size: int) -> str:
    rpc_url = required("BASE_SEPOLIA_RPC_URL")
    contract_address = Web3.to_checksum_address(required("CONTRACT_ADDRESS"))
    wallet_address = Web3.to_checksum_address(required("HOST_WALLET_ADDRESS"))
    private_key = required("HOST_PRIVATE_KEY")

    w3 = Web3(Web3.HTTPProvider(rpc_url))
    if not w3.is_connected():
        raise RuntimeError("Cannot connect to Base Sepolia RPC")

    account = w3.eth.account.from_key(private_key)
    if account.address != wallet_address:
        raise RuntimeError("HOST_WALLET_ADDRESS does not match HOST_PRIVATE_KEY")

    contract = w3.eth.contract(address=contract_address, abi=ABI)

    pin_hash = Web3.keccak(text=pin)
    digest_hex = "0x" + sha256_digest.hex()

    nonce = w3.eth.get_transaction_count(wallet_address)
    gas_price = w3.eth.gas_price

    tx = contract.functions.registerFile(
        pin_hash,
        cid,
        digest_hex,
        file_name,
        file_size,
    ).build_transaction(
        {
            "from": wallet_address,
            "nonce": nonce,
            "chainId": CHAIN_ID,
            "gas": 350_000,
            "maxFeePerGas": gas_price * 2,
            "maxPriorityFeePerGas": w3.to_wei(0.001, "gwei"),
        }
    )

    signed = w3.eth.account.sign_transaction(tx, private_key=private_key)
    tx_hash = w3.eth.send_raw_transaction(signed.raw_transaction)
    receipt = w3.eth.wait_for_transaction_receipt(tx_hash, timeout=180)
    if receipt.status != 1:
        raise RuntimeError("Blockchain transaction failed")

    return tx_hash.hex()


def main() -> int:
    load_dotenv(Path(__file__).with_name(".env"))

    if len(sys.argv) != 2:
        print("Usage: python host_transfer.py <file.zip>")
        return 2

    path = Path(sys.argv[1]).expanduser().resolve()
    if not path.is_file():
        print(f"File not found: {path}")
        return 2
    # Any file type allowed — fileName is recorded on-chain so the
    # receiver's browser can pick the right preview (pdf/image/text).

    pin = input("Enter a 4-digit demo PIN: ").strip()
    if len(pin) != 4 or not pin.isdigit():
        print("PIN must be exactly four digits.")
        return 2

    jwt = required("PINATA_JWT")

    print("1/3 Calculating SHA-256...")
    digest = sha256_file(path)
    print("SHA-256:", digest.hex())

    print("2/3 Uploading ZIP to Pinata IPFS...")
    cid = upload_to_pinata(path, jwt)
    print("CID:", cid)

    print("3/3 Registering CID + SHA-256 + PIN hash on Base Sepolia...")
    tx_hash = register_on_chain(pin, cid, digest, path.name, path.stat().st_size)
    print("Transaction:", tx_hash)
    print("DONE. Save the PIN. The browser can now retrieve the file metadata.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
