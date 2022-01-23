#include <Arduino.h>
#include <string>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>

#include <WebSocketsClient.h>
#include <SocketIOclient.h>

#include <Hash.h>

ESP8266WiFiMulti WiFiMulti;
SocketIOclient socketIO;


/* ----- */
#ifndef STASSID
#define STASSID "MininoxD"
#define STAPSK  "MininoxD1234xD:v"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

/* Define pines*/

#define LedB 2 

#define Rele 4 //D2
/* globals */
#define TemperaturaLimite 10
float TemperaturaLimiteMin = TemperaturaLimite + 0.5;
float TemperaturaLimiteMax = TemperaturaLimite + 1.0;
/* Definicion del sensor */
#include "DHT.h"
#define DHTPIN 5     // D1 
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
/* fin de definicion del sensor */
bool isAutomatic = 1;
bool EstadoRele = 0;
float temp = 0;

/* control difuso */
#define TemperaturaMF 11
float TemperaturaMFMin = TemperaturaMF + 0.5;
float TemperaturaMFMax = TemperaturaMF + 1.0;
#define TemperaturaF 15
float TemperaturaFMin = TemperaturaF + 0.5;
float TemperaturaFMax = TemperaturaF + 1.0;
#define TemperaturaA 17
float TemperaturaAMin = TemperaturaA + 0.5;
float TemperaturaAMax = TemperaturaA + 1.0;
#define Humedad 56
float HumedadMin = Humedad + 1.0;
float HumedadMax = Humedad + 2.0;


void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
    String cmd = "";
    String action = "";
    bool is_action = true;
    String caracter = "";
    String comillas= "";
    char com = 34;
    String comi = comi +(char)com;

    /* enviar */
    DynamicJsonDocument doc(1024);
    JsonArray array = doc.to<JsonArray>();
    JsonObject param1 = array.createNestedObject();
    String output;
  

    switch(type) {
        case sIOtype_DISCONNECT:
            Serial.printf("[IOc] Disconnected!\n");
            break;
        case sIOtype_CONNECT:
            Serial.printf("[IOc] Connected to url: %s\n", payload);

            // join default namespace (no auto join in Socket.IO V3)
            socketIO.send(sIOtype_CONNECT, "/");
            break;
        case sIOtype_EVENT:
            
          /*   Serial.printf("[IOc] get event: %s\n", payload); */
            cmd = "";
            for(int i = 0; i < length; i++) {
               if(i ==0) {
                  
               }else if(i == (length-1)) {
                   

               }else{
                  caracter += (char)payload[i];
                   if(caracter == ","){
                     is_action = false;
                   }
                   if(is_action){
                       comillas += (char)payload[i];

                       if(comillas != comi){
                          action += (char)payload[i];
                       }
                   }else{
                       cmd += (char)payload[i];
                   }
                  caracter = "";
                  comillas = "";
               }
            } //merging payload to single string
   /*          Serial.println(cmd); */
            if(action == "server:send-switch-mode"){
                Serial.println("Click switch mode");
                if(isAutomatic){
                    isAutomatic= 0;
                }else{
                    isAutomatic= 1;
                }
                sendCurrentMode();
            }else if(action == "server:send-switch-rele"){
                Serial.println("Click switch rele");
                if(EstadoRele){
                    EstadoRele= 0;
                    digitalWrite(Rele,LOW);
                }else{
                    EstadoRele= 1;
                    digitalWrite(Rele,HIGH);
                }
                sendCurrentMode();
            }
            
            break;
        case sIOtype_ACK:
            Serial.printf("[IOc] get ack: %u\n", length);
            hexdump(payload, length);
            break;
        case sIOtype_ERROR:
            Serial.printf("[IOc] get error: %u\n", length);
            hexdump(payload, length);
            break;
        case sIOtype_BINARY_EVENT:
            Serial.printf("[IOc] get binary: %u\n", length);
            hexdump(payload, length);
            break;
        case sIOtype_BINARY_ACK:
            Serial.printf("[IOc] get binary ack: %u\n", length);
            hexdump(payload, length);
            break;
    }
}

