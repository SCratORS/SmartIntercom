#include "esphome.h"
#include "LittleFS.h"
#include "AudioFileSourceLittleFS.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2SNoDAC.h"

#define DEBUGf(format, ...) ESP_LOGD("audio", format "\n", ##__VA_ARGS__)

#define get_component(constructor) static_cast<ESPAudio *> \
  (const_cast<custom_component::CustomComponentConstructor *>(&constructor)->get_component(0))

#define playFile(audioComponent, fname) get_component(audioComponent)->play(fname)
#define isPlaying(audioComponent) get_component(audioComponent)->isPlay()

#define I2SO_DATA 2

class ESPAudio : public Component {
    private:
        AudioGeneratorWAV *wav;
        AudioFileSourceLittleFS *file;
        AudioOutputI2SNoDAC *out;

        void stop () {
            this->wav->stop();
            delete(this->wav); this->wav=NULL;
            delete(this->out); this->out=NULL;
            delete(this->file); this->file=NULL;
            pinMode(I2SO_DATA, OUTPUT);
            DEBUGf("Stopped playing");
        }
        
        bool start(const char* fileName) {
            this->file = new AudioFileSourceLittleFS(fileName);
            if (!this->file) {
                DEBUGf("Can't find file %s", fileName);
                return false;
            }
            this->out = new AudioOutputI2SNoDAC();
            this->wav = new AudioGeneratorWAV();
            if(this->wav->begin(this->file, this->out)) {
                DEBUGf("Started playing WAV file %s", fileName);
                return true;
            } else {
                DEBUGf("Failed to play WAV file %s", fileName);
                stop();
                return false;
            }
        }

    
    public:
        ESPAudio() : wav(NULL), file(NULL), out(NULL) {}

        void setup() override {
            LittleFS.begin();
        }
        
        void loop() override {
            if (this->wav && this->wav->isRunning()) {
                if (!this->wav->loop()) stop();
            }
        }
    
        void play(const char* fName) {
            if(this->file) stop();
            start(fName);
        }
        
        bool isPlay() {
            return this->wav && this->wav->isRunning();
        }
    
};

