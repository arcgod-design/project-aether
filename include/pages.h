#ifndef SUCCESS_PAGE_H
#define SUCCESS_PAGE_H

// IP_PLACEHOLDER gets replaced at runtime

const char SUCCESS_PAGE[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Project Aether — Connected</title>
<style>
body{font-family:Arial,sans-serif;background:#0b0f14;color:#eef2f6;margin:0;min-height:100vh;display:grid;place-items:center}
.card{width:min(92vw,520px);background:#121923;border:1px solid #2b3948;border-radius:18px;padding:28px;box-sizing:border-box;box-shadow:0 20px 60px #0008}
h1{margin-top:0;font-size:26px}.ok{color:#7de2b2;font-weight:700}
.address{margin:15px 0;padding:14px;background:#0b1118;border-radius:10px;font-family:monospace;word-break:break-all}
button{width:100%;padding:14px;border:0;border-radius:10px;background:#36b37e;color:#07140f;font-weight:700;font-size:16px;cursor:pointer;margin-top:10px}
.small{color:#aeb8c4;font-size:13px;line-height:1.5}
</style>
</head>
<body>
<div class="card">
<h1>Project Aether</h1>
<div class="ok">ESP32 connected to phone hotspot</div>
<p class="small">Now connect your phone back to its own mobile hotspot.<br>Then open one of the addresses below.</p>
<div class="address" id="mdns">http://aether-node.local/</div>
<button onclick="cp('mdns')">COPY ADDRESS</button>
<div class="address" id="ip">IP_PLACEHOLDER</div>
<button onclick="cp('ip')">COPY IP ADDRESS</button>
<p class="small">The IP address is the fallback if <b>aether-node.local</b> does not resolve on your phone.</p>
<script>
function cp(id){
  const t=document.getElementById(id).textContent;
  if(navigator.clipboard){navigator.clipboard.writeText(t).then(()=>alert("Copied:\n"+t)).catch(()=>alert(t));}
  else alert(t);
}
</script>
</div>
</body>
</html>
)rawliteral";

const char ERROR_PAGE[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Aether — Connection Failed</title>
<style>
body{font-family:Arial;background:#0b0f14;color:#fff;min-height:100vh;display:grid;place-items:center}
.card{width:min(90vw,500px);background:#121923;padding:28px;border-radius:18px}
.err{color:#ff8d8d}a{color:#7de2b2}
</style>
</head>
<body>
<div class="card">
<h1>Connection Failed</h1>
<p class="err">ESP32 could not connect to the supplied hotspot.</p>
<p>Check the hotspot name and password and try again.</p>
<p><a href="/setup">Try Again</a></p>
</div>
</body>
</html>
)rawliteral";

#endif
