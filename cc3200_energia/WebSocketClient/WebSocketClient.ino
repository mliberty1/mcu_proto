/* WebSocket Client for CC3200 LaunchPad */

#include <WiFi.h>

// Here we define a maximum framelength to 64 bytes. Default is 256.
#define MAX_FRAME_LENGTH 64

// Define how many callback functions you have. Default is 1.
#define CALLBACK_FUNCTIONS 1

#include <WebSocketClient.h>

char wifi_ssid[] = "RoughBuffalo";
char wifi_password[] = "321libertyfamily!";
char websocket_server[] = "mcu_proto.jetperch.com";
int websocket_port = 80;
char websocket_path[] = "/ws";

WiFiClient client;
WebSocketClient webSocketClient;

void setup() {
  Serial.begin(115200);
  Serial.print("Attempting to connect to Network named: ");
  Serial.println(wifi_ssid); 
  
  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
  WiFi.begin(wifi_ssid, wifi_password);
  while ( WiFi.status() != WL_CONNECTED) {
    // print dots while we wait to connect
    Serial.print(".");
    delay(250);
  }
  
  Serial.println("\nYou're connected to the network");
  Serial.println("Waiting for an ip address");
  
  while (WiFi.localIP() == INADDR_NONE) {
    // print dots while we wait for an ip addresss
    Serial.print(".");
    delay(250);
  }

  Serial.println("\nIP Address obtained");
  printWifiStatus();

  // Connect to the websocket server
  while (!client.connect(websocket_server, websocket_port)) {
    Serial.println("Connection failed.");
    delay(1000);
  }
  Serial.println("Connected.");

  // Handshake with the server
  webSocketClient.path = websocket_path;
  webSocketClient.host = websocket_server;
  
  if (webSocketClient.handshake(client)) {
    Serial.println("Handshake successful");
  } else {
    Serial.println("Handshake failed.");
    while(1) {
      // Hang on failure
    }  
  }
}

void loop() {
  String data;
  if (client.connected()) {
    webSocketClient.getData(data);

    if (data.length() > 0) {
      Serial.print("Received data: ");
      Serial.println(data);
    }
    //webSocketClient.sendData(data);
  } else {
    Serial.println("Client disconnected.");
    while (1) {
      // todo : reconnect.
    }
  }
  
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
