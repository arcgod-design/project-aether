#ifndef SETUP_PAGE_H
#define SETUP_PAGE_H

const char SETUP_PAGE[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Project Aether — Network Setup</title>
<style>
body{font-family:Arial,sans-serif;background:#0b0f14;color:#eef2f6;margin:0;min-height:100vh;display:grid;place-items:center}
.card{width:min(92vw,520px);background:#121923;border:1px solid #2b3948;border-radius:18px;padding:28px;box-sizing:border-box;box-shadow:0 20px 60px #0008}
h1{margin-top:0;font-size:26px}p{color:#aeb8c4;line-height:1.5}
label{display:block;font-size:13px;margin-bottom:7px;color:#94a3b2}
input{width:100%;box-sizing:border-box;padding:13px;margin-bottom:16px;border-radius:10px;border:1px solid #3a4654;background:#0d131b;color:white;font-size:16px}
button{width:100%;padding:14px;border:0;border-radius:10px;background:#36b37e;color:#07140f;font-weight:700;font-size:16px;cursor:pointer}
.note{margin-top:16px;padding:12px;border-radius:10px;background:#0b1118;color:#aeb8c4;font-size:13px}
</style>
</head>
<body>
<div class="card">
<h1>Project Aether — Network Setup</h1>
<p>Connect the ESP32 to your phone's mobile hotspot.<br>The phone hotspot provides Internet access to the ESP32.</p>
<form method="POST" action="/connect">
<label>Mobile hotspot name (SSID)</label>
<input name="ssid" type="text" autocomplete="off" required placeholder="MyPhone">
<label>Mobile hotspot password</label>
<input name="password" type="password" autocomplete="off" required placeholder="Hotspot password">
<button type="submit">CONNECT ESP32</button>
</form>
<div class="note">After the ESP32 connects successfully, this setup Wi-Fi will disappear.<br>Connect your phone back to its own mobile hotspot and open the address shown by the ESP32.</div>
</div>
</body>
</html>
)rawliteral";

#endif
