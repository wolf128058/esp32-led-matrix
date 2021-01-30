#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#else
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#endif

struct OTA_CONFIG {
    const int version;
    const char* check_url;
    const char* binary_url;
    const bool debug;
};

void FirmwareUpdate(OTA_CONFIG config, void (*onUpdateDoneCallback)(unsigned int));
