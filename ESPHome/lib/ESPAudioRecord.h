//Не используй этот файл. Он для тестирования и врядли рабочий.
#include "esphome.h"
#include "LittleFS.h"
#include <ESP8266AudioRecord.h>

#define DEBUGr(format, ...) ESP_LOGD("audio_record", format "\n", ##__VA_ARGS__)

#define get_record_component(constructor) static_cast<ESPAudioRecord *> \
  (const_cast<custom_component::CustomComponentConstructor *>(&constructor)->get_component(0))

#define StartRecord(AudioRecordComponent, fname) get_record_component(AudioRecordComponent)->RecordStart(fname)
#define StopRecord(AudioRecordComponent) get_record_component(AudioRecordComponent)->RecordStop()
#define isRecording(AudioRecordComponent) get_record_component(AudioRecordComponent)->Recording()

class ESPAudioRecord : public Component {
    private:
      
      ESP8266AudioRecord *audio_record;  
      bool record_init = false;
      bool record_start = false;  

    public:
        ESPAudioRecord(): audio_record(NULL) {}

        void setup() override {
            if (LittleFS.begin()) {
              this->audio_record = ESP8266AudioRecord::getInstance();
              this->audio_record->init(40000);
              this->record_init = true;
            } else DEBUGr ("LITTLEFS could not be opened! Budet panika pri play audio files!");
        }

        void loop() override {
          if (this->record_start) {
            if (this->audio_record->Recording()) this->audio_record->RecordHandle();
            else {
              DEBUGr("Record complete.");
              this->record_start = false;
            }
          }
        }

        void RecordStart(const String &fname) {
          DEBUGr("Start Record in file: %s", fname);
          if (this->record_init) {
            this->record_start = this->audio_record->RecordStart(fname);
            if (!this->record_start) DEBUGr("Failed start record.");
          } else DEBUGr("Record not init.");
        }

        void RecordStop() {
          this->audio_record->RecordStop();
        }

        bool Recording() {
          return this->record_start;
        }
};
