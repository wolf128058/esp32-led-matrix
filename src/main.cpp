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
#include <ArduinoJson.h>


#include "eeprom_settings.h"
#include "auto_update.h"

//hostname
#define HOSTNAME "MatrixWall"
#define OTA_PASS_HASH "c4267c48649a272644e23149ecbed632"
#define WIFI_SSID "Freifunk"
#define WIFI_PASS ""
#define URL_Version_Info_Default "https://api.github.com/repos/wolf128058/esp32-led-matrix/releases/latest"
#define URL_Firmware_Default "https://github.com/wolf128058/esp32-led-matrix/releases/latest/download/" BUILD_ENV_NAME ".bin"
#define WIFI_TIMEOUT 30000
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

//config
eepromData_t cfg;

void onFirmwareUpdateDone(unsigned int newVersion)
{
    //update was done, save new version number
    cfg.firmwareVer = newVersion;
    saveConfig(cfg);
    //reboot happens after return
}

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ;
}

void beginEEPROM()
{
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
}

void writeStringToEEPROM(int addrOffset, const String &strToWrite)
{
  int len = strToWrite.length();
  beginEEPROM();
  for (int i = 0; i < len; i++)
  {
    EEPROM.put(addrOffset + i, strToWrite[i]);
  }
  EEPROM.put(addrOffset + strToWrite.length(), '\0');
  EEPROM.commit();
  EEPROM.end();
}

String readStringFromEEPROM(int addrOffset)
{
  char read[1024];
  int pos = 0;
  beginEEPROM();
  do
  {
    read[pos] = EEPROM.read(addrOffset + pos);
  } while (read[pos++] != '\0');
  EEPROM.end();
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
  writeStringToEEPROM(sizeof(eepromData_t)+1, message);      //store received message to EEPROM
  EEPROM.commit();                    //commit the save
  server.send(200);                   //ok http code
}

void setup() {
  Serial.begin(115200);
  DynamicJsonDocument doc(8192);
  loadConfig(cfg);
  if (cfg.initialized != 1 || cfg.magic != 1337)
  {
      //not initialized
      cfg.magic = 1337;
      cfg.initialized = 1;
      strncpy(cfg.SSID, WIFI_SSID, sizeof(WIFI_SSID));
      strncpy(cfg.password, WIFI_PASS, sizeof(WIFI_PASS));
      cfg.ota_update_enable = 1;
      cfg.auto_update_enable = 1;
      cfg.firmwareVer = 1;
      strncpy(cfg.version_info_url, URL_Version_Info_Default, sizeof(URL_Version_Info_Default));
      strncpy(cfg.version_update_url, URL_Firmware_Default, sizeof(URL_Firmware_Default));
      strncpy(cfg.hostname, HOSTNAME, sizeof(HOSTNAME));
      strncpy(cfg.ota_pass_hash, OTA_PASS_HASH, sizeof(OTA_PASS_HASH));
      cfg.wifi_timeout = WIFI_TIMEOUT;
      cfg.wifi_delay = WIFI_DELAY;
      cfg.debug = 1;
      saveConfig(cfg);
  }

  ledMatrix.init();
  ledMatrix.setText("Connecting ...");
  WiFi.mode(WIFI_STA);
  //Initiate WiFi Connection
  WiFi.begin(cfg.SSID, cfg.password);
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout <= cfg.wifi_timeout)
  {
      timeout += cfg.wifi_delay;
      if (cfg.ota_update_enable)
      {
        delay_with_ota(cfg.wifi_delay);
      }
      else
      {
        delay(cfg.wifi_delay);
      }
  }

  if (cfg.ota_update_enable)
  {
    setup_arduino_ota(cfg.hostname, cfg.ota_pass_hash);
  }

  if (cfg.auto_update_enable)
  {
    OTA_CONFIG ota_config = {
      .version = cfg.firmwareVer,
      .check_url = cfg.version_info_url,
      .binary_url = cfg.version_update_url,
      .debug = (bool) cfg.debug,
    };

    //only checked on startup
    //you may want to implement some reboot trigger
    FirmwareUpdate(ota_config, &onFirmwareUpdateDone);
  }

  ledMatrix.setNextText("Connected to " + String(cfg.SSID));
  ledMatrix.setNextText(IpAddress2String(WiFi.localIP()));
  server.on("/data",HTTP_POST,dataHandler);
  //start web server
  server.begin();
  ledMatrix.setNextText("Webserver on: " + IpAddress2String(WiFi.localIP()));
  //At first start, read previous message from EEPROM
  message = readStringFromEEPROM(sizeof(eepromData_t)+1);
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
  if (cfg.ota_update_enable)
  {
    ArduinoOTA.handle();
  }
  delay(50);
}
