// Copyright (c) 2015 Jetperch LLC
// This file is licensed under the MIT License
// http://opensource.org/licenses/MIT

#include "mbed.h"
#include "EthernetInterface.h"
#include "Websocket.h"
#include "APA102.h"

DigitalOut led_red(LED_RED);
DigitalOut led_green(LED_GREEN);
DigitalIn sw2(SW2);
DigitalIn sw3(SW3);
Serial pc(USBTX, USBRX);
SPI spi(D11, D12, D13); // mosi, miso, sclk
const int LED_COUNT = 60;
APA102 apa102(LED_COUNT, &spi);

Mail<int, 16> mail_box;


void led_thread(void const *argument)
{
    float offset = 0.0f;
    const float iter_incr = 0.005f;
    const float led_incr = 1.0f / LED_COUNT;
    float value = 0;
    
    spi.format(8, 0);
    spi.frequency(1000000);
    apa102.clear();
    apa102.setRGB(0, 255, 0, 0);
    apa102.setRGB(1, 0, 255, 0);
    apa102.setRGB(2, 0, 0, 255);
    apa102.refresh();
    Thread::wait(1000);
    
    int mode = 0;
    const float BRIGHTNESS = 0.1f;
    float brightness = BRIGHTNESS;
    
    while (true) {
        brightness = BRIGHTNESS;
        
        osEvent evt = mail_box.get(0);
        if (evt.status == osEventMail) {
            int *mail = (int *) evt.value.p;
            int mode_new = *mail;
            mail_box.free(mail);
            if (mode_new) {
                brightness = 1.0f;
            }
            mode = mode_new;
        }
        
        if (mode) {
            value = offset;
            for (int i = 0; i < LED_COUNT; ++i) {
                apa102.setHSV(i, value, 1.0f, brightness);
                value += led_incr;
                if (value >= 1.0f) {
                    value -= 1.0f;
                }
            }
            offset += iter_incr;
            if (offset >= 1.0f) {
                offset -= 1.0f;
            }
        } else {
            apa102.clear();
        }
        apa102.refresh();
        Thread::wait(10);
    }
}


int main() {
    char recv[256];
 
    pc.baud(115200);
    pc.printf("FRDM-K64F booted.\r\n");
    led_green = 1;
    led_red = 1;
    
    pc.printf("Initializing LEDs.\r\n");
    Thread thread(led_thread);
 
    pc.printf("Connecting to network.\r\n");
    EthernetInterface eth;
    while (eth.init()) {
        pc.printf("eth.init() failed.  Retry...\r\n");
        wait(1.0);
    }
    while (eth.connect()) {
        pc.printf("eth.connect() failed.  Retry...\r\n");
        wait(1.0);
    }
    pc.printf("IP Address is %s\r\n", eth.getIPAddress());
 
    Websocket ws("ws://mcu_proto.jetperch.com/ws");
 
    while (1) {
        if (!ws.is_connected()) {
            led_red = 0;
            pc.printf("Websocket not connected\r\n");
            ws.connect();
            wait(1.0);
            continue;
        }
        
        if (led_red == 0) {
            pc.printf("Websocket connected\r\n");
            led_green = 0;
        }

        led_red = 1;
        //ws.send("WebSocket Hello World over Ethernet");
        if (ws.read(recv)) {
            pc.printf("rcv: %s\r\n", recv);
            if (strcmp(recv, "mbed_ON") == 0) {
                led_green = 1;
            } else if (strcmp(recv, "mbed_OFF") == 0) {
                led_green = 0;
            }
            int *mail = mail_box.alloc();
            *mail = led_green;
            mail_box.put(mail);
        }
        pc.printf(".");
        wait(0.1);
    }
}
