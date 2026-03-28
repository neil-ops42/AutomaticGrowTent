#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#include "config.h"
#include "relays.h"
#include "sensors.h"

// HTML pages in PROGMEM
extern const char HTML_INDEX[];
extern const char HTML_CONTROL[];
extern const char HTML_CHARTS[];
extern const char HTML_HISTORY[];

class WebServerClass
{
public:
    void begin();
    void loop();

private:
    AsyncWebServer server = AsyncWebServer(80);
    AsyncWebSocket ws = AsyncWebSocket("/ws");

    void setupRoutes();
    void setupWebSocket();
    void handleWebSocketMessage(AsyncWebSocketClient* client, const String& msg);
    void broadcastRelayState();
};

extern WebServerClass WebServer;

#endif