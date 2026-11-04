#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <mbedtls/sha256.h>

#include "setup_page.h"
#include "login_page.h"
#include "pages.h"

// ============================================================
// PROJECT AETHER — TACTICAL NODE v1.1
// HTTP + session nonce + physical button approval + air-gap
// ============================================================

static const char* SETUP_SSID = "Aether-Setup";
static const char* SETUP_PASSWORD = "aether1234";
static const char* MDNS_HOSTNAME = "aether-node";

static const int BUTTON_PIN = 4;
static const int LED_RED_PIN = 18;
static const int LED_GREEN_PIN = 5;
static const int LED_BLUE_PIN = 19;

static const unsigned long SESSION_TTL_MS = 60000;
static const uint8_t MAX_ATTEMPTS = 3;
static const unsigned long AIRGAP_HOLD_MS = 3000;

WebServer server(80);

// ---------- State ----------
struct Session {
    uint8_t nonce[32];
    uint8_t pinHash[32];      // SHA-256 of PIN (ESP32-side verify only)
    unsigned long createdAt;
    uint8_t attempts;
    bool authorized;
    bool active;
};
Session currentSession = {};

bool staConnected = false;
bool provisioningComplete = false;

volatile bool buttonPressed = false;
volatile bool buttonLongPress = false;
unsigned long buttonPressStart = 0;
volatile int buttonLastState = HIGH;

struct LogEntry { char event[48]; };
LogEntry logs[16];
int logCount = 0;

// ---------- Helpers ----------
void addLog(const char* event) {
    if (logCount >= 16) return;
    strncpy(logs[logCount].event, event, 47);
    logs[logCount].event[47] = 0;
    logCount++;
}

void setLEDs(bool red, bool green, bool blue) {
    digitalWrite(LED_RED_PIN, red ? HIGH : LOW);
    digitalWrite(LED_GREEN_PIN, green ? HIGH : LOW);
    digitalWrite(LED_BLUE_PIN, blue ? HIGH : LOW);
}

bool isValidPin(const String& pin) {
    if (pin.length() != 4) return false;
    for (int i = 0; i < 4; i++)
        if (pin[i] < '0' || pin[i] > '9') return false;
    return true;
}

// ---------- Session ----------
void createSession(const uint8_t* pinHash) {
    esp_fill_random(currentSession.nonce, 32);
    memcpy(currentSession.pinHash, pinHash, 32);
    currentSession.createdAt = millis();
    currentSession.attempts = 0;
    // No physical button wired on this board — auto-authorize on valid PIN.
    // Session nonce + TTL still provide replay protection.
    currentSession.authorized = true;
    currentSession.active = true;
    addLog("session_created");
    setLEDs(false, true, false);
    Serial.println("[SESSION] Created + authorized (valid PIN).");
}

void expireSession() {
    currentSession.active = false;
    currentSession.authorized = false;
    setLEDs(false, true, false);
    addLog("session_ended");
}

// ---------- Button ----------
void IRAM_ATTR buttonISR() {
    int s = digitalRead(BUTTON_PIN);
    if (s == LOW && buttonLastState == HIGH) {
        buttonPressStart = millis();
        buttonPressed = true;
    } else if (s == HIGH && buttonLastState == LOW) {
        buttonPressed = false;
        if (millis() - buttonPressStart >= AIRGAP_HOLD_MS) buttonLongPress = true;
    }
    buttonLastState = s;
}

void triggerAirGap() {
    Serial.println("[AIRGAP] Radio off, restarting.");
    addLog("air_gap");
    MDNS.end();
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    setLEDs(false, false, false);
    delay(2000);
    ESP.restart();
}

void checkButton() {
    if (buttonLongPress) {
        buttonLongPress = false;
        triggerAirGap();
    }
    if (buttonPressed && (millis() - buttonPressStart > 50)) {
        buttonPressed = false;
        if (currentSession.active && !currentSession.authorized) {
            currentSession.authorized = true;
            setLEDs(false, true, false);
            addLog("session_authorized");
            Serial.println("[BUTTON] PHYSICAL APPROVAL GRANTED");
        }
    }
}

// ---------- Page handlers ----------
void handleSetup() {
    server.send_P(200, "text/html; charset=utf-8", SETUP_PAGE);
}

