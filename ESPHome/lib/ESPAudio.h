#include "esphome.h"
#include "LittleFS.h"
#include "AudioFileSourceLittleFS.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2SNoDAC.h"
#include "AudioFileSourceHTTPStream.h"
#include "AudioFileSourceBuffer.h"

#define DEBUGa(format, ...) ESP_LOGD("audio", format "\n", ##__VA_ARGS__)

#define get_component(constructor) static_cast<ESPAudio *> \
  (const_cast<custom_component::CustomComponentConstructor *>(&constructor)->get_component(0))

#define playFile(audioComponent, fname) get_component(audioComponent)->play_file(fname)
#define playStream(audioComponent, furl) get_component(audioComponent)->play_stream(furl)
#define isPlaying(audioComponent) get_component(audioComponent)->isPlay()

#define I2SO_DATA 2

class ESPAudio : public Component {
    private:
        AudioGeneratorWAV *wav;
        AudioFileSourceLittleFS *file;
        AudioFileSourceHTTPStream *stream;
        AudioOutputI2SNoDAC *out;
        AudioFileSourceBuffer *buff;

        void stop () {
            this->wav->stop();
            delete(this->wav); this->wav=NULL;
            delete(this->out); this->out=NULL;
            delete(this->file); this->file=NULL;
            delete(this->buff); this->buff=NULL;
            delete(this->stream); this->stream=NULL;
            pinMode(I2SO_DATA, OUTPUT);
            DEBUGa("Stopped playing");
        }
        
        bool start_file(const char* fileName) {
            this->file = new AudioFileSourceLittleFS(fileName);
            if (!this->file) {
                DEBUGa("Can't find file %s", fileName);
                return false;
            }
            this->out = new AudioOutputI2SNoDAC();
            this->wav = new AudioGeneratorWAV();
            if(this->wav->begin(this->file, this->out)) {
                DEBUGa("Started playing WAV file %s", fileName);
                return true;
            } else {
                DEBUGa("Failed to play WAV file %s", fileName);
                stop();
                return false;
            }
        }

        bool start_stream(const char* URL) {
            this->stream = new AudioFileSourceHTTPStream(URL);
            this->buff = new AudioFileSourceBuffer(this->stream, 2048);
            this->out = new AudioOutputI2SNoDAC();
            this->wav = new AudioGeneratorWAV();
            if(this->wav->begin(this->buff, this->out)) {
                DEBUGa("Started playing URL stream %s", URL);
                return true;
            } else {
                DEBUGa("Failed to play URL stream %s", URL);
                stop();
                return false;
            }
        }

    
    public:
        ESPAudio() : wav(NULL), file(NULL), out(NULL), buff(NULL), stream(NULL) {}

        void setup() override {
            LittleFS.begin();
        }
        
        void loop() override {
            if (this->wav && this->wav->isRunning()) {
                if (!this->wav->loop()) stop();
            }
        }
    
        void play_file(const char* fName) {
            if(this->file || this->stream) stop();
            start_file(fName);
        }

        void play_stream(const char* fURL) {
            if(this->file || this->stream) stop();
            start_stream(fURL);
        }
        
        bool isPlay() {
            return this->wav && this->wav->isRunning();
        }
    
};

