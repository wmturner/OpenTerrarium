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


  StaticJsonBuffer<1000> jsonBuffer;
  JsonArray& configObject = jsonBuffer.parseArray(schedules);
  if (!configObject.success()) {
    Serial.println("JSON De-serialization failed!");
  }

  // Get current epoch time
  time_t t = now();
  for (int i = 0; i <= configObject.size(); i++) {

    int start_seconds = int(configObject[i]["hour"]) * 60 * 60 + int(configObject[i]["minute"]) * 60 + int(configObject[i]["second"]);
    int current_seconds = hour(t) * 60 * 60 + minute(t) * 60 + second(t);

    if (current_seconds >= start_seconds && current_seconds < start_seconds + int(configObject[i]["duration"])) {
      pinMode(configObject[i]["pin"], OUTPUT);
      digitalWrite(configObject[i]["pin"], HIGH);
    } else {
      digitalWrite(configObject[i]["pin"], LOW);
    }
  }

  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  while (!client.available()) {
    delay(1);
  }
  rest.handle(client);

}

int getConfig() {
  JsonObject& configObject = httpsRequest();
  const int date = configObject["date"];
  JsonArray& data = configObject["schedules"];
  setTime(date);

  time_t t = now();
  Serial.println(hour(t));

  String jsonStr;
  data.printTo(jsonStr);
  Serial.println(jsonStr);
  schedules = jsonStr;
  Serial.println("closing connection");
  return 0;
}

// Custom function accessible by the API
int updateConfig(String command) {
  getConfig();
  return 0;
}

JsonObject& httpsRequest() {
  WiFiClientSecure client;

  Serial.print("Downloading config from ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
  }

  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }

  String url = "/prod";

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');


    if (line == "\r") {
      Serial.println("headers received");
      break;
    }

  }

  String line = client.readStringUntil('\n');
  char charBuf[line.length() + 1];
  line.toCharArray(charBuf, line.length() + 1);

  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject& configObject = jsonBuffer.parseObject(charBuf);
  if (!configObject.success()) {
    Serial.println("JSON Serialization failed!");

  }
  return configObject;

}