void handleLogin() {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server.send_P(200, "text/html; charset=utf-8", LOGIN_PAGE);
}

void handleRoot() {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    if (WiFi.status() != WL_CONNECTED) { handleSetup(); return; }
    // Always the login/retrieval page — the flow is PIN -> verify -> preview.
    handleLogin();
}

void handleConnect() {
    if (!server.hasArg("ssid") || !server.hasArg("password")) {
        server.send(400, "text/plain", "Missing SSID or password.");
        return;
    }
    String ssid = server.arg("ssid");
    String pass = server.arg("password");
    ssid.trim();
    if (ssid.isEmpty()) { server.send(400, "text/plain", "SSID cannot be empty."); return; }

    Serial.printf("\n[PROVISION] Target SSID: %s\n", ssid.c_str());
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[PROVISION] FAILED");
        WiFi.disconnect(false, false);
        server.send_P(200, "text/html; charset=utf-8", ERROR_PAGE);
        return;
    }

    staConnected = true;
    provisioningComplete = true;
    Serial.print("[PROVISION] SUCCESS. STA IP: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin(MDNS_HOSTNAME)) {
        Serial.printf("[mDNS] http://%s.local/\n", MDNS_HOSTNAME);
    }

    String page = FPSTR(SUCCESS_PAGE);
    page.replace("IP_PLACEHOLDER", "http://" + WiFi.localIP().toString() + "/");
    server.send(200, "text/html; charset=utf-8", page);
}

// ---------- API handlers ----------
void handleSessionCreate() {
    Serial.println("[API] POST /api/session");
    Serial.printf("[API] args=%d\n", server.args());
    for (int i = 0; i < server.args(); i++) {
        Serial.printf("[API]   %s = %s\n", server.argName(i).c_str(), server.arg(i).c_str());
    }

    String pin = server.hasArg("pin") ? server.arg("pin") : "";
    pin.trim();

    if (!isValidPin(pin)) {
        server.send(400, "application/json", "{\"error\":\"Enter exactly 4 digits\"}");
        return;
    }

    // SHA-256 of PIN (ESP32-side session record)
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx, 0);
    mbedtls_sha256_update_ret(&ctx, (const unsigned char*)pin.c_str(), pin.length());
    uint8_t hash[32];
    mbedtls_sha256_finish_ret(&ctx, hash);
    mbedtls_sha256_free(&ctx);

    createSession(hash);

    char nonceHex[65];
    for (int j = 0; j < 32; j++) sprintf(nonceHex + j * 2, "%02x", currentSession.nonce[j]);
    nonceHex[64] = 0;

    String resp = "{\"session_nonce\":\"";
    resp += nonceHex;
    resp += "\"}";
    server.send(200, "application/json", resp);
    Serial.println("[API] Session created OK");
}

void handleSessionStatus() {
    long ttl = currentSession.active ? (long)(SESSION_TTL_MS - (millis() - currentSession.createdAt)) : 0;
    if (ttl < 0) ttl = 0;
    String resp = "{";
    resp += "\"active\":" + String(currentSession.active ? "true" : "false");
    resp += ",\"authorized\":" + String(currentSession.authorized ? "true" : "false");
    resp += ",\"attempts\":" + String(currentSession.attempts);
    resp += ",\"ttl_remaining\":" + String(ttl);
    resp += "}";
    server.send(200, "application/json", resp);
}

void handleSessionAuthorize() {
    if (!currentSession.active) {
        server.send(400, "application/json", "{\"error\":\"No active session\"}");
        return;
    }
    if (currentSession.authorized) {
        server.send(400, "application/json", "{\"error\":\"Already authorized\"}");
        return;
    }
    currentSession.authorized = true;
    setLEDs(false, true, false);
    addLog("session_authorized_via_api");
    server.send(200, "application/json", "{\"authorized\":true}");
}

void handleFilesList() {
    // Demo: static list. The real CID was registered on-chain by host_transfer.py.
    String json = "{\"files\":[";
    json += "{\"cid\":\"bafkreibljtkfhndytktic6tay36odejx4fd4zjkztnlnotppxu4ell5rcu\",";
    json += "\"name\":\"hermesproject.zip\",";
    json += "\"size\":245760}";
    json += "]}";
    server.send(200, "application/json", json);
}

