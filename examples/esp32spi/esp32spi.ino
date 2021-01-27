#include <SPI.h>
#include "LedMatrix.h"

#include <WiFi.h>
#include <WiFiClient.h>
const char ssid[] = "Freifunk"; // WiFi name
const char password[] = ""; // WiFi password

#include <NTPClient.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

#define NUMBER_OF_DEVICES 4 //number of led matrix connect in series
#define CS_PIN 15
#define CLK_PIN 14
#define MISO_PIN 2 //we do not use this pin just fill to match constructor
#define MOSI_PIN 12

LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
enum {TIME, DATE};
boolean displayMode = TIME;
long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
String formattedDate;
String timeStamp, hour, minute, second;
String dateStamp, year, month, date;


String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ;
}
  
void setup() {
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
  timeClient.begin();
  timeClient.setTimeOffset(3600); // Set offset time in seconds, GMT+1 = 3600 */
  displayMode = DATE;
}

void loop() {
  
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }

  if (displayMode == DATE) {
    displayMode = TIME;
  }

  currentMillis = millis();
  if (currentMillis - previousMillis > interval && displayMode == TIME) {
    previousMillis = millis();
    formattedDate = timeClient.getFormattedTime();
    ledMatrix.setNextText(formattedDate);
  }

  
  ledMatrix.clear();
  ledMatrix.scrollTextLeft();
  ledMatrix.drawText();
  ledMatrix.commit();
  delay(50);
}
