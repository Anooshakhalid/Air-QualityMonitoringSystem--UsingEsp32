// OTA DRIVE IMPLEMENTED
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <DHT.h>
#include <WiFiManager.h>
#include "ThingSpeak.h"

// Pin Definitions
const int LED_PIN = 4;
const int DUST_PIN = 34;
const int LDR_PIN = 36;
const int DHT_PIN = 21;

// ThingSpeak Configuration
const unsigned long CHANNEL_ID = 2934087;
const char *WRITE_API_KEY = "3Y33NLA4BOVUE561";

WiFiClient client;

#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

const char* otaDriveDeviceKey = "bbf16015-f8b4-4c73-97ea-e84065712c14"; // your OTA Drive key
const char* firmwareVersion = "1.0.0.3"; // updated firmware version

// Timing Variables
unsigned long lastOtaCheck = 0;
unsigned long otaCheckInterval = 3600000; // 1 hour

unsigned long lastSensorRead = 0;
const unsigned long sensorReadInterval = 5000; // 5 seconds

unsigned long lastThingSpeakUpdate = 0;
const unsigned long thingSpeakInterval = 300000; // 5 minutes

// Last sensor values
float lastDustDensity = 0;
float lastLdrVoltage = 0;
float lastTemperature = 0;
float lastHumidity = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // WiFiManager: Connect or start AP
  WiFiManager wifiManager;
  wifiManager.autoConnect("ESP32_Setup");
  Serial.println("WiFi connected: " + WiFi.localIP().toString());

  ThingSpeak.begin(client);  // Initialize ThingSpeak
  dht.begin();

  Serial.println("System ready. OTA Drive and ThingSpeak active.");
}

void loop() {
  unsigned long currentMillis = millis();

  // --- Check for OTA Drive Updates ---
  if (currentMillis - lastOtaCheck > otaCheckInterval) {
    lastOtaCheck = currentMillis;
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Checking for OTA Drive update...");
      updateOTADrive();
    } else {
      Serial.println("Wi-Fi not connected. Skipping OTA update check.");
    }
  }

  // --- Read Sensors every 5 seconds ---
  if (currentMillis - lastSensorRead > sensorReadInterval) {
    lastSensorRead = currentMillis;

    // --- Dust Sensor Reading ---
    digitalWrite(LED_PIN, LOW);
    delayMicroseconds(280);
    int dustRaw = analogRead(DUST_PIN);
    delayMicroseconds(40);
    digitalWrite(LED_PIN, HIGH);
    delayMicroseconds(9680);

    float dustVoltage = dustRaw * (3.3 / 4095.0);
    float dustDensity = (dustVoltage - 0.6) * 1000.0 / 0.5;
    if (dustDensity < 0) dustDensity = 0;

    // --- LDR Reading ---
    int ldrRaw = analogRead(LDR_PIN);
    float ldrVoltage = ldrRaw * (3.3 / 4095.0);

    // --- DHT11 Reading ---
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // --- Serial Monitor Output ---
    Serial.println("+--------------------------------------+");
    Serial.println("|        Environment Status            |");
    Serial.println("+--------------------------------------+");

    // Dust Interpretation
    if (dustDensity < 50) {
      Serial.println("| Air Quality: Good                    |");
    } else if (dustDensity < 150) {
      Serial.println("| Air Quality: Moderate                |");
    } else {
      Serial.println("| Air Quality: Poor                    |");
    }

    // LDR Interpretation
    if (ldrVoltage > 2.5) {
      Serial.println("| Light Level: Bright                  |");
    } else if (ldrVoltage > 1.0) {
      Serial.println("| Light Level: Dim                     |");
    } else {
      Serial.println("| Light Level: Dark                    |");
    }

    // DHT11 Temperature and Humidity
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("| Temperature/Humidity sensor error    |");
    } else {
      Serial.printf("| Temperature: %.1f °C                 |\n", temperature);
      Serial.printf("| Humidity: %.1f %%                     |\n", humidity);
    }

    Serial.println("+--------------------------------------+\n");

    // Save latest readings
    lastDustDensity = dustDensity;
    lastLdrVoltage = ldrVoltage;
    lastTemperature = temperature;
    lastHumidity = humidity;
  }

  // --- Send Data to ThingSpeak every 5 minutes ---
  if (currentMillis - lastThingSpeakUpdate > thingSpeakInterval) {
    lastThingSpeakUpdate = currentMillis;

    ThingSpeak.setField(3, lastDustDensity);
    ThingSpeak.setField(4, lastLdrVoltage);
    ThingSpeak.setField(1, lastTemperature);
    ThingSpeak.setField(2, lastHumidity);

    int statusCode = ThingSpeak.writeFields(CHANNEL_ID, WRITE_API_KEY);

    if (statusCode == 200) {
      Serial.println("ThingSpeak update successful.");
    } else {
      Serial.printf("ThingSpeak update failed. Code: %d\n", statusCode);
    }
  }
}

// ===== OTA Drive Device Update Section =====
String getChipId() {
  String ChipIdHex = String((uint32_t)(ESP.getEfuseMac() >> 32), HEX);
  ChipIdHex += String((uint32_t)ESP.getEfuseMac(), HEX);
  return ChipIdHex;
}

void updateOTADrive() {
  String url = "http://otadrive.com/deviceapi/update?";
  url += "k=" + String(otaDriveDeviceKey);
  url += "&v=" + String(firmwareVersion);
  url += "&s=" + getChipId();

  Serial.println("Starting OTA update process...");
  Serial.print("Using OTA Drive Device Key: ");
  Serial.println(otaDriveDeviceKey);

  WiFiClient client;
  t_httpUpdate_return ret = httpUpdate.update(client, url, firmwareVersion);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.print("OTA update failed. Error code: ");
      Serial.println(httpUpdate.getLastError());
      Serial.println(httpUpdate.getLastErrorString());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("No OTA updates available.");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("OTA update successful! Device will reboot automatically.");
      break;
  }
}