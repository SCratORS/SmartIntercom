#include "esphome.h"

#ifdef ESP32
    #include "FS.h"
    #include "SPIFFS.h"
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
            if (SPIFFS.begin()) this->ftpSrv = new FTPServer(SPIFFS);
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

