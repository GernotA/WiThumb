
/***************************************************************************************************
 * This sample code is designed for WiThumb:
 * https://www.kickstarter.com/projects/hackarobot/withumb-arduino-compatible-wifi-usb-thumb-imu
 * 
 * 
 * In this example, WiThumb posts temperature readings to the cloud (hosted on thingspeak.com)
 * 
 * It requires the following library. Please install it before compiling this.
 *     https://github.com/adafruit/Adafruit_MCP9808_Library
 * 
 * For more information about WiThumb, please check out my Instructables
 * http://www.instructables.com/id/How-to-Build-a-WiFi-Thermometer/
 * 
 * Comments, questions? Please check out our Google+ community:
 * https://plus.google.com/u/0/communities/111064186308711245928
 * 
 **************************************************************************************************/
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include "Adafruit_MCP9808.h"

long  firstTime=millis();

// sample and post temperature about every 2 minutes
#define sampleInterval 120

// temperature sensor timeout in 3 seconds
#define timeOutSensor 3
// if sensor timed out, deep sleep and wake up in 30 seconds
#define tempSensorResetInterval 30 

// WiFi timeout if not connected in 10 seconds
#define timeOutWiFi   10  

// if not able to connect to WiFi , deep sleep and wake up in 30 seconds
#define wifiResetInterval 30

// I2C address of MCP9808
#define tempSensorAddress 0x1F

// Create the MCP9808 temperature sensor object
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();


char ssid[] = "ssid";                // enter your network SSID (name)
char pass[] = "pass";                // enter your network password

float temperature;                  // temperature in F

const int httpPort = 80;  
//https://api.thingspeak.com/channels/12633/feeds.json?results=2
const char* host = "api.thingspeak.com";
const char* thingspeak_key = "key";

void setup() {

  Serial.begin(115200);
  Serial.println("\n-----------------------------------------\n");
  Serial.println("REBOOT");
  Wire.begin();
  

  if (findTemperatureSensor()) {
    temperature=readTemperature();
    if (!connectToWiFi()) {
      Serial.println("\nWiFi timeout - reset in "+String(wifiResetInterval)+ " seconds");
      ESP.deepSleep(wifiResetInterval*1000000, WAKE_RF_DEFAULT);
    } else {
      postToSparkFun();
    } 
  } else {
     Serial.println("Unable to find sensor - reset and wake up in "+String(tempSensorResetInterval)+ " seconds");
     ESP.deepSleep(tempSensorResetInterval*1000000, WAKE_RF_DEFAULT);
  }

  long sleepTime = sampleInterval*1000 - (millis()-firstTime);
  if (sleepTime < 0) { 
      sleepTime =1;
  } 
  Serial.println("\nData posted. Go to deep sleep and reset in "+String((sleepTime/1000.0))+ " seconds"); 
  ESP.deepSleep(sleepTime*1000, WAKE_RF_DEFAULT);
}

void loop() {

}

void postToSparkFun() {
    Serial.println("connecting to "+String(host));

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(host, httpPort)) {
      Serial.println("connection failed");
      return;
    }
     // We now create a URI for the request
    String url = "/update";
    url += "?api_key=";
    url += thingspeak_key;
    url += "&field2=";
    url += String(temperature);
 
    Serial.print("Requesting URL: ");
    Serial.println(url);
 
    // This will send the request to the server
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }
 
    // Read all the lines of the reply from server and print them to Serial
    while(client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
    }
 
    Serial.println();
    Serial.println("closing connection");
}


void doDelays(long ms) { // yield for 'ms' milli-seconds
  long endMs = millis() + ms;
  while (millis() < endMs) {
     yield();
  }
}

bool findTemperatureSensor() {
  long startMS=millis();
  while(1) {
    if (millis()-startMS > timeOutSensor*1000) {     // timeout
      return false;
    } else if (tempsensor.begin(tempSensorAddress)) {
      return true;
    }
  }
}

float readTemperature() {
  float t;
  Serial.println("wake up MCP9808.... "); 
  tempsensor.shutdown_wake(0);   // Don't remove this line! required before reading temp
  delay(300);                    // wait 300ms so that the sensor is ready for reading. Otherwise, the temperature won't change.
  t = tempsensor.readTempC();
  //t = tempsensor.readTempC()*1.8 + 32.0;

  Serial.println("Temperature is "+String(t)+" degree C"); 
  Serial.println("Shutdown MCP9808.... ");
  tempsensor.shutdown_wake(1);   // shutdown MSP9808 - power consumption ~0.1 micro Ampere
  return t;
}

bool connectToWiFi() {
  Serial.println("Connecting to "+String(ssid));
  WiFi.begin(ssid, pass);
  long startMS=millis();

  while (WiFi.status() != WL_CONNECTED) {
    doDelays(500);
    Serial.print(".");
    if (millis()-startMS > timeOutWiFi *1000) {
      return false;
    } 
  } 
  Serial.print("\nWiFi connected (IP address: ");
  Serial.print(WiFi.localIP());
  Serial.println(")");
  return true;
}
