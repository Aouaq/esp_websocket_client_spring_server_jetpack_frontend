#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h> 
#include "DHT.h"

const char* ssid = "SSID";   //your wifi ssid aka name
const char* password = "pwd";  //wifi password 

const char* server_host = "X.X.X.X"; // Replace with your Spring Boot laptop/PC Local IP address
const int server_port = 8082;             // Spring Boot port
const char* server_path = "/ws/sensor-data"; // The path we will register in Spring

// DHT11 configuration
#define DHTPIN 2     
#define DHTTYPE DHT11  

// Create objects
DHT dht(DHTPIN, DHTTYPE);
ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket; // Changed to Client

// Function to get DHT11 readings as JSON string
String getSensorReadings() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return "";
  }
  
  String json = "{";
  json += "\"temperature\":" + String(temperature, 1) + ","; // Fixed to numeric JSON types
  json += "\"humidity\":" + String(humidity, 1);
  json += "}";
  
  return json;
}

// WebSocket client event handler
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WS] Disconnected from Spring Backend!\n");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WS] Connected to Spring Backend at url: %s\n", payload);
      break;
    case WStype_TEXT:
      Serial.printf("[WS] Received text from Spring: %s\n", payload);
      // If Spring sends a message back, you can handle it here
      break;
    case WStype_BIN:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize DHT11
  dht.begin();
  Serial.println("DHT11 sensor initialized");

  // Connect to WiFi
  WiFiMulti.addAP(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("\nConnected! IP address: ");
  Serial.println(WiFi.localIP());

  // Server address, port, and URL path
  webSocket.begin(server_host, server_port, server_path);
  webSocket.onEvent(webSocketEvent);
  
  // Try to reconnect automatically if connection is lost
  webSocket.setReconnectInterval(5000);
}

void loop() {
  webSocket.loop();

  // Send sensor readings to Spring Boot every second
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 1000) { 
    lastSend = millis();
    
    if(WiFiMulti.run() == WL_CONNECTED) {
      String readings = getSensorReadings();
      if(readings != "") {
        // Send data to the Spring Backend server
        webSocket.sendTXT(readings);
        Serial.println("Sent to Spring: " + readings);
      }
    }
  }
}