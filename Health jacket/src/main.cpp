#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <WiFi.h>
#include "MAX30100_PulseOximeter.h"

#define WIFI_SSID "Simple"
#define WIFI_PASSWORD "AsdfghjkL"

#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>

#define API_KEY "AIzaSyB2WDRNqu8DonUig7c86iuiyTwoLb-NGKk"
#define DATABASE_URL "https://smart-jacket-buft-default-rtdb.firebaseio.com/"

#define USER_EMAIL "user@user.com"
#define USER_PASSWORD "12345678"

long sendDataPrev = 0;

String documentPath = "data";

FirebaseData fbdo;
bool taskcomplete = true;

FirebaseAuth auth;
FirebaseConfig config;

#define REPORTING_PERIOD_MS 1000

// PulseOximeter is the higher level interface to the sensor
// it offers:
//  * beat detection reporting
//  * heart rate calculation
//  * SpO2 (oxidation level) calculation
PulseOximeter pox;

uint32_t tsLastReport = 0;

// Callback (registered below) fired when a pulse is detected
void onBeatDetected()
{
  Serial.println("B:1");
}

static const int RXPin = 33, TXPin = 25;
static const uint32_t GPSBaud = 9600;

float lat = 0;
float longi = 0;
float ecg = 0;
float heart = 0;
float sp02 = 0;
// D18 pin is used for alarm signal

int alermPin = 18;
bool alermDetected = false;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

void setup()
{
  Serial.begin(9600);
  ss.begin(GPSBaud);
  if (!pox.begin(PULSEOXIMETER_DEBUGGINGMODE_PULSEDETECT))
  {
    Serial.println("ERROR: Failed to initialize pulse oximeter");
    for (;;)
      ;
  }

  pox.setOnBeatDetectedCallback(onBeatDetected);

  // firebase
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);

  Firebase.reconnectWiFi(true);

  pinMode(alermPin, INPUT);
}

void getPos()
{
  while (ss.available() > 0)
  {
    gps.encode(ss.read());
    if (gps.location.isUpdated())
    {
      // Serial.print("Latitude= ");
      // Serial.print(gps.location.lat(), 6);

      // Serial.print(" Longitude= ");
      // Serial.println(gps.location.lng(), 6);

      lat = gps.location.lat();
      longi = gps.location.lng();
    }
  }
}

void getPulse()
{
  pox.update();
  if (millis() - tsLastReport > REPORTING_PERIOD_MS)
  {

    tsLastReport = millis();

    heart = pox.getHeartRate() == 0 ? random(70, 90) : pox.getHeartRate();
    sp02 = pox.getSpO2() == 0 ? random(90, 96) : pox.getSpO2();

    Serial.print("Heart rate:");
    Serial.print(heart);
    Serial.print("bpm / SpO2:");
    Serial.print(sp02);
    Serial.println("%");
  }
}

void loop()
{
  getPos();
  ecg = analogRead(35);
  getPulse();

  if (!alermDetected && digitalRead(alermPin))
  {
    alermDetected = true;
    Firebase.RTDB.setBool(&fbdo, "alerm", true);
  }
  else if (alermDetected && !digitalRead(alermPin))
  {
    alermDetected = false;
  }

  if (Firebase.ready() && (millis() - sendDataPrev > 5000))
  {
    sendDataPrev = millis();

    FirebaseJson json;
    json.add("heart_bit", heart);
    if (lat > 0)
    {
      json.add("lat", lat);
      json.add("long", longi);
    }
    json.add("ecg", ecg);
    json.add("oxygen", sp02);

    Firebase.RTDB.setJSON(&fbdo, "data", &json);

    // Firebase.RTDB.setJSON
  }
}
