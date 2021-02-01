#include "auto_update.h"

void FirmwareUpdate(OTA_CONFIG config, void (*onUpdateDoneCallback)(unsigned int) = NULL)
{
    //show current version in debug
    if (config.debug)
    {
        char buffer [20];
        sprintf (buffer, "version: %d", config.version);
        Serial.println(buffer);
    }

    //Initiate WiFi Connection
    int mytimeout = 0;
    int mydelay = 500;
    int wifi_timeout = 60000;
    while (WiFi.status() != WL_CONNECTED && mytimeout <= wifi_timeout && WiFi.localIP()[3] == 0)
    {
        Serial.println("Waiting for Wifi (Update)");
        mytimeout += mydelay;
        delay(mydelay);
    }

    //no wifi - can't do
    if (WiFi.status() != WL_CONNECTED)
    {
        if (config.debug)
        Serial.println("no wifi connection - no updates");
        return;
    }
    else
    {
        // Überprüfen der Firmwareversion des programmms aud dem Server
        int firmwareVersionNew = 0;
        HTTPClient http;
        #ifdef ESP8266
        http.begin(config.check_url);     // Webseite aufrufen
        #else
        WiFiClientSecure client;
        client.setInsecure();
        http.begin(client, config.check_url);     // Webseite aufrufen
        #endif
        int httpCode = http.GET();            // Antwort des Servers einlesen
        String httpLocation = http.getLocation();
        while (httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND ) // Wenn Antwort Weiterleitung
        {
            Serial.println("Redirection to : " + httpLocation);
            HTTPClient http;
            http.begin(client, httpLocation);
            httpCode = http.GET();
            httpLocation = http.getLocation();
        }

        if (httpCode == HTTP_CODE_OK)         // Wenn Antwort OK
        {
            String payload = http.getString();  // Webseite einlesen
            firmwareVersionNew = payload.toInt();      // Zahl aus Sting bilden
        }
        else
        {
            Serial.println("Version file not found.");
            Serial.println("HTTP-Status:   " + String(httpCode));
            Serial.println("HTTP-Location: " + String(httpLocation));
        }
        http.end();

        if (firmwareVersionNew > config.version)        // Firmwareversion mit aktueller vergleichen
        {
            if (config.debug)
            {
                Serial.println("Neue Firmware verfuegbar");
                Serial.println("Starte Download");
            }
            #ifdef ESP8266
            ESPhttpUpdate.rebootOnUpdate(false);// reboot abschalten, wir wollen erst Meldungen ausgeben
            t_httpUpdate_return ret = ESPhttpUpdate.update(config.binary_url);
            #else
            httpUpdate.rebootOnUpdate(false);// reboot abschalten, wir wollen erst Meldungen ausgeben
            t_httpUpdate_return ret = httpUpdate.update( client, config.binary_url );
            #endif
            switch (ret)
            {
                case HTTP_UPDATE_OK:
                    if (config.debug)
                    {
                        Serial.println("Update erfolgreich");
                    }
                    //call the callback function
                    (*onUpdateDoneCallback)(firmwareVersionNew);
                    if (config.debug)
                    {
                        Serial.println("Reset");
                        Serial.flush();
                    }
                    delay(1);
                    #ifdef ESP8266
                    ESP.reset();
                    #else
                    ESP.restart();
                    #endif
                    delay(100);
                    break;
                default:
                    if (config.debug)
                    #ifdef ESP8266
                    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                    #else
                    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
                    #endif
                    break;
            }
        }
    }
}
