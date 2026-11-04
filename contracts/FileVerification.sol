// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

/**
 * Project Aether - class-project proof of concept.
 *
 * The blockchain stores file metadata and an integrity digest.
 * The actual file remains off-chain (IPFS).
 *
 * IMPORTANT: A 4-digit PIN is NOT strong authentication. A public
 * blockchain does not make a PIN secret. This contract is for a demo.
 */
contract FileVerification {
    address public immutable admin;

    struct FileRecord {
        string cid;
        bytes32 sha256Digest;
        string fileName;
        uint256 fileSize;
        uint256 createdAt;
        bool exists;
    }

    mapping(bytes32 => FileRecord) private records;

    event FileRegistered(
        bytes32 indexed pinHash,
        string cid,
        bytes32 sha256Digest,
        string fileName,
        uint256 fileSize,
        uint256 createdAt
    );

    constructor() {
        admin = msg.sender;
    }

    modifier onlyAdmin() {
        require(msg.sender == admin, "Only admin");
        _;
    }

    function registerFile(
        bytes32 pinHash,
        string calldata cid,
        bytes32 sha256Digest,
        string calldata fileName,
        uint256 fileSize
    ) external onlyAdmin {
        require(pinHash != bytes32(0), "Invalid PIN hash");
        require(bytes(cid).length > 0, "CID required");
        require(sha256Digest != bytes32(0), "Hash required");
        require(!records[pinHash].exists, "PIN already registered");

        records[pinHash] = FileRecord({
            cid: cid,
            sha256Digest: sha256Digest,
            fileName: fileName,
            fileSize: fileSize,
            createdAt: block.timestamp,
            exists: true
        });

        emit FileRegistered(pinHash, cid, sha256Digest, fileName, fileSize, block.timestamp);
    }

    function getFile(bytes32 pinHash)
        external
        view
        returns (
            string memory cid,
            bytes32 sha256Digest,
            string memory fileName,
            uint256 fileSize,
            uint256 createdAt,
            bool exists
        )
    {
        FileRecord memory r = records[pinHash];
        require(r.exists, "Invalid PIN");
        return (r.cid, r.sha256Digest, r.fileName, r.fileSize, r.createdAt, r.exists);
    }
}
