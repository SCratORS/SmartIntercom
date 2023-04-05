#include "esphome.h"

#if defined(ESP32) && !defined(USE_ESP32_VARIANT_ESP32C3)
    #define LED 16
    #define FORMAT_FS_IF_FAILED true
#else
    #define LED 2
    #define FORMAT_FS_IF_FAILED
#endif

#if defined(SDCARD)
    #include "SD.h"
    #define aFS SD
    #define CRITICAL_FREE 0
#else
    #include "LittleFS.h"
    #define aFS LittleFS
    #define CRITICAL_FREE 300000
#endif

#include "FTPServer.h"

#define DEBUGf(format, ...) ESP_LOGD("FTP", format, ##__VA_ARGS__)
#define FIRMWARE_VERSION "1.7.5"

#if defined(ESP8266)
    #define DEBUGs(format, ...) ESP_LOGD("UPDATER", format, ##__VA_ARGS__)
    #define get_update_component(constructor) static_cast<FirmwareUpdate *> \
    (const_cast<custom_component::CustomComponentConstructor *>(&constructor)->get_component(0))
    #define startUpdate(FSComponent) get_update_component(FSComponent)->update_start()
    #define FIRMWARE_PATH "/firmware.bin"
    #define FIRMWARE_PATH_BAK "/firmware.bak"
    static void progressCallBack(size_t currSize, size_t totalSize) {
        digitalWrite(LED, !digitalRead(LED));
    }
    class FirmwareUpdate : public Component {
        public:
            FirmwareUpdate() {}

            void update_start() {         
                File firmware =  aFS.open(FIRMWARE_PATH, "r");
                if (firmware) {              
                    DEBUGs("Start update firmware.");
                    Update.onProgress(progressCallBack);
                    Update.begin(firmware.size(), U_FLASH);
                    Update.writeStream(firmware);
                    if (Update.end()) DEBUGs("Update finished.");
                    else DEBUGs("Update error.");
                    firmware.close();
                    if (!(aFS.remove(FIRMWARE_PATH) || aFS.rename(FIRMWARE_PATH, FIRMWARE_PATH_BAK))) DEBUGs("Can't delete or rename update file.");
                    delay(2000);
                    ESP.restart();
                } else {
                    DEBUGs("Firmware not found.");
                }
            }
    };
#endif

class FTPSrv : public Component {
    public:
        FTPSrv(const char* username, const char* password) : username_(username), password_(password) {}

        float get_setup_priority() const override { return esphome::setup_priority::AFTER_WIFI; }

        void setup() override {
            if (aFS.begin(FORMAT_FS_IF_FAILED)) {
                this->server_ = new FTPServer(aFS);
                this->server_->begin(this->username_, this->password_);
                DEBUGf("Server started.");
            } else {
                DEBUGf("File system could not be opened. Server will not work.");
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


class FSInfoSensor : public PollingComponent, public TextSensor {
    public:
        FSInfoSensor() : PollingComponent(60000) {}

        TextSensor *firmware_curent_version = new TextSensor();
        TextSensor *fs_info_sensor = new TextSensor();

        float get_setup_priority() const override { return esphome::setup_priority::AFTER_WIFI; }

        void setup() override {  
            this->mount_filesystem();
            #if defined(ESP8266)
                if (aFS.exists(FIRMWARE_PATH)) firmware_curent_version->publish_state("Доступно обновление");
                else firmware_curent_version->publish_state(FIRMWARE_VERSION);
            #else
                firmware_curent_version->publish_state(FIRMWARE_VERSION);
            #endif
        }

        void update() override {
            if (this->fs_initialized_) {
                char str[10] = "";
                double base = log(max(this->get_fs_used() - CRITICAL_FREE, (int64_t) 0)) / log(1024);
                uint8_t b_hi = round(base);
                sprintf(str, "%.2f %s", pow(1024, base - b_hi), this->sizes[b_hi]);
                fs_info_sensor->publish_state(to_string(str));
            } else fs_info_sensor->publish_state("Ошибка");
        }

    protected:
        const char *sizes[5] = { "B", "KB", "MB", "GB", "TB" };
        bool fs_initialized_{};

        void mount_filesystem() {
            if (!this->fs_initialized_) {
                this->fs_initialized_ = aFS.begin();
            }
        }

        int64_t get_fs_used() {
            #if defined(SDCARD) || defined(ESP32)
                return aFS.totalBytes() - aFS.usedBytes();
            #else
                FSInfo fs_info_;
                if (aFS.info(fs_info_)) return fs_info_.totalBytes - fs_info_.usedBytes; 
                else return 0; 
            #endif
        }
};
