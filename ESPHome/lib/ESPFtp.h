#include "esphome.h"
#include "LittleFS.h"
#include "FTPServer.h"

#define DEBUGf(format, ...) ESP_LOGD("ftp", format "\n", ##__VA_ARGS__)

class ESPFtp : public Component {
    private:
        FTPServer * ftpSrv;
    public:
        ESPFtp(const char* username, const char* password) : ftpSrv(NULL) {
            if (LittleFS.begin()) this->ftpSrv = new FTPServer(LittleFS);
            if (this->ftpSrv) this->ftpSrv->begin (username, password);
            else DEBUGf ("File system could not be opened; ftp server will not work");
        }

        void loop() override {
            if (this->ftpSrv) this->ftpSrv->handleFTP ();  
        }    
};

