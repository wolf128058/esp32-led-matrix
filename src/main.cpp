#include <Arduino.h>
#include <EEPROM.h>

#include "arduino_ota_update.h"

#include <SPI.h>
#include "LedMatrix.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#ifdef ESP8266
#include <ESP8266WebServer.h>
#define WebServer ESP8266WebServer
#else
#include <WebServer.h>
#include <HTTP_Method.h>
#endif

#include <WiFiClient.h>

//hostname
#define HOSTNAME "MatrixWall"
#define OTA_PASS_HASH "c4267c48649a272644e23149ecbed632"
#define WIFI_SSID "Freifunk"
#define WIFI_PASS ""
#define WIFI_TIMEOUT 10000
#define WIFI_DELAY 500

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

//Scrolling Direction
int direction = 0;
byte intensity = 0;

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ;
}

void writeStringToEEPROM(int addrOffset, const String &strToWrite)
{
  byte len = strToWrite.length();
  for (int i = 0; i < len; i++)
  {
    EEPROM.write(addrOffset + i, strToWrite[i]);
  }
}

String readStringFromEEPROM(int addrOffset)
{
  String read;
  while (read.length() == 0 || read[read.length() -1] != 0)
  {
    read[read.length()] = EEPROM.read(addrOffset + read.length());
  }
  return read;
}

void dataHandler(){
  String msg = server.arg("message");   //message from POST data
  if (server.arg("direction").length() > 0) {
    direction = server.arg("direction").toInt();
  }
  if (server.arg("intensity").length() > 0) {
    int int_intensity = server.arg("intensity").toInt();
    intensity = (int)int_intensity;
  }
  message = msg;
  writeStringToEEPROM(0,message);      //store received message to EEPROM
  EEPROM.commit();                    //commit the save
  server.send(200);                   //redirect http code
}
  
void setup() {
  //This uses EEPROM to store previous message
  //Initialize EEPROM
  #ifdef ESP8266
  EEPROM.begin(4095);
  #else
  if (!EEPROM.begin(1000)) {
    delay(1000);
    ESP.restart();
  }
  #endif
  ledMatrix.init();
  ledMatrix.setText("Connecting ...");
  WiFi.mode(WIFI_STA);
  //Initiate WiFi Connection
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout <= WIFI_TIMEOUT)
  {
      timeout += WIFI_DELAY;
      delay_with_ota(WIFI_DELAY);
  }

  setup_arduino_ota(HOSTNAME, OTA_PASS_HASH);

  ledMatrix.setNextText("Connected to Freifunk");
  ledMatrix.setNextText(IpAddress2String(WiFi.localIP()));
  server.on("/data",HTTP_POST,dataHandler);
  //start web server
  server.begin();
  ledMatrix.setNextText("Webserver on: " + IpAddress2String(WiFi.localIP()));
  //At first start, read previous message from EEPROM
  message = readStringFromEEPROM(0);
  int len = message.length();
  if (len > 0) {
      ledMatrix.setNextText(message);
  }

}

void loop() {
  ledMatrix.clear();
  server.handleClient();
  int len = message.length();
  ledMatrix.setIntensity(intensity);
  if (len <= 100 && len > 0) {
    ledMatrix.setNextText(message);
  }
  switch (direction) {
    case 1:
      ledMatrix.scrollTextRight();
      break;
    case 2:
      ledMatrix.oscillateText();
      break;
    default:
      ledMatrix.scrollTextLeft();
      break;
  }
  ledMatrix.drawText();
  ledMatrix.commit();
  ArduinoOTA.handle();
  delay(50);
}
