#include "esphome.h"
#include "LittleFS.h"
#include "FTPServer.h"

#define DEBUGf(format, ...) ESP_LOGD("ftp", format "\n", ##__VA_ARGS__)

class ESPFtp : public Component {
    private:
        FTPServer ftpSrv = FTPServer(LittleFS);
    public:
        ESPFtp(const char* username, const char* password) {
            if (LittleFS.begin()) {
                DEBUGf ("FTP Server start");
                ftpSrv.begin (username, password);
            } else {
                DEBUGf ("File system could not be opened; ftp server will not work");
            }
        }

        void loop() override {
            ftpSrv.handleFTP ();  
        }    
};

