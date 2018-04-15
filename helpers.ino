
JsonObject& httpsRequestGet() {
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
  Serial.println(charBuf);

  StaticJsonBuffer<1024> jsonBuffer;
  JsonObject& configObject = jsonBuffer.parseObject(charBuf);
  if (!configObject.success()) {
    Serial.println("JSON Serialization failed!");

  }
  return configObject;

}

int getConfig() {
  JsonObject& configObject = httpsRequestGet();
  const int date = configObject["date"];
  JsonArray& data = configObject["schedules"];
  setTime(date);

  String jsonStr;
  data.printTo(jsonStr);
  Serial.println(jsonStr);
  schedules = jsonStr;
  Serial.println(schedules);
  
  return 0;
}

// Custom function accessible by the API
int updateConfig(String command) {
  getConfig();
  return 0;
}

