/* WebSocket Client for CC3200 LaunchPad */

#include "wifi_config.h"
#include <WiFi.h>
#include "WebClient.h"
#define LED RED_LED

char websocket_server[] = "mcu_proto.jetperch.com";
int websocket_port = 80;
char websocket_path[] = "/ws";

WiFiClient client;
WebsocketClient webSocketClient(websocket_server, websocket_port, websocket_path, false, wscMessage);

void wscMessage(char* msg)
{
  Serial.print("Got msg : ");
  Serial.println(msg);
  digitalWrite(LED, !digitalRead(LED));
}

void wifi_connect() {
  boolean isConnected = (WiFi.status() == WL_CONNECTED);
  boolean hasIpAddr = !(WiFi.localIP() == INADDR_NONE);
  
  if (isConnected & hasIpAddr) {
    return;
  }
  
  if (!isConnected) {
    Serial.print("Connecting to WiFi network: ");
    Serial.println(wifi_ssid);
    while ( WiFi.status() != WL_CONNECTED) {
      // print dots while we wait to connect
      Serial.print(".");
      delay(100);
    }
    Serial.println();
  }
  
  if (!isConnected || !hasIpAddr) {
    Serial.println("Waiting for IP address");
    while (WiFi.localIP() == INADDR_NONE) {
      // print dots while we wait for an ip addresss
      Serial.print(".");
      delay(100);
    }
    Serial.println();
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }

  // print the received signal strength:
  Serial.print("signal strength (RSSI): ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");  
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
  WiFi.begin(wifi_ssid, wifi_password);
  wifi_connect();
  webSocketClient.connect();
}

void loop() {
  String data;
  wifi_connect();
  webSocketClient.run();
  delay(50);
}

