#ifndef LOGIN_PAGE_H
#define LOGIN_PAGE_H

const char LOGIN_PAGE[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Project Aether — Secure Retrieval</title>
<script src="https://cdnjs.cloudflare.com/ajax/libs/ethers/5.7.2/ethers.umd.min.js"></script>
<style>
body{font-family:Arial,sans-serif;background:#0b0f14;color:#eef2f6;margin:0;min-height:100vh;display:grid;place-items:center;padding:20px}
.card{width:min(92vw,480px);background:#121923;border:1px solid #2b3948;border-radius:18px;padding:28px;box-sizing:border-box;box-shadow:0 20px 60px #0008}
h1{margin-top:0;font-size:24px}p{color:#aeb8c4;line-height:1.5;margin-bottom:20px}
label{display:block;font-size:13px;margin-bottom:7px;color:#94a3b2}
input{width:100%;box-sizing:border-box;padding:13px;border-radius:10px;border:1px solid #3a4654;background:#0d131b;color:white;font-size:16px}
button{width:100%;padding:14px;border:0;border-radius:10px;background:#36b37e;color:#07140f;font-weight:700;font-size:16px;cursor:pointer}
button:disabled{background:#2b3948;color:#7e8b98;cursor:not-allowed}
button+button{margin-top:8px}
.status{margin-top:18px;padding:12px;border-radius:10px;background:#0b1118;white-space:pre-wrap;min-height:72px;font-family:monospace;font-size:13px}
.ok{color:#7de2b2}.err{color:#ff8d8d}
.small{font-size:12px;color:#7e8b98}
.debug{margin-top:12px;padding:10px;border-radius:8px;background:#0a0f15;color:#7e8b98;font-size:11px;word-break:break-all;max-height:120px;overflow:auto}
.spinner{border:3px solid #2b3948;border-top:3px solid #36b37e;border-radius:50%;width:24px;height:24px;animation:spin 1s linear infinite;margin:0 auto 12px}
@keyframes spin{0%{transform:rotate(0deg)}100%{transform:rotate(360deg)}}
.fmeta{font-size:11px;color:#7e8b98;margin-top:4px}
#previewArea{margin-top:16px;padding:12px;background:#0b1118;border-radius:10px;max-height:60vh;overflow:auto;display:none}
#previewArea img{max-width:100%}
#previewArea iframe{width:100%;height:500px;border:none}
</style>
</head>
<body>
<div class="card">
<h1>Project Aether — Secure Retrieval</h1>
<p>Enter the 4-digit PIN for the file assigned to you.</p>
<div class="row"><label>4-digit Access PIN</label><input id="pin" inputmode="numeric" maxlength="4" placeholder="4827" autocomplete="off"></div>
<button id="go" disabled>VERIFY & RETRIEVE</button>
<button id="dlBtn" class="btn-sec" style="display:none;background:#2b3948;color:#eef2f6">DOWNLOAD FILE</button>
<div id="status" class="status">Enter PIN to begin</div>
<div id="previewArea"></div>
<div id="debug" class="debug">System ready</div>
<p class="small">Integrity: SHA-256 verified against the blockchain record. No app installed; nothing persists on this device.</p>
</div>
<script>
const RPC_URL = "https://base-sepolia.g.alchemy.com/v2/alch_wE4l5QcffNreB_ZahJm1m";
const CONTRACT_ADDRESS = "0xA3FFBC19730222a2C0a85e65791B890B4ecd6C26";
const ABI = ["function getFile(bytes32 pinHash) view returns (string cid, bytes32 sha256Digest, string fileName, uint256 fileSize, uint256 createdAt, bool exists)"];
const GATEWAYS = ["https://gateway.pinata.cloud/ipfs/","https://dweb.link/ipfs/","https://ipfs.io/ipfs/","https://cloudflare-ipfs.com/ipfs/"];

const $ = (id) => document.getElementById(id);
const log = (msg, ok) => { const el = $("status"); el.textContent = msg; el.className = "status" + (ok ? " ok" : ""); };
const debug = (msg) => { $("debug").textContent = msg; };

let fileBlob = null;
let fileName = "";

// ---------- Pure-JS SHA-256 (works on HTTP, no crypto.subtle) ----------
function sha256Hex(buf) {
  const K = [
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2];
  function rr(v, n) { return (v >>> n) | (v << (32 - n)); }
  const H = new Int32Array([0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19]);
  const bytes = new Uint8Array(buf);
  const len = bytes.length;
  const bitLen = len * 8;
  const withPad = ((len + 9 + 63) >> 6) << 6;
  const words = new Int32Array(withPad >> 2);
  for (let i = 0; i < len; i++) words[i >> 2] |= bytes[i] << ((3 - (i & 3)) << 3);
  words[len >> 2] |= 0x80 << ((3 - (len & 3)) << 3);
  words[(withPad >> 2) - 1] = bitLen >>> 0;
  words[(withPad >> 2) - 2] = Math.floor(bitLen / 0x100000000);
  const w = new Int32Array(64);
  for (let off = 0; off < words.length; off += 16) {
    for (let i = 0; i < 16; i++) w[i] = words[off + i];
    for (let i = 16; i < 64; i++) {
      const s0 = rr(w[i-15],7) ^ rr(w[i-15],18) ^ (w[i-15] >>> 3);
      const s1 = rr(w[i-2],17) ^ rr(w[i-2],19) ^ (w[i-2] >>> 10);
      w[i] = (w[i-16] + s0 + w[i-7] + s1) | 0;
    }
    let a=H[0],b=H[1],c=H[2],d=H[3],e=H[4],f=H[5],g=H[6],h=H[7];
    for (let i = 0; i < 64; i++) {
      const S1 = rr(e,6) ^ rr(e,11) ^ rr(e,25);
      const ch = (e & f) ^ (~e & g);
      const t1 = (h + S1 + ch + K[i] + w[i]) | 0;
      const S0 = rr(a,2) ^ rr(a,13) ^ rr(a,22);
      const maj = (a & b) ^ (a & c) ^ (b & c);
      const t2 = (S0 + maj) | 0;
      h=g; g=f; f=e; e=(d + t1)|0; d=c; c=b; b=a; a=(t1 + t2)|0;
    }
    H[0]=(H[0]+a)|0; H[1]=(H[1]+b)|0; H[2]=(H[2]+c)|0; H[3]=(H[3]+d)|0;
    H[4]=(H[4]+e)|0; H[5]=(H[5]+f)|0; H[6]=(H[6]+g)|0; H[7]=(H[7]+h)|0;
  }
  let hex = "";
  for (let i = 0; i < 8; i++) {
    hex += (H[i] >>> 0).toString(16).padStart(8, "0");
  }
  return "0x" + hex;
}

// ---------- PIN input ----------
$("pin").addEventListener("input", function () {
    $("go").disabled = !/^\d{4}$/.test(this.value);
});

// ---------- Main flow ----------
$("go").addEventListener("click", async () => {
    const pin = $("pin").value.trim();
    if (!/^\d{4}$/.test(pin)) { log("Enter exactly 4 digits."); return; }

    $("go").disabled = true;
    $("go").style.display = "none";
    $("dlBtn").style.display = "none";
    $("previewArea").style.display = "none";

    try {
        // 1. Session on ESP32 (nonce + TTL, replay protection)
        log("1/4 Creating session…");
        const res = await fetch("/api/session", {
            method: "POST",
            headers: { "Content-Type": "application/x-www-form-urlencoded" },
            body: "pin=" + encodeURIComponent(pin)
        });
        if (!res.ok) throw new Error("Session failed: HTTP " + res.status);
        const data = await res.json();
        debug("Nonce: " + data.session_nonce);

        // 2. Blockchain lookup: keccak256(PIN) — matches host registration
        log("2/4 Verifying PIN on Base Sepolia…");
        const pinHash = ethers.utils.keccak256(ethers.utils.toUtf8Bytes(pin));
        debug("PIN hash: " + pinHash);
        const provider = new ethers.providers.JsonRpcProvider(RPC_URL);
        const contract = new ethers.Contract(CONTRACT_ADDRESS, ABI, provider);
        const result = await contract.getFile(pinHash);
        if (!result.exists) throw new Error("PIN not recognised — no file assigned to this PIN");

        fileName = result.fileName;
        debug("File: " + fileName + " | CID: " + result.cid);

        // 3. Fetch from IPFS (multi-gateway)
        log("3/4 Fetching file from IPFS…");
        let lastErr = null;
        fileBlob = null;
        for (const base of GATEWAYS) {
            log("Gateway: " + base);
            try {
                const r = await fetch(base + result.cid);
                if (r.ok) { fileBlob = await r.blob(); break; }
                lastErr = "HTTP " + r.status;
            } catch (e) { lastErr = e.message; }
        }
        if (!fileBlob) throw new Error("All IPFS gateways failed: " + lastErr);

        // 4. SHA-256 integrity vs on-chain record
        log("4/4 Verifying SHA-256 integrity…");
        const buf = await fileBlob.arrayBuffer();
        const actual = sha256Hex(buf);
        debug("On-chain: " + result.sha256Digest + "\nActual:   " + actual);
        if (actual.toLowerCase() !== result.sha256Digest.toLowerCase()) {
            fileBlob = null;
            throw new Error("INTEGRITY CHECK FAILED — file tampered");
        }

        renderPreview();
        log("VERIFIED — " + fileName + "\nSHA-256 matches blockchain.", true);
        $("dlBtn").style.display = "block";

    } catch (e) {
        console.error(e);
        fileBlob = null;
        $("go").disabled = false;
        $("go").style.display = "block";
        log("FAILED\n" + (e.reason || e.message || String(e)));
    }
});

// ---------- Preview ----------
function renderPreview() {
    const area = $("previewArea");
    const url = URL.createObjectURL(fileBlob);
    area.style.display = "block";
    const n = fileName.toLowerCase();
    if (/\.(png|jpe?g|gif|webp|bmp)$/.test(n)) {
        area.innerHTML = "<strong>" + fileName + "</strong><br><img src='" + url + "'>";
    } else if (/\.pdf$/.test(n)) {
        area.innerHTML = "<strong>" + fileName + "</strong><br><iframe src='" + url + "'></iframe>";
    } else if (/\.(txt|md|csv|json|log|c|cpp|py|js|sol)$/.test(n)) {
        fileBlob.text().then(t => {
            area.innerHTML = "<strong>" + fileName + "</strong><br><pre style='white-space:pre-wrap'>" + t + "</pre>";
        });
    } else {
        area.innerHTML = "<strong>" + fileName + "</strong><br><span class='fmeta'>Binary file — use Download.</span>";
    }
}

// ---------- Download ----------
$("dlBtn").addEventListener("click", () => {
    if (!fileBlob) return;
    const url = URL.createObjectURL(fileBlob);
    const a = document.createElement("a");
    a.href = url; a.download = fileName;
    document.body.appendChild(a); a.click(); a.remove();
    log("Downloaded: " + fileName, true);
});

// ---------- Diagnostics ----------
setTimeout(() => {
    debug(typeof ethers === "undefined"
        ? "ethers.js NOT loaded — check phone Internet."
        : "ethers.js loaded. Pure-JS SHA-256 active (works on HTTP).");
}, 1000);
</script>
</body>
</html>
)rawliteral";

#endif
