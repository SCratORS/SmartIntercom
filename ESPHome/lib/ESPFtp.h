#include "esphome.h"
//#define SDCARD

#define FORMAT_FS_IF_FAILED true




#ifdef ESP32
    #include "WiFi.h"
    #ifdef USE_ESP32_VARIANT_ESP32C3
        #include "LittleFS.h"
    #else
        #include "FS.h"
        #ifdef SDCARD
            #include "SD.h"
        #else
            #include "LittleFS.h"
        #endif
    #endif
#else
    #include "ESP8266WiFi.h"
    #include "LittleFS.h"
#endif

#include "FTPServer.h"

#define DEBUGf(format, ...) ESP_LOGD("ftp", format "\n", ##__VA_ARGS__)

class ESPFtp : public Component {
    private:
        FTPServer * ftpSrv;
        const char* uname;
        const char* passwd;
        bool ftp_initialized_{};
        bool fs_initialized_{};
    public:
        ESPFtp(const char* username, const char* password) : ftpSrv(NULL), uname(username), passwd(password) {
            this->ftp_initialized_ = false;
            #ifdef ESP32
                #ifdef USE_ESP32_VARIANT_ESP32C3
                    this->fs_initialized_ = LittleFS.begin(FORMAT_FS_IF_FAILED);
                #else
                    #ifdef SDCARD
                        this->fs_initialized_ = SD.begin();
                    #else
                        this->fs_initialized_ = LittleFS.begin(FORMAT_FS_IF_FAILED);
                    #endif
                #endif
            #else
                this->fs_initialized_ = LittleFS.begin();
            #endif
        }

        void initialize() {
            #ifdef ESP32
                #ifdef USE_ESP32_VARIANT_ESP32C3
                    if (this->fs_initialized_) this->ftpSrv = new FTPServer(LittleFS);
                #else
                    #ifdef SDCARD
                        if (this->fs_initialized_) this->ftpSrv = new FTPServer(SD);
                    #else
                        if (this->fs_initialized_) this->ftpSrv = new FTPServer(LittleFS);
                    #endif
                #endif
            #else
                if (this->fs_initialized_) this->ftpSrv = new FTPServer(LittleFS);
            #endif
            if (this->ftpSrv) {
                this->ftpSrv->begin (uname, passwd);
                this->ftp_initialized_ = true;
            }
            else DEBUGf ("File system could not be opened; ftp server will not work");
        }

        void loop() override {
            if (!this->ftp_initialized_) {
                if (WiFi.status() == WL_CONNECTED) this->initialize();
            } else if (this->ftpSrv) this->ftpSrv->handleFTP();
        }    
};

