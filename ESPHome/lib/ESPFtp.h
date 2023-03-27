#include "esphome.h"

#if defined(ESP32)
    #define FORMAT_FS_IF_FAILED true
#else
    #define FORMAT_FS_IF_FAILED
#endif
#if defined(SDCARD)
    #include "SD.h"
    #define aFS SD
#else
    #include "LittleFS.h"
    #define aFS LittleFS
#endif

#include "FTPServer.h"

#define DEBUGf(format, ...) ESP_LOGD("ftp", format "\n", ##__VA_ARGS__)

class ESPFtp : public Component {
public:
    ESPFtp(const char* username, const char* password) : username_(username), password_(password) {}

    float get_setup_priority() const override { return esphome::setup_priority::AFTER_WIFI; }

    void setup() override {
        if (aFS.begin(FORMAT_FS_IF_FAILED)) {
            this->server_ = new FTPServer(aFS);
            this->server_->begin(this->username_, this->password_);
            DEBUGf("FTP server started.");
        } else {
            DEBUGf("File system could not be opened. FTP server will not work.");
        }
    }

    void loop() override {
        if (this->server_) this->server_->handleFTP();
    }

protected:
    FTPServer * server_{};
    const char* username_;
    const char* password_;
};