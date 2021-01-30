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
        HTTPClient http;
        int firmwareVersionNew = 0;
        http.begin(config.check_url);     // Webseite aufrufen
        int httpCode = http.GET();            // Antwort des Servers einlesen
        if (httpCode == HTTP_CODE_OK)         // Wenn Antwort OK
        {
            String payload = http.getString();  // Webseite einlesen
            firmwareVersionNew = payload.toInt();      // Zahl aus Sting bilden
        }
        http.end();

        if (firmwareVersionNew > config.version)        // Firmwareversion mit aktueller vergleichen
        {
            if (config.debug)
            {
                Serial.println("Neue Firmware verfuegbar");
                Serial.println("Starte Download");
            }
            ESPhttpUpdate.rebootOnUpdate(false);// reboot abschalten, wir wollen erst Meldungen ausgeben
            t_httpUpdate_return ret = ESPhttpUpdate.update(config.binary_url);
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
                    ESP.reset();
                    delay(100);
                    break;
                default:
                    if (config.debug)
                    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                    break;
            }
        }
    }
}
