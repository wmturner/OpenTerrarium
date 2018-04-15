#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <aREST.h>
#include <WiFiUdp.h>
#include <esp8266wifi.h>
#include <esp8266httpclient.h>
#include <TimeLib.h>

#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define TIME_REQUEST 7 // ASCII bell character requests a time sync message 

const char* host = "mwpx3akfue.execute-api.us-west-1.amazonaws.com";
const int httpsPort = 443;
// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "8F F3 D2 61 54 E1 0A 2F E7 60 23 96 E1 92 A0 93 BE 02 99 0F";
// Create aREST instance
aREST rest = aREST();
const size_t bufferSize = JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(3) + 6*JSON_OBJECT_SIZE(7) + 900;
// WiFi parameters
const char* ssid = "simple-mesh";
const char* password = "ukie1234!";

// The port to listen for incoming TCP connections
#define LISTEN_PORT           80

// Create an instance of the server
WiFiServer server(LISTEN_PORT);

// Variables to be exposed to the API
int temperature;
int humidity;
String schedules;
int configObject;
// Declare functions to be exposed to the API
int ledControl(String command);
  
 int onArray[4];
 int onIndex = 0;
 int offArray[4];
 int offIndex = 0;
 

String pinhash[20];


   
void setup(void)
{
  // Start Serial
  Serial.begin(115200);
  // Init variables and expose them to REST API
  temperature = 24;
  humidity = 40;
  rest.variable("temperature", &temperature);
  rest.variable("humidity", &humidity);
  rest.variable("schedules", &schedules);


  // Function to be exposed
  rest.function("update", updateConfig);

  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("1");
  rest.set_name("esp8266");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
  getConfig();

}

void loop() {

   
  StaticJsonBuffer<1024> jsonBuffer;
  JsonArray& configObject = jsonBuffer.parseArray(schedules);
  if (!configObject.success()) {
    Serial.println("JSON De-serialization failed!");
  }
  // Get current epoch time
  time_t t = now();
  Serial.println(String(hour(t)));

  for (int i = 0; i <= configObject.size(); i++) {


    int pin=configObject[i]["p"];
    
    // Ensure pin is configured as an output
    pinMode(pin, OUTPUT);
    // Compute how many seconds from start of day, schedule item should start at
    int start_seconds = int(configObject[i]["h"]) * 60 * 60 + int(configObject[i]["m"]) * 60 + int(configObject[i]["s"]);


    String current_pinhash = String(start_seconds) + String(pin);
    
    // Store the current amount of seconds since the beginning of the day
    int current_seconds = hour(t) * 60 * 60 + minute(t) * 60 + second(t);
    int finished_seconds = start_seconds + int(configObject[i]["d"]);

    // If the time the schedule should start occurs on or after the current amount of seconds in the day,
    // and before the end of the time interval:
    if (current_seconds >= start_seconds && current_seconds < finished_seconds) {
      Serial.println("True");
      // Turn the relay on
      digitalWrite(configObject[i]["p"], HIGH);
      
      // When turning a relay on, assign the current pin hash to the list so we can
      // we can check it before turning off
      pinhash[pin] = current_pinhash;

    } else {
      // Otherwise make sure the relay is off
      if (pinhash[pin] == current_pinhash) {
         digitalWrite(configObject[i]["p"], LOW);

      } 
    }
  }
  
  delay(1000);
  offIndex=0;
  onIndex=0;
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  while (!client.available()) {
    delay(1);
  }
  rest.handle(client);
}




