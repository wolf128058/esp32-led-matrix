#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

struct OTA_CONFIG {
    const int version;
    const char* check_url;
    const char* binary_url;
    const bool debug;
};

void FirmwareUpdate(OTA_CONFIG config, void (*onUpdateDoneCallback)(unsigned int));
