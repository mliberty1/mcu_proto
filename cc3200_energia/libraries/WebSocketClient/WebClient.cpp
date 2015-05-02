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

#include "WebClient.h"
#include "Base64.h"
#include "sha1.h"

const uint8_t CONNECTION_TIMER_DEFAULT = 100;


WebsocketClient::WebsocketClient(char* host, uint16_t port, char* path, boolean ssl, 
        onConnect fconnect, onMessage fmessage);)
{
  _fconnect = fconnect;
  _host = host;
  _port = port;
  _path = path;
  _connected = false;
  _connectionTimer = CONNECTION_TIMER_DEFAULT;
  _fnc = fnc;
  _ssl = ssl;
}

void printHash(uint8_t* hash) {
    int i;
    for (i=0; i<20; i++) {
        Serial.print("0123456789abcdef"[hash[i] >> 4]);
        Serial.print("0123456789abcdef"[hash[i] & 0x0f]);
    }
    Serial.println();
}

void WebsocketClient::generate_key() {
    char key[16];
    for (uint8_t i=0; i<16; i++) {
        key[i] = random(0, 255); // Must be randomized for each connection
    }
    base64_encode(_key, key, 16); 
    Serial.print("Generated key ");
    Serial.println(_key);
}

boolean WebsocketClient::validate_accept_key(char * acceptKey) {
    /*
    The client sends a Sec-WebSocket-Key which is a random value that has been base64 encoded. 
    To form a response, the GUID 258EAFA5-E914-47DA-95CA-C5AB0DC85B11 is appended to this base64 encoded key. 
    The base64 encoded key will not be decoded first.
    The resulting string is then hashed with SHA-1, then base64 encoded. 
    Finally, the resulting reply occurs in the header Sec-WebSocket-Accept.
    */

    Sha1.init();
    Sha1.print(_key);
    Sha1.print("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    char res[30];
    char* ptr = (char*)Sha1.result();
    base64_encode(res, ptr, 20);

    if (strcmp(res, acceptKey) == 0) {
        Serial.println("Handshake matches");
        return true;
    } else {
        Serial.print("Bad handshake : ");
        Serial.print(acceptKey);
        Serial.print(" != ");
        Serial.print(res);
        Serial.print(" ( ");
        Serial.print(strcmp(res, acceptKey));
        Serial.println(" ) ");
    }
    return false;
}

boolean WebsocketClient::client_connect() {
    Serial.print("Connecting to ");
    Serial.print(_host);
    if ((_ssl && client.sslConnect(_host, _port)) || 
            (!_ssl && client.connect(_host, _port))) {
        Serial.println(" : success");
        return true;
    } else {
        Serial.println(" : FAILED");
        return false;
  }
}

void WebsocketClient::client_send_header() {
    client.print("GET ");
    client.print(_path);
    client.print(" HTTP/1.1\r\n");
    client.print("Upgrade: websocket\r\n");
    client.print("Connection: Upgrade\r\n");
    client.print("Host: ");
    client.print(_host);
    client.println(); 
    client.print("Sec-WebSocket-Key: ");
    client.print(_key);
    client.println();
    client.print("Sec-WebSocket-Protocol: chat");
    client.println();
    client.print("Sec-WebSocket-Version: 13\r\n");
    client.println();
}

boolean WebsocketClient::client_read_line(char * line, uint16_t length) {
    boolean lineEndFound = false;
    uint8_t i = 0;

    while (1) {
        if (i == length - 1) {
            Serial.print("Header line too long ? : ");
            line[i] = NULL;
            Serial.println(line);
            return false;
        }
        uint16_t timeout = 5000; // 5 seconds 
        while (!client.available()) {
            delay(1);
            timeout--;
            if (timeout == 0) {
                Serial.println("Connection failed");
                return false;
            }
        }
        char c = client.read();
        if (c == '\r' ) { // end of line
            line[i++] = 0;
            client.read(); // discard \n
            Serial.println(line);
            return true;
        }
        line[i++] = c;
    }
}    

boolean WebsocketClient::client_validate_response() {
    //Find Sec-Websocket-Accept and validate it
    //Then find \r\n\r\n that ends the HTTP header
    boolean handshake = false;
    char line[100];
  
    while (1) {
        if (!client_read_line(line, sizeof(line))) {
            return false;
        }
        if (line[0] == 0) { // blank line is end of header
            break;
        }
        char* acceptKey = strstr(line, "Sec-Websocket-Accept");
        if (acceptKey) {
            acceptKey = strstr(line, ":");
            acceptKey += 2;
            handshake = validate_accept_key(acceptKey);
        }
    }
    return handshake;
}

boolean WebsocketClient::connect() {
    generate_key();
    _connected = false;
    if (!client_connect()) {
        return false;
    }
    client_send_header();
    if (!client_validate_response()) {
        client.stop();
        return false;
    }
    _connected = true;
    if (_fconnect) {
        _fconnect();
    }
    return true;
}

void WebsocketClient::connectRetry()
{
    if (_connectionTimer <= 0) {
        Serial.println("Starting reconnect");
        connect();
        _connectionTimer = CONNECTION_TIMER_DEFAULT;
    } else {
        --_connectionTimer;
    }
}

int WebsocketClient::run()
{
  if (!client.connected()) {
      _connected = false;
      connectRetry();
  }
  if (client.available() > 0)
  {
    Serial.print("Data : ");
    Serial.println(client.available());

    char c = client.read();
    uint8_t op = c & 0b00001111;
    uint8_t fin = c & 0b10000000;
    Serial.print("Opcode : ");
    Serial.println(op,BIN);
    Serial.print("Fin : ");
    Serial.println(fin,BIN);
    /*
      0x00: this frame continues the payload from the last.
      0x01: this frame includes utf-8 text data.
      0x02: this frame includes binary data.
      0x08: this frame terminates the connection.
      0x09: this frame is a ping.
      0x10: this frame is a pong.    
    */
    if (op == 0x00 || op == 0x01 || op==0x02) //Data
    {
      Serial.println("WS Got opcode packet");
      if (fin > 0)
      {
        Serial.println("Single frame message");
        c = client.read();
        char masked = c & 0b10000000;
        uint16_t len = c & 0b01111111;
        
        if (len == 126)
        {
          //next 2 bytes are length
          len = client.read();
          len << 8;
          len = len & client.read();
        }
        if (len == 127)
        {
          //next 8 bytes are length
          Serial.println("64bit messages not supported");
          return -1;
        }
        
        Serial.print("Message is ");
        Serial.print(len);
        Serial.println(" chars long");        
        
        //Generally server replies are not masked, but RFC does not forbid it
        char mask[4];
        if (masked > 0)
        {
          mask[0] = client.read();
          mask[1] = client.read();
          mask[2] = client.read();
          mask[3] = client.read();
        }
        
        char data[len+1]; //Max 16bit length message, so 65kbyte ...
        for (uint8_t i=0; i<len; i++)
        {
          data[i] = client.read();
          if (masked > 0) data[i] = data[i] ^ mask[i % 4];
        }
        data[len] = NULL;
        Serial.println("Frame contents : ");
        Serial.println(data); //This is UTF-8 code, but for the general ASCII table UTF8 and ASCII are the same, so it wont matter if we dont send/recieve special chars.
        _fnc(data);
      } //Currently this code does not handle fragmented messenges, since a single message can be 64bit long, only streaming binary data seems likely to need fragmentation.
      
    } else if (op == 0x08)
    {
      Serial.println("WS Disconnect opcode");
      client.write(op); //RFC requires we return a close op code before closing the connection
      delay(25);
      client.stop();
    } else if (op == 0x09)
    {
      Serial.println("Got ping ...");      
      sendPong();
    } else if (op = 0x10)
    {
      Serial.println("Got pong ...");
      c = client.read();
      char masked = c & 0b10000000;
      uint16_t len = c & 0b01111111;      
      while (len > 0) //A pong can contain app data, but shouldnt if we didnt send any...
      {
        client.read();
        len--;
      }
    } else {
      Serial.print("Unknown opcode "); //Or not start of package if we failed to parse the entire previous one
      Serial.println(op);
    }
  }
  return 0;
}

boolean WebsocketClient::sendPong()
{
  client.write(0x8A); //Pong
  client.write(0x00); //no mask, zero length
}

boolean WebsocketClient::sendPing()
{
  client.write(0x89); //Ping
  client.write(0x00); //no mask, zero length
}

boolean WebsocketClient::sendMessage(char* msg, uint16_t length)
{
  /*
  +-+-+-+-+-------+-+-------------+-------------------------------+
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-------+-+-------------+-------------------------------+
  |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
  |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
  |N|V|V|V|       |S|             |   (if payload len==126/127)   |
  | |1|2|3|       |K|             |                               |
  +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
  |     Extended payload length continued, if payload len == 127  |
  + - - - - - - - - - - - - - - - +-------------------------------+
  |                               | Masking-key, if MASK set to 1 |
  +-------------------------------+-------------------------------+
  | Masking-key (continued)       |          Payload Data         |
  +-------------------------------- - - - - - - - - - - - - - - - +
  :                     Payload Data continued ...                :
  + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
  |                     Payload Data continued ...                |
  +---------------------------------------------------------------+
  
  Opcodes
  0x00: this frame continues the payload from the last.
  0x01: this frame includes utf-8 text data.
  0x02: this frame includes binary data.
  0x08: this frame terminates the connection.
  0x09: this frame is a ping.
  0x10: this frame is a pong.
  */
    
  client.write(0b10000001);
  
  /*
  payload_len (7 bits): the length of the payload.
  
  0-125 mean the payload is that long. 
  126 means that the following two bytes indicate the length.
  127 means the next 8 bytes indicate the length. 
  
  So it comes in ~7bit, 16bit and 64bit.
  */
  
  if (length <= 125) 
  {
    client.write(0b10000000 | length);
  }
  else
  {
    client.write(0b11111110); //mask+126
    client.write((uint8_t)(length >> 8));
    client.write((uint8_t)(length & 0xFF));
  }
  //64bit outgoing messenges not supported
  
  byte mask[4];
  for (uint8_t i=0; i<4; i++)
  {
    mask[i] = random(0, 255);
    client.write(mask[i]);
  }
  for (uint16_t i=0; i<length; i++) {
    client.write(msg[i] ^ mask[i % 4]);
  }
}

