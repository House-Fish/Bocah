#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <math.h> // For sqrt()

Adafruit_MPU6050 mpu;

// Wi-Fi credentials
const char* ssid = "Easytohack";
const char* password = "12345678";

// Server endpoint
const char* serverUrl = "http://128.199.180.89:5000/api/receive";

// Constants
const int SAMPLE_SIZE = 50; // Number of samples for rolling analysis
float accelerationBuffer[SAMPLE_SIZE];
float deltaBuffer[SAMPLE_SIZE];
int bufferIndex = 0;

// Timing variables
unsigned long modeStartTime = 0;
String currentMode = "Unknown";

// Function to calculate the standard deviation
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

// Function to connect to Wi-Fi
void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi!");
}

// Function to send data to the server
void sendToServer(String mode, unsigned long duration) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Prepare JSON payload
    String jsonPayload = "{";
    jsonPayload += "\"mode\": \"" + mode + "\",";
    jsonPayload += "\"duration\": " + String(duration / 1000) + ","; // Convert duration to seconds
    jsonPayload += "\"message\": \"Mode change detected\"";
    jsonPayload += "}";

    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json"); // Set content type to JSON
    int httpResponseCode = http.POST(jsonPayload);

    // Check HTTP response
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("Server Response: ");
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end(); // Free resources
  } else {
    Serial.println("Wi-Fi disconnected. Reconnecting...");
    connectToWiFi();
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10); // Wait for Serial monitor

  connectToWiFi();

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("MPU6050 initialized.");
  delay(1000);

  // Initialize the acceleration buffer
  for (int i = 0; i < SAMPLE_SIZE; i++) {
    accelerationBuffer[i] = 0;
    deltaBuffer[i] = 0;
  }

  modeStartTime = millis(); // Initialize mode start time
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Calculate acceleration magnitude
  float accelMagnitude = sqrt(
    pow(a.acceleration.x, 2) +
    pow(a.acceleration.y, 2) +
    pow(a.acceleration.z, 2)
  );

  // Calculate rate of change (delta) for acceleration magnitude
  float delta = abs(accelMagnitude - accelerationBuffer[(bufferIndex - 1 + SAMPLE_SIZE) % SAMPLE_SIZE]);

  // Update the buffers
  accelerationBuffer[bufferIndex] = accelMagnitude;
  deltaBuffer[bufferIndex] = delta;
  bufferIndex = (bufferIndex + 1) % SAMPLE_SIZE;

  // Calculate the standard deviation and average rate of change
  float stdDev = calculateStandardDeviation(accelerationBuffer, SAMPLE_SIZE);
  float avgDelta = calculateStandardDeviation(deltaBuffer, SAMPLE_SIZE);

  // Determine transportation mode
  String newMode;
  if (stdDev < 0.2 && avgDelta < 0.05) {
      newMode = "Standing Still";
  } else if (stdDev >= 0.2 && stdDev < 1.0 && avgDelta >= 0.05 && avgDelta < 0.2) {
      newMode = "Walking";
  } else if (stdDev >= 1.0 && stdDev < 2.5 && avgDelta >= 0.2 && avgDelta < 0.6) {
      newMode = "Taking a train";
  } else if (stdDev >= 2.5 && avgDelta >= 0.6) {
      newMode = "Car";
  } else {
      newMode = "Unknown";
  }

  // Check if mode has changed
  if (newMode != currentMode) {
    unsigned long timeSpent = millis() - modeStartTime;
    Serial.print("Mode changed to: ");
    Serial.print(newMode);
    Serial.print(" | Time spent in previous mode: ");
    Serial.print(timeSpent / 1000);
    Serial.println(" seconds");

    // Send data to the server
    sendToServer(currentMode, timeSpent);

    // Update mode and reset timer
    currentMode = newMode;
    modeStartTime = millis();
  }

  delay(500);
}
