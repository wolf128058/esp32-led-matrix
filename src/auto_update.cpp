#include <ArduinoJson.h>
#include "auto_update.h"

void FirmwareUpdate(OTA_CONFIG config, void (*onUpdateDoneCallback)(unsigned int) = NULL)
{
    // Show current version in debug
    if (config.debug)
    {
        char buffer [20];
        sprintf (buffer, "version: %d", config.version);
        Serial.println(buffer);
    }

    // Initiate WiFi Connection
    int mytimeout = 0;
    int mydelay = 500;
    int wifi_timeout = 60000;
    while (WiFi.status() != WL_CONNECTED && mytimeout <= wifi_timeout && WiFi.localIP()[3] == 0)
    {
        Serial.println("Waiting for Wifi (Update)");
        mytimeout += mydelay;
        delay(mydelay);
    }

    // No wifi - can't do update
    if (WiFi.status() != WL_CONNECTED)
    {
        if (config.debug)
        Serial.println("no wifi connection - no updates");
        return;
    }
    else
    {
        // Check Firmware-Version on Server
        int firmwareVersionNew = 0;
        HTTPClient http;
        #ifdef ESP8266
        // Call Url
        http.begin(config.check_url);
        #else
        WiFiClientSecure client;
        client.setInsecure();
        // Call Url
        http.begin(client, config.check_url);
        #endif
        // Read Answer
        int httpCode = http.GET();
        String httpLocation = http.getLocation();
        // Loop from Redirection to Redirection
        while (httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND )
        {
            Serial.println("Redirection to : " + httpLocation);
            HTTPClient http;
            #ifdef ESP8266
            http.begin(httpLocation);
            #else
            http.begin(client, httpLocation);
            #endif
            httpCode = http.GET();
            httpLocation = http.getLocation();
        }

        // If Response is OK (200)
        if (httpCode == HTTP_CODE_OK)
        {
            DynamicJsonDocument doc(8192);
            // Read String from Website
            String payload = http.getString();
            // Serial.println("Payload: " + String(payload));
            deserializeJson(doc, payload);
            const char* tag_name = doc["tag_name"];
            // Cast Integer of Payload
            firmwareVersionNew =  String(tag_name).toInt();
            Serial.println("Version: " + String(firmwareVersionNew));
        }
        else
        {
            Serial.println("Version file not found.");
            Serial.println("URL:           " + String(config.check_url));
            Serial.println("HTTP-Status:   " + String(httpCode));
            Serial.println("HTTP-Location: " + String(httpLocation));
        }
        http.end();

        // Compare Fimware-Version online with local one
        if (firmwareVersionNew > config.version)
        {
            if (config.debug)
            {
                Serial.println("Neue Firmware verfuegbar: " + String(firmwareVersionNew));
                Serial.println("Starte Download");
            }
            #ifdef ESP8266
            // Suppress Reboot on Update for beeing able to read messages first
            ESPhttpUpdate.rebootOnUpdate(false);
            t_httpUpdate_return ret = ESPhttpUpdate.update(config.binary_url);
            #else
            // Suppress Reboot on Update for beeing able to read messages first
            httpUpdate.rebootOnUpdate(false);
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
