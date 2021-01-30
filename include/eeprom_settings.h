#include <Arduino.h>
#include <EEPROM.h>
#include <WString.h>

typedef struct {
    int initialized;                        // 0=no configuration, 1=valid configuration
    int magic;
    char SSID[31];                    // SSID of WiFi
    char password[31];                // Password of WiFi
    int auto_update_enable;
    int ota_update_enable;
    int firmwareVer;
    char version_info_url[120];
    char version_update_url[120];
    char ota_pass_hash[32];
    char hostname[30];
    int wifi_timeout;
    int wifi_delay;
    int debug;
} eepromData_t;

void eraseConfig(eepromData_t& cfg, unsigned int cfgStart = 0);

void saveConfig(eepromData_t& cfg, unsigned int cfgStart = 0);

void loadConfig(eepromData_t& cfg, unsigned int cfgStart = 0);
