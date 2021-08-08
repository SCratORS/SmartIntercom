#include "esphome.h"
#include "LittleFS.h"
#include "ESPFtpServer.h"

#define DEBUGf(format, ...) ESP_LOGD("ftp", format "\n", ##__VA_ARGS__)

class ESPFtp : public Component {
    private:

    public:
        FtpServer ftpSrv;
        
        void setup() override {
            if (LittleFS.begin()) {
                DEBUGf ("FTP Server start");
                ftpSrv.begin ("esp", "esp");
            } else {
                DEBUGf ("File system could not be opened; ftp server will not work");
            }
        }
        
        void loop() override {
            ftpSrv.handleFTP (LittleFS);  
        }    
};