void setup() {
    // Serial.begin(921600);
    Serial.begin(115200);
    /* pines */
    pinMode(Rele,OUTPUT);
    pinMode(LedB,OUTPUT);
    digitalWrite(LedB,HIGH);

    Serial.println();
    Serial.println();
    Serial.println();

      for(uint8_t t = 4; t > 0; t--) {
          Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
          Serial.flush();
          delay(1000);
      }

    // disable AP
    if(WiFi.getMode() & WIFI_AP) {
        WiFi.softAPdisconnect(true);
    }

    WiFiMulti.addAP(ssid, password);

    //WiFi.disconnect();
    while(WiFiMulti.run() != WL_CONNECTED) {
        digitalWrite(LedB,HIGH);
        delay(100);
        Serial.print(".");
        digitalWrite(LedB,LOW);
        delay(100);
    }
    dht.begin();
    String ip = WiFi.localIP().toString();
    Serial.printf("[SETUP] WiFi Connected %s\n", ip.c_str());
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    // server address, port and URL
   /*  socketIO.begin("192.168.1.38", 5000, "/socket.io/?EIO=4"); */


    socketIO.begin("165.227.82.162", 80, "/socket.io/?EIO=4");

    // event handler
    socketIO.onEvent(socketIOEvent);
}

unsigned long messageTimestamp = 0;
unsigned long statusTimestamp = 0;
void loop() {
    socketIO.loop();
    uint64_t now = millis();
    if(now - messageTimestamp > 10000) {
        messageTimestamp = now;
        delay(2000);
        float h = dht.readHumidity();
        // Read temperature as Celsius (the default)
        float t = dht.readTemperature();
        // Read temperature as Fahrenheit (isFahrenheit = true)
        float f = dht.readTemperature(true);

        // Check if any reads failed and exit early (to try again).
        if (isnan(h) || isnan(t) || isnan(f)) {
            Serial.println(F("Failed to read from DHT sensor!"));
            return;
        }

        /* if(isAutomatic){
            if(t <= TemperaturaLimiteMin){
                digitalWrite(Rele,HIGH);
                EstadoRele = 1;
                sendCurrentMode();
            }
            if(t >= TemperaturaLimiteMax){
                digitalWrite(Rele,LOW);
                EstadoRele = 0;
                sendCurrentMode();
            }
        } */
        if(isAutomatic){
           if(t < TemperaturaMFMin){
              digitalWrite(Rele,HIGH);
              EstadoRele = 1;
            }
            if(t >= TemperaturaMFMax && t <= TemperaturaFMin){
              if(h <= HumedadMin){
                digitalWrite(Rele,HIGH);
                EstadoRele = 1;
              }
              if(h >= HumedadMax){
                digitalWrite(Rele,LOW);
                EstadoRele = 0;
              }
            }
            if(t > TemperaturaFMax && t <= TemperaturaAMin){
              if(h <= HumedadMin){
                digitalWrite(Rele,HIGH);
                EstadoRele = 1;
              }
              if(h >= HumedadMax){
                digitalWrite(Rele,LOW);
                EstadoRele = 0;
              }
            }
            if(t > TemperaturaAMax){
              digitalWrite(Rele,LOW);
              EstadoRele = 0;
            }
        }

        // Compute heat index in Fahrenheit (the default)
        float hif = dht.computeHeatIndex(f, h);
        // Compute heat index in Celsius (isFahreheit = false)
        float hic = dht.computeHeatIndex(t, h, false);
         DynamicJsonDocument doc(1024);
        JsonArray array = doc.to<JsonArray>();

        // add evnet name
        // Hint: socket.on('event_name', ....
        array.add("client-a:current-wheater");

        // add payload (parameters) for the event
        JsonObject param1 = array.createNestedObject();
        param1["humidity"] = h;
        param1["temperatureF"] = t;
        param1["temperatureC"] = f;
        param1["hotIndexF"] = hif;
        param1["hotIndexC"] = hic;
        param1["isAutomatic"] = isAutomatic;
        param1["statusRele"] = EstadoRele;

        // JSON to String (serializion)
        String output;
        serializeJson(doc, output);

        // Send event
        socketIO.sendEVENT(output);

        // Print JSON for debugging
        Serial.println(output);
    }  
}
void  sendCurrentMode(){
     /* send */
    DynamicJsonDocument status(1024);
    JsonArray twoarray = status.to<JsonArray>();
    twoarray.add("client-a:current-status");
    JsonObject param2 = twoarray.createNestedObject();
    param2["isAutomatic"] = isAutomatic;
    param2["statusRele"] = EstadoRele;

    String datatwo;
    serializeJson(status, datatwo);
    socketIO.sendEVENT(datatwo);
    Serial.println(datatwo);
}
