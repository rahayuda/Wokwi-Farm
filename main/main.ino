#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <WiFiClientSecure.h> // ...

const char* ssid = "Wokwi-GUEST"; // ...
const char* password = ""; // ...

// ...
#define DHTPIN 14 // ...
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ...
#define trigPin 16
#define echoPin 17

// ...
#define LDR_PIN 35  // ...

// ...
String firebaseHost = "https://wokwi-eea2f-default-rtdb.firebaseio.com/sensorData.json"; // ...

WiFiClientSecure espClient; // ...
HTTPClient http;

unsigned long lastMsg = 0;

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  dht.begin();

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(LDR_PIN, INPUT); // ...

  espClient.setInsecure(); // ...
}

int findDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2;
  return distance;
}

void sendToFirebase(float temperature, float humidity, int distance, int lightIntensity) {
  if (WiFi.status() == WL_CONNECTED) {
    http.begin(espClient, firebaseHost); // ...
    http.addHeader("Content-Type", "application/json");

    // ...
    String jsonData = "{\"temperature\": " + String(temperature) + ", \"humidity\": " + String(humidity) + 
                      ", \"distance\": " + String(distance) + ", \"lightIntensity\": " + String(lightIntensity) + "}";

    // ...
    int httpResponseCode = http.PUT(jsonData); // ...

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
  delay(10);

  unsigned long now = millis();
  if (now - lastMsg > 5000) { // ...
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int distance = findDistance();
    int lightIntensity = analogRead(LDR_PIN); // ...

    if (!isnan(temperature) && !isnan(humidity)) {
      // ...
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.print(" °C, Humidity: ");
      Serial.print(humidity);
      Serial.print(" %, Distance: ");
      Serial.print(distance);
      Serial.print(" cm, Light Intensity: ");
      Serial.println(lightIntensity);

      // ...
      sendToFirebase(temperature, humidity, distance, lightIntensity);
    } else {
      Serial.println("Failed to read from DHT sensor!");
    }

    lastMsg = now;
  }
}