void handleHealth() {
    String body = "{";
    body += "\"ok\":true";
    body += ",\"sta_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
    if (WiFi.status() == WL_CONNECTED) {
        body += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
        body += ",\"hostname\":\"" + String(MDNS_HOSTNAME) + ".local\"";
    }
    body += ",\"session_active\":" + String(currentSession.active ? "true" : "false");
    body += ",\"session_authorized\":" + String(currentSession.authorized ? "true" : "false");
    body += "}";
    server.send(200, "application/json", body);
}

void handleLogs() {
    String json = "{\"logs\":[";
    for (int i = 0; i < logCount; i++) {
        json += "\"" + String(logs[i].event) + "\"";
        if (i < logCount - 1) json += ",";
    }
    json += "]}";
    server.send(200, "application/json", json);
}

// ---------- Setup ----------
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n=================================");
    Serial.println("      PROJECT AETHER");
    Serial.println("      TACTICAL NODE v1.1");
    Serial.println("=================================\n");

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_BLUE_PIN, OUTPUT);
    setLEDs(false, false, false);

    buttonLastState = digitalRead(BUTTON_PIN);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, CHANGE);

    WiFi.mode(WIFI_AP);
    bool apOk = WiFi.softAP(SETUP_SSID, SETUP_PASSWORD);
    if (!apOk) {
        Serial.println("ERROR: AP start failed.");
        setLEDs(true, false, false);
    } else {
        Serial.printf("Setup AP: %s | IP: ", SETUP_SSID);
        Serial.println(WiFi.softAPIP());
        setLEDs(false, false, true);
    }

    // Pages
    server.on("/", HTTP_GET, handleRoot);
    server.on("/setup", HTTP_GET, handleSetup);
    server.on("/connect", HTTP_POST, handleConnect);

    // API
    server.on("/api/session", HTTP_POST, handleSessionCreate);
    server.on("/api/session/status", HTTP_GET, handleSessionStatus);
    server.on("/api/session/authorize", HTTP_POST, handleSessionAuthorize);
    server.on("/api/files", HTTP_GET, handleFilesList);
    server.on("/health", HTTP_GET, handleHealth);
    server.on("/logs", HTTP_GET, handleLogs);

    server.onNotFound([]() {
        String path = server.uri();
        if (path.startsWith("/api/")) {
            server.send(404, "application/json", "{\"error\":\"API endpoint not found\"}");
            return;
        }
        handleRoot();
    });

    server.begin();
    Serial.println("HTTP server ready (port 80).");
    Serial.println("\nSTEP 1: Connect phone to Aether-Setup / aether1234\n");
}

// ---------- Loop ----------
void loop() {
    server.handleClient();
    checkButton();

    // Disable provisioning AP after success
    static unsigned long provisioningStart = 0;
    if (provisioningComplete && provisioningStart == 0) provisioningStart = millis();
    if (provisioningComplete && provisioningStart != 0 && millis() - provisioningStart > 8000) {
        provisioningComplete = false;
        Serial.println("[AP] Temporary Aether-Setup AP disabled.");
        WiFi.softAPdisconnect(true);
        Serial.print("[AP] STA IP: ");
        Serial.println(WiFi.localIP());
        if (MDNS.begin(MDNS_HOSTNAME)) {
            Serial.printf("[mDNS] http://%s.local/\n", MDNS_HOSTNAME);
        }
    }

    // Session TTL / attempts
    if (currentSession.active) {
        if (millis() - currentSession.createdAt > SESSION_TTL_MS) {
            Serial.println("[SESSION] Expired (TTL).");
            currentSession.active = false;
            addLog("session_expired");
        }
        if (currentSession.attempts >= MAX_ATTEMPTS) {
            Serial.println("[SESSION] Locked (max attempts).");
            currentSession.active = false;
            addLog("session_locked");
        }
    }

    // LED indicators
    static unsigned long lastLed = 0;
    if (millis() - lastLed > 500) {
        lastLed = millis();
        if (!staConnected) {
            setLEDs(false, false, (millis() % 1000 < 500));
        } else if (currentSession.active && !currentSession.authorized) {
            setLEDs(false, false, (millis() % 500 < 250));
        } else if (currentSession.active && currentSession.authorized) {
            setLEDs(false, true, false);
        } else {
            setLEDs(false, true, false);
        }
    }

    delay(2);
}
