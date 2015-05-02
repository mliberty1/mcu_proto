/* WebSocket Client for CC3200 LaunchPad */

// https://www.olimex.com/Products/Modules/LED/MOD-LED8x8RGB/open-source-hardware
// https://github.com/OLIMEX/DUINO/tree/master/AVR/MOD-LED8x8RGB

#include "wifi_config.h"
#include <WiFi.h>
#include "WebClient.h"
#include <SPI.h>
#include "font.h"
#include "lcd8x8rgb.h"
#define LED RED_LED

char websocket_server[] = "mcu_proto.jetperch.com";
int websocket_port = 80;
char websocket_path[] = "/ws";

boolean draw_en = false;
WiFiClient client;
WebsocketClient webSocketClient(websocket_server, websocket_port, websocket_path, false, wscMessage);

char responseMessage[] = "{\"cmd\": \"subscribe\", \"channel\": \"CC3200\"}";

void wscConnect() {
  webSocketClient.sendMessage(responseMessage, sizeof(responseMessage));
}

void wscMessage(char* msg)
{
  Serial.print("Got msg : ");
  Serial.println(msg);
  
  if (strcmp(msg, "CC3200_ON") == 0) {
    draw_en = true;
  } else if (strcmp(msg, "CC3200_OFF") == 0) {
    draw_en = false;
  }
  digitalWrite(LED, !draw_en);
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
  
  // Configure SPI
  SPI.begin(); // start the SPI library
  pinMode(chipSelectPin, OUTPUT);
  SPI.setDataMode(SPI_MODE1);
  SPI.setClockDivider(SPI_CLOCK_DIV16);

  color = 1;
  drawRectangle(4, 4, 5, 5);
  Transfer();
  vClear();
  
  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
  WiFi.begin(wifi_ssid, wifi_password);
  wifi_connect();
  webSocketClient.connect();
}

int i = 0;

void loop() {
  String data;
  wifi_connect();
  webSocketClient.run();
  if (draw_en) {
    lScroll();
    drawLine(8, 1, 8, 8);
  } else {
    vClear();
  }
  ++color;
  Transfer();
  delay(50);
  ++i;
  if (i >= 8) {
    i = 0;
  }
}

