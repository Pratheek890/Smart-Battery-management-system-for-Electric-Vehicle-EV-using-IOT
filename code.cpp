#include <Wire.h>
#include <DHT.h>
#include <WiFi.h>
#include <ThingSpeak.h>

#define DHTPIN 32
#define DHTTYPE DHT11
#define ACS712_PIN 5
#define BUZZER_PIN 25
#define LED_PIN 26
#define MOTOR_PIN 14

const char* ssid = "wifiname";
const char* password = "wifipassword";
const char* apiKey = "thinkspeakapi";
const unsigned long channelID = channelid;

WiFiClient client;
DHT dht(DHTPIN, DHTTYPE);

float current = 0.0, voltage = 7.4;
float soc = 100.0, soh = 90.0, sop = 80.0;
float temperature = 0.0, humidity = 0.0;

unsigned long previousMillis = 0;
const long interval = 5000;

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, HIGH);

  Serial.println("Smart BMS Starting...");
  delay(2000);

  connectToWiFi();
  ThingSpeak.begin(client);
}

void loop() {
  if (millis() - previousMillis >= interval) {
    previousMillis = millis();

    current = readCurrent();
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor!");
      temperature = 0.0;
      humidity = 0.0;
    }

    updateBatteryParameters();
    controlMotorAndBuzzer();
    displayOnSerial();
    sendDataToThingSpeak();
  }
}

float readCurrent() {
  int sensorValue = analogRead(ACS712_PIN);
  float voltageSensor = (sensorValue / 4095.0) * 3.3;
  float current = (voltageSensor - 2.5) / 0.066;
  return abs(current);
}

void updateBatteryParameters() {
  soc -= (current * 0.01);
  if (soc < 0) soc = 0;
}

void controlMotorAndBuzzer() {
  if (temperature > 30.0) {
    digitalWrite(MOTOR_PIN, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("Temperature > 30°C. Motor OFF, Buzzer ON.");
  } else {
    digitalWrite(MOTOR_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Temperature ≤ 30°C. Motor ON, Buzzer OFF.");
  }
}

void displayOnSerial() {
  Serial.println("==============================");
  Serial.println("Smart BMS Data:");
  Serial.print("SOC: "); Serial.print(soc, 1); Serial.println(" %");
  Serial.print("SOH: "); Serial.print(soh, 1); Serial.println(" %");
  Serial.print("SOP: "); Serial.print(sop, 1); Serial.println(" %");
  Serial.print("Voltage: "); Serial.print(voltage, 1); Serial.println(" V");
  Serial.print("Current: "); Serial.print(current, 1); Serial.println(" A");
  Serial.print("Temperature: "); Serial.print(temperature, 1); Serial.println(" °C");
  Serial.print("Humidity: "); Serial.print(humidity, 1); Serial.println(" %");
  Serial.println("==============================");
}

void sendDataToThingSpeak() {
  ThingSpeak.setField(1, soc);
  ThingSpeak.setField(2, soh);
  ThingSpeak.setField(3, sop);
  ThingSpeak.setField(4, voltage);
  ThingSpeak.setField(5, current);
  ThingSpeak.setField(6, temperature);
  ThingSpeak.setField(7, humidity);

  int response = ThingSpeak.writeFields(channelID, apiKey);
  if (response == 200) {
    Serial.println("Data sent to ThingSpeak successfully!");
  } else {
    Serial.print("ThingSpeak write failed, error code: ");
    Serial.println(response);
  }
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startAttemptTime >= 10000) {
      Serial.println("\nFailed to connect to WiFi");
      return;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}
