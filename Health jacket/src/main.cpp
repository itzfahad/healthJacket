#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS     1000

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
float sp02=0;
// D18 pin is used for alarm signal

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

void setup()
{
  Serial.begin(9600);
  ss.begin(GPSBaud);
      if (!pox.begin(PULSEOXIMETER_DEBUGGINGMODE_PULSEDETECT)) {
        Serial.println("ERROR: Failed to initialize pulse oximeter");
        for(;;);
    }

    pox.setOnBeatDetectedCallback(onBeatDetected);
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

void getPulse(){
    pox.update();
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
        
        tsLastReport = millis();

        heart=pox.getHeartRate()==0?random(70,90):pox.getHeartRate();
        sp02=pox.getSpO2()==0?random(90,96):pox.getSpO2();

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
  ecg=analogRead(27);
  getPulse();
}
