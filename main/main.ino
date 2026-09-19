#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <WiFiClientSecure.h>
#include "time.h"

const char* ssid = "Wokwi-GUEST";
const char* password = "";

#define DHTPIN 14
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define trigPin 16
#define echoPin 17
#define LDR_PIN 35

String firebaseHost = "https://wokwi-eea2f-default-rtdb.firebaseio.com/sensorData.json";

WiFiClientSecure espClient;
HTTPClient http;

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600; // WITA (UTC+8 Denpasar)
const int daylightOffset_sec = 0;

void setup() {
  Serial.begin(115200);
  delay(10);
  
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  
  // Konfigurasi waktu NTP untuk Denpasar
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  dht.begin();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(LDR_PIN, INPUT);

  espClient.setInsecure();
}

void getDenpasarProfile(float &temp, float &hum, int &lux, int &dist) {
  struct tm timeinfo;
  int hour = 12; // Default jika NTP belum sinkron

  if (getLocalTime(&timeinfo)) {
    hour = timeinfo.tm_hour;
  }

  // Profil Cuaca Denpasar berdasarkan jam WITA
  if (hour >= 6 && hour < 11) {         // PAGI (06.00 - 11.00)
    temp = 26.0; hum = 80.0; lux = 1000; dist = 100;
  } else if (hour >= 11 && hour < 16) {  // SIANG TERIK (11.00 - 16.00)
    temp = 33.5; hum = 55.0; lux = 4000; dist = 100;
  } else if (hour >= 16 && hour < 19) {  // SORE (16.00 - 19.00)
    temp = 28.0; hum = 68.0; lux = 300;  dist = 100;
  } else {                               // MALAM (19.00 - 06.00)
    temp = 24.5; hum = 85.0; lux = 0;    dist = 120;
  }
}

void sendToFirebase(float temperature, float humidity, int distance, int lightIntensity) {
  if (WiFi.status() == WL_CONNECTED) {
    http.begin(espClient, firebaseHost);
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{\"temperature\": " + String(temperature) + 
                      ", \"humidity\": " + String(humidity) + 
                      ", \"distance\": " + String(distance) + 
                      ", \"lightIntensity\": " + String(lightIntensity) + "}";

    int httpResponseCode = http.PUT(jsonData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.println("Error on sending PUT: " + String(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

void loop() {
  delay(1000); // Beri jeda sinkronisasi NTP
  
  float temp, hum;
  int lux, dist;

  // Ambil data sesuai jam real-time
  getDenpasarProfile(temp, hum, lux, dist);

  Serial.print("Temperature: "); Serial.print(temp);
  Serial.print(" °C, Humidity: "); Serial.print(hum);
  Serial.print(" %, Distance: "); Serial.print(dist);
  Serial.print(" cm, Light Intensity: "); Serial.println(lux);

  sendToFirebase(temp, hum, dist, lux);

  delay(2000);
}
