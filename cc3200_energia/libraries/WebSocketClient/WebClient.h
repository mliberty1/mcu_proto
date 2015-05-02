/*
  WebSocketClient demo for Energia platform
  https://github.com/MORA99/Stokerbot/tree/master/Libraries/WebSocketClient
  Released into the public domain - http://unlicense.org
  
  Note: This package includes Base64 and sha1 implementation that I did not write.
  Base64 : https://github.com/adamvr/arduino-base64
  Sha1 : Part of https://code.google.com/p/cryptosuite/
  
  Both are open licenses as of this writing, but you should check to make sure they are compatible with your project.
  If not they can be replaced by other implementations.
*/

#ifndef WebsocketClient_h
#define WebsocketClient_h

#include <WiFi.h>
#include <WiFiClient.h>
//#include <EthernetClient>

typedef void (*onConnect)();
typedef void (*onMessage)(char* msg);

class WebsocketClient
{
  private:
  onConnect _fconnect;
  onMessage _fnc;
  WiFiClient client;
//  EthernetClient client;
  uint16_t _port;
  char* _host;
  char* _path;
  char _key[25];
  boolean _ssl;
  boolean _connected;
  uint8_t _connectionTimer;
  
  boolean sendPong();
  void connectRetry();

public:
  WebsocketClient(char* host, uint16_t port, char* path, boolean ssl, 
                  onConnect fconnect, onMessage fmessage);
  boolean connect();
  int run();
  boolean sendPing();
  boolean sendMessage(char* msg, uint16_t length);
  
private:
  void generate_key();
  boolean validate_accept_key(char * acceptKey);
  boolean client_connect();
  void client_send_header();
  boolean client_validate_response();
  boolean client_read_line(char * line, uint16_t length);
};
#endif

