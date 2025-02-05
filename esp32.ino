#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Base64.h>

#define BUZZER_PIN 27 // Replace with your buzzer pin
#define JERK_THRESHOLD 0.5 // Threshold for jerk
#define GPS_TIMEOUT 5000 // Timeout for GPS data in milliseconds
#define HARD_CODED_LAT "" // Hardcoded latitude
#define HARD_CODED_LON "" // Hardcoded longitude

const char* ssid = "device name"; // Replace with your Wi-Fi SSID
const char* password = "your wifi password"; // Replace with your Wi-Fi Password
const char* serverUrl = "http://add ur url/updateData"; // Python server URL

// Twilio credentials
const char* twilioAccountSid = "your twilio account sid";
const char* twilioAuthToken = "your twilio auth token";
const char* twilioPhoneNumber = "pre-assigned twilio phone number";
const char* recipientPhoneNumber = "your contact number";

Adafruit_MPU6050 mpu;

unsigned long lastSendTime = 0;
unsigned long lastGpsUpdate = 0;

float previousAx = 0, previousAy = 0, previousAz = 0;
unsigned long previousTime = 0;

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startTime > 10000) { // Timeout after 10 seconds
      Serial.println("Failed to connect to WiFi");
      delay(1000); // Wait before retrying
      WiFi.begin(ssid, password); // Retry connection
      startTime = millis();
    }
  }
  Serial.println("");
  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void sendSmsAlert(String latitude, String longitude) {
  // Prepare the message body with Google Maps link
String messageBody = "Accident Alert!! Location " + latitude + "," + longitude;


  // Prepare the Twilio API URL
  String url = "https://api.twilio.com/2010-04-01/Accounts/" + String(twilioAccountSid) + "/Messages.json";

  // Prepare the HTTP client
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Encode the credentials
  String auth = String(twilioAccountSid) + ":" + String(twilioAuthToken);
  String encodedAuth = base64::encode(auth);
  http.addHeader("Authorization", "Basic " + encodedAuth);

  // Prepare the POST data
  String postData = "From=" + String(twilioPhoneNumber) + "&To=" + String(recipientPhoneNumber) + "&Body=" + messageBody;

  // Send the POST request
  int httpResponseCode = http.POST(postData);

  if (httpResponseCode > 0) {
    Serial.println("SMS sent successfully");
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.print("Error sending SMS: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Turn off buzzer initially

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to initialize MPU6050!");
    while (1);
  }
  Serial.println("MPU6050 initialized");

  // Connect to Wi-Fi
  connectToWiFi();
}

void loop() {
  // Check Wi-Fi connection and reconnect if necessary
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  unsigned long currentTime = millis();
  float deltaT = (currentTime - previousTime) / 1000.0; // Time in seconds
  previousTime = currentTime;

  // Calculate jerk
  float jerkX = (a.acceleration.x - previousAx) / deltaT;
  float jerkY = (a.acceleration.y - previousAy) / deltaT;
  float jerkZ = (a.acceleration.z - previousAz) / deltaT;
  previousAx = a.acceleration.x;
  previousAy = a.acceleration.y;
  previousAz = a.acceleration.z;

  float totalJerk = sqrt(jerkX * jerkX + jerkY * jerkY + jerkZ * jerkZ);

  // Read GPS data (simulate for now)
  String gpsLat = ""; // Simulated GPS latitude
  String gpsLon = ""; // Simulated GPS longitude
  if (millis() - lastGpsUpdate > GPS_TIMEOUT) {
    // Use hardcoded GPS values
    gpsLat = HARD_CODED_LAT;
    gpsLon = HARD_CODED_LON;
    Serial.println("Using hardcoded GPS data");
  } else {
    // Simulate GPS read (replace with actual GPS code)
    gpsLat = "12.9636773";
    gpsLon = "77.5034558";
    lastGpsUpdate = millis();
  }

  // Prepare JSON data
  String jsonData = String("{") +
                    "\"accX\": " + a.acceleration.x +
                    ", \"accY\": " + a.acceleration.y +
                    ", \"accZ\": " + a.acceleration.z +
                    ", \"jerkX\": " + jerkX +
                    ", \"jerkY\": " + jerkY +
                    ", \"jerkZ\": " + jerkZ +
                    ", \"latitude\": \"" + gpsLat + "\", \"longitude\": \"" + gpsLon + "\"}";

  // Send data to Python server
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      Serial.println("Data sent successfully");
    } else {
      Serial.print("Error sending data: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }

  // Activate buzzer if jerk exceeds threshold
  if (totalJerk > JERK_THRESHOLD) {
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("Jerk exceeded threshold! Activating buzzer...");
    sendSmsAlert(gpsLat, gpsLon); // Send SMS alert
    delay(6000); // Keep buzzer on for 6 seconds
    digitalWrite(BUZZER_PIN, LOW);
  }

  delay(1000); // Wait before next loop iteration
}
