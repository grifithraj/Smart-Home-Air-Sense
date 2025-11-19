#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// --------------------
// WiFi Credentials
// --------------------
const char* ssid = "iPhone";
const char* password = "12345678";

// --------------------
// ThingsBoard MQTT
// --------------------
const char* mqtt_server = "mqtt.thingsboard.cloud";
const int   mqtt_port   = 1883;
const char* access_token = "nbzK915r1AU3roBj13RN";

// --------------------
// Sensors & Pins
// --------------------
#define DHTPIN 4
#define DHTTYPE DHT22
#define MQ2PIN 34
#define BUZZER_PIN 25

#define TEMP_THRESHOLD 35
#define GAS_THRESHOLD 1600

DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);

// --------------------
// Connect to WiFi
// --------------------
void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// --------------------
// Send Sensor Data to ThingsBoard
// --------------------
void sendData() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int gasValue = analogRead(MQ2PIN);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("❌ Failed to read from DHT22!");
    return;
  }

  // JSON Payload
  String payload = "{";
  payload += "\"temperature\":" + String(temperature) + ",";
  payload += "\"humidity\":" + String(humidity) + ",";
  payload += "\"gas\":" + String(gasValue);
  payload += "}";

  Serial.println("📤 Sending payload:");
  Serial.println(payload);

  client.publish("v1/devices/me/telemetry", payload.c_str());

  // ---------------------------
  // BUZZER ALERT BASED ON VALUE
  // ---------------------------
  if (temperature > TEMP_THRESHOLD || gasValue > GAS_THRESHOLD) {
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("🚨 ALERT! Threshold exceeded! Buzzer ON");
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// --------------------
// MQTT Reconnect
// --------------------
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to ThingsBoard...");

    if (client.connect("ESP32_Device", access_token, "")) {
      Serial.println("Connected!");
    } else {
      Serial.print("Failed! rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

// --------------------
// Setup
// --------------------
void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  dht.begin();
  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
}

// --------------------
// Loop
// --------------------
void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();
  sendData();

  delay(2000); // send every 2 seconds
}
