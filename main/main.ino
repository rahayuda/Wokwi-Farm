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
  
  // Inisialisasi Seed Random berdasarkan waktu micros agar nilai acak bervariasi
  randomSeed(micros());

  // Konfigurasi waktu NTP untuk Denpasar
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  dht.begin();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(LDR_PIN, INPUT);

  espClient.setInsecure();
}

// Fungsi pembantu untuk menghasilkan angka desimal acak dalam rentang min - max
float getRandomFloat(float minVal, float maxVal) {
  return minVal + (float)random(0, 1000) / 1000.0 * (maxVal - minVal);
}

void getDenpasarProfile(float &temp, float &hum, int &lux, int &dist) {
  struct tm timeinfo;
  int hour = 12; // Default jika NTP belum sinkron

  if (getLocalTime(&timeinfo)) {
    hour = timeinfo.tm_hour;
  }

  // Profil Cuaca Denpasar dengan Rentang Bias Variabel
  if (hour >= 6 && hour < 11) {         // PAGI (06.00 - 11.00)
    temp = getRandomFloat(25.0, 27.5);   // 25.0 - 27.5 °C
    hum  = getRandomFloat(75.0, 85.0);   // 75 - 85 %
    lux  = random(900, 1500);            // 900 - 1500 Lux
    dist = random(90, 110);              // 90 - 110 cm
  } else if (hour >= 11 && hour < 16) {  // SIANG TERIK (11.00 - 16.00)
    temp = getRandomFloat(32.0, 35.0);   // 32.0 - 35.0 °C
    hum  = getRandomFloat(50.0, 60.0);   // 50 - 60 %
    lux  = random(3500, 4200);           // 3500 - 4200 Lux
    dist = random(90, 110);              // 90 - 110 cm
  } else if (hour >= 16 && hour < 19) {  // SORE (16.00 - 19.00)
    temp = getRandomFloat(27.0, 29.5);   // 27.0 - 29.5 °C
    hum  = getRandomFloat(65.0, 75.0);   // 65 - 75 %
    lux  = random(200, 500);             // 200 - 500 Lux
    dist = random(90, 110);              // 90 - 110 cm
  } else {                               // MALAM (19.00 - 06.00)
    temp = getRandomFloat(23.5, 25.5);   // 23.5 - 25.5 °C
    hum  = getRandomFloat(80.0, 90.0);   // 80 - 90 %
    lux  = random(0, 50);                // 0 - 50 Lux
    dist = random(110, 130);             // 110 - 130 cm
  }
}

void sendToFirebase(float temperature, float humidity, int distance, int lightIntensity) {
  if (WiFi.status() == WL_CONNECTED) {
    http.begin(espClient, firebaseHost);
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{\"temperature\": " + String(temperature, 1) + 
                      ", \"humidity\": " + String(humidity, 1) + 
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

  // Ambil data acak sesuai rentang jam real-time Denpasar
  getDenpasarProfile(temp, hum, lux, dist);

  Serial.print("Temperature: "); Serial.print(temp, 1);
  Serial.print(" °C, Humidity: "); Serial.print(hum, 1);
  Serial.print(" %, Distance: "); Serial.print(dist);
  Serial.print(" cm, Light Intensity: "); Serial.println(lux);

  sendToFirebase(temp, hum, dist, lux);

  delay(2000);
}
