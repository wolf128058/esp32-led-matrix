#include <EEPROM.h>

#include <WebServer.h>
#include <HTTP_Method.h>

#include <SPI.h>
#include "LedMatrix.h"

#include <WiFi.h>
#include <WiFiClient.h>
const char ssid[] = "Freifunk"; // WiFi name
const char password[] = ""; // WiFi password

#define NUMBER_OF_DEVICES 4 //number of led matrix connect in series
#define CS_PIN 15
#define CLK_PIN 14
#define MISO_PIN 2 //we do not use this pin just fill to match constructor
#define MOSI_PIN 12

LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

//Create WebServer instance
WebServer server(80);

//Initialize message to display
String message = "";

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ;
}

void dataHandler(){
  String msg = server.arg("message");   //message from POST data
  message = msg;
  EEPROM.writeString(0,message);      //store received message to EEPROM
  EEPROM.commit();                    //commit the save
  server.send(200);                   //redirect http code
}
  
void setup() {
  //This uses EEPROM to store previous message
  //Initialize EEPROM
  if (!EEPROM.begin(1000)) {
    delay(1000);
    ESP.restart();
  }
  ledMatrix.init();
  ledMatrix.setText("Connecting ...");
  WiFi.mode(WIFI_STA);
  //Initiate WiFi Connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  ledMatrix.setNextText("Connected to Freifunk");
  ledMatrix.setNextText(IpAddress2String(WiFi.localIP()));
  server.on("/data",HTTP_POST,dataHandler);
  //start web server
  server.begin();
  ledMatrix.setNextText("Webserver on: " + IpAddress2String(WiFi.localIP()));
  //At first start, read previous message from EEPROM
  message = EEPROM.readString(0);
  int len = message.length();
  if (len > 0) {
      ledMatrix.setNextText(message);
  }

}

void loop() {
  ledMatrix.clear();
  server.handleClient();
  int len = message.length();
  if (len <= 100 && len > 0) {
    ledMatrix.setNextText(message);
  }
  ledMatrix.scrollTextLeft();
  ledMatrix.drawText();
  ledMatrix.commit();
  delay(50);
}
