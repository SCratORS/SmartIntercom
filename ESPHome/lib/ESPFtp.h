#include "esphome.h"

#ifdef ESP32
    #ifdef USE_ESP32_VARIANT_ESP32C3
        #include "LittleFS.h"
    #else
        #include "FS.h"
        #ifdef SDCARD
            #include "SD.h"
        #else
            #include "SPIFFS.h"
        #endif
    #endif
#else
    #include "LittleFS.h"
#endif

#include "FTPServer.h"

#define DEBUGf(format, ...) ESP_LOGD("ftp", format "\n", ##__VA_ARGS__)

class ESPFtp : public Component {
    private:
        FTPServer * ftpSrv;
    public:
        ESPFtp(const char* username, const char* password) : ftpSrv(NULL) {
        #ifdef ESP32
            #ifdef USE_ESP32_VARIANT_ESP32C3
                if (LittleFS.begin()) this->ftpSrv = new FTPServer(LittleFS);
            #else
                #ifdef SDCARD
                    if (SD.begin()) this->ftpSrv = new FTPServer(SD);
                #else
                    if (SPIFFS.begin()) this->ftpSrv = new FTPServer(SPIFFS);
                #endif
            #endif
        #else
            if (LittleFS.begin()) this->ftpSrv = new FTPServer(LittleFS);
        #endif
        
            if (this->ftpSrv) this->ftpSrv->begin (username, password);
            else DEBUGf ("File system could not be opened; ftp server will not work");
        }

        void loop() override {
            if (this->ftpSrv) this->ftpSrv->handleFTP ();  
            
        }    
};

