#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <math.h>

Adafruit_MPU6050 mpu;

// Wi-Fi credentials
const char* ssid = "Easytohack";
const char* password = "12345678";

// Server endpoint
const char* serverUrl = "http://128.199.180.89:5000/events";  // Updated endpoint

// Device ID (replace with your device's UUID)
const char* deviceId = "550e8400-e29b-41d4-a716-446655440000";  // Example UUID

// DHT11 configuration
#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Constants
const int SAMPLE_SIZE = 50;
float accelerationBuffer[SAMPLE_SIZE];
float deltaBuffer[SAMPLE_SIZE];
int bufferIndex = 0;

// Timing variables
unsigned long modeStartTime = 0;
unsigned long lastSensorSendTime = 0;
const unsigned long sensorSendInterval = 15000; // 15 seconds
String currentMode = "Unknown";

float calculateStandardDeviation(float data[], int size) {
    float sum = 0, mean = 0, variance = 0;
    for (int i = 0; i < size; i++) {
        sum += data[i];
    }
    mean = sum / size;
    for (int i = 0; i < size; i++) {
        variance += pow(data[i] - mean, 2);
    }
    variance /= size;
    return sqrt(variance);
}

void connectToWiFi() {
    Serial.print("Connecting to Wi-Fi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi!");
}

// Function to send transportation data
void sendTransportationData(String mode, unsigned long duration) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        String jsonPayload = "{";
        jsonPayload += "\"type\": \"transportation\",";
        jsonPayload += "\"device_id\": \"" + String(deviceId) + "\",";
        jsonPayload += "\"mode\": \"" + mode.toLowerCase() + "\",";
        jsonPayload += "\"duration\": " + String(duration / 1000) + ","; // Convert to seconds
        jsonPayload += "\"message\": \"Mode change detected\"";
        jsonPayload += "}";

        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json");
        int httpResponseCode = http.POST(jsonPayload);

        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.printf("Server Response [Transport]: %d - %s\n", httpResponseCode, response.c_str());
        } else {
            Serial.printf("Error sending transport data: %d\n", httpResponseCode);
        }

        http.end();
    }
}

// Function to send AC (temperature/humidity) data
void sendACData(unsigned long duration) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        String jsonPayload = "{";
        jsonPayload += "\"type\": \"air-conditioner\",";
        jsonPayload += "\"device_id\": \"" + String(deviceId) + "\",";
        jsonPayload += "\"duration\": " + String(duration / 1000) + ","; // Convert to seconds
        jsonPayload += "\"message\": \"Mode change detected\"";
        jsonPayload += "}";

        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json");
        int httpResponseCode = http.POST(jsonPayload);

        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.printf("Server Response [AC]: %d - %s\n", httpResponseCode, response.c_str());
        } else {
            Serial.printf("Error sending AC data: %d\n", httpResponseCode);
        }

        http.end();
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    connectToWiFi();

    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) delay(10);
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    dht.begin();

    for (int i = 0; i < SAMPLE_SIZE; i++) {
        accelerationBuffer[i] = 0;
        deltaBuffer[i] = 0;
    }

    modeStartTime = millis();
}

void loop() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float accelMagnitude = sqrt(
        pow(a.acceleration.x, 2) +
        pow(a.acceleration.y, 2) +
        pow(a.acceleration.z, 2)
    );

    float delta = abs(accelMagnitude - accelerationBuffer[(bufferIndex - 1 + SAMPLE_SIZE) % SAMPLE_SIZE]);

    accelerationBuffer[bufferIndex] = accelMagnitude;
    deltaBuffer[bufferIndex] = delta;
    bufferIndex = (bufferIndex + 1) % SAMPLE_SIZE;

    float stdDev = calculateStandardDeviation(accelerationBuffer, SAMPLE_SIZE);
    float avgDelta = calculateStandardDeviation(deltaBuffer, SAMPLE_SIZE);

    String newMode;
    if (stdDev < 0.2 && avgDelta < 0.05) {
        newMode = "walk";  // Changed to match API spec
    } else if (stdDev >= 0.2 && stdDev < 1.0 && avgDelta >= 0.05 && avgDelta < 0.2) {
        newMode = "walk";
    } else if (stdDev >= 1.0 && stdDev < 2.5 && avgDelta >= 0.2 && avgDelta < 0.6) {
        newMode = "train";
    } else if (stdDev >= 2.5 && avgDelta >= 0.6) {
        newMode = "car";
    } else {
        newMode = "walk";  // Default to walk when unknown
    }

    // Check if mode has changed
    if (newMode != currentMode) {
        unsigned long timeSpent = millis() - modeStartTime;
        Serial.printf("Mode changed to: %s (Time spent: %lu seconds)\n", 
                     newMode.c_str(), timeSpent / 1000);

        // Send transportation data
        sendTransportationData(currentMode, timeSpent);

        currentMode = newMode;
        modeStartTime = millis();
    }

    // Send AC data periodically
    if (millis() - lastSensorSendTime >= sensorSendInterval) {
        float temp = dht.readTemperature();
        float humi = dht.readHumidity();

        if (!isnan(temp) && !isnan(humi)) {
            Serial.printf("Temp: %.2f°C, Humidity: %.2f%%\n", temp, humi);
            // Send AC data
            sendACData(sensorSendInterval);
        }

        lastSensorSendTime = millis();
    }

    delay(500);
}