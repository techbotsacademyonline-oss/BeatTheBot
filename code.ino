#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// Wi-Fi
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Servo pins
constexpr int SERVO_PINS[4] = {13, 14, 25, 26};

// Servo limits (calibrate to your 4-DOF arm)
constexpr int SERVO_MIN_DEG[4] = {60, 60, 65, 55};
constexpr int SERVO_MAX_DEG[4] = {120, 120, 115, 125};
constexpr int START_DEG[4] = {90, 90, 90, 90};

constexpr int SERVO_MIN_US = 500;
constexpr int SERVO_MAX_US = 2500;
constexpr unsigned long SERVO_UPDATE_MS = 20;
constexpr unsigned long COMMAND_TIMEOUT_MS = 1500;

WebServer server(80);
Servo servos[4];

int currentDeg[4] = {90, 90, 90, 90};
int targetDeg[4] = {90, 90, 90, 90};

unsigned long lastCommandMs = 0;
unsigned long lastServoUpdateMs = 0;
bool hasCommand = false;

float clamp01(float value) {
  if (value < 0.0f) return 0.0f;
  if (value > 1.0f) return 1.0f;
  return value;
}

int mapNormalized(float value, int low, int high) {
  value = clamp01(value);
  return low + (int)((high - low) * value + 0.5f);
}

void addCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Cache-Control", "no-store");
}

void sendJson(int statusCode, const String& body) {
  addCorsHeaders();
  server.send(statusCode, "application/json", body);
}

void handleHealth() {
  sendJson(200, "{\"ok\":true,\"device\":\"BeatTheBot ESP32\"}");
}

void handleOptions() {
  addCorsHeaders();
  server.send(204);
}

void handleArm() {
  if (!server.hasArg("plain")) {
    sendJson(400, "{\"ok\":false,\"error\":\"Missing JSON body\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    sendJson(400, "{\"ok\":false,\"error\":\"Invalid JSON\"}");
    return;
  }

  if (!doc["paddleY"].is<float>() ||
      !doc["ballX"].is<float>() ||
      !doc["ballY"].is<float>()) {
    sendJson(400, "{\"ok\":false,\"error\":\"Expected paddleY, ballX, ballY\"}");
    return;
  }

  float paddleY = clamp01(doc["paddleY"].as<float>());
  float ballX = clamp01(doc["ballX"].as<float>());
  float ballY = clamp01(doc["ballY"].as<float>());

  // Map to 4 DOF arm joints
  targetDeg[0] = mapNormalized(ballX, SERVO_MIN_DEG[0], SERVO_MAX_DEG[0]);
  targetDeg[1] = mapNormalized(paddleY, SERVO_MIN_DEG[1], SERVO_MAX_DEG[1]);
  targetDeg[2] = mapNormalized(ballY, SERVO_MIN_DEG[2], SERVO_MAX_DEG[2]);
  targetDeg[3] = mapNormalized(1.0f - paddleY, SERVO_MIN_DEG[3], SERVO_MAX_DEG[3]);

  lastCommandMs = millis();
  hasCommand = true;

  sendJson(200, "{\"ok\":true}");
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("ESP32 IP: http://");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  connectWiFi();

  for (int i = 0; i < 4; i++) {
    currentDeg[i] = START_DEG[i];
    targetDeg[i] = START_DEG[i];
    servos[i].setPeriodHertz(50);
    servos[i].attach(SERVO_PINS[i], SERVO_MIN_US, SERVO_MAX_US);
    servos[i].write(currentDeg[i]);
  }

  server.on("/health", HTTP_GET, handleHealth);
  server.on("/health", HTTP_OPTIONS, handleOptions);
  server.on("/arm", HTTP_POST, handleArm);
  server.on("/arm", HTTP_OPTIONS, handleOptions);

  server.onNotFound([]() {
    sendJson(404, "{\"ok\":false,\"error\":\"Not found\"}");
  });

  server.begin();
  Serial.println("HTTP server ready");
}

void loop() {
  server.handleClient();

  if (hasCommand && millis() - lastCommandMs > COMMAND_TIMEOUT_MS) {
    hasCommand = false;
  }

  if (millis() - lastServoUpdateMs >= SERVO_UPDATE_MS) {
    lastServoUpdateMs = millis();

    for (int i = 0; i < 4; i++) {
      if (currentDeg[i] < targetDeg[i]) currentDeg[i]++;
      else if (currentDeg[i] > targetDeg[i]) currentDeg[i]--;

      currentDeg[i] = constrain(currentDeg[i], SERVO_MIN_DEG[i], SERVO_MAX_DEG[i]);
      servos[i].write(currentDeg[i]);
    }
  }
}
