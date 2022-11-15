#define FS_NO_GLOBALS
#define SDCARD

#include "esphome.h"

#ifdef ESP32
    #include "FS.h"
    #include "AudioOutputI2S.h"

    #ifdef SDCARD
        #include "SD.h"
        #include "AudioFileSourceSD.h"
    #else
        #include "SPIFFS.h"
        #include "AudioFileSourceSPIFFS.h"
    #endif

#else
    #include "LittleFS.h"
    #include "AudioFileSourceLittleFS.h"
    #include "AudioOutputI2SNoDAC.h"
#endif

#include "AudioGeneratorMP3.h"
#include "AudioGeneratorWAV.h"
#include "AudioFileSourceHTTPStream.h"
#include "AudioFileSourceBuffer.h"

#define DEBUGa(format, ...) ESP_LOGD("audio", format "\n", ##__VA_ARGS__)

#define get_audio_component(constructor) static_cast<ESPAudio *> \
  (const_cast<custom_component::CustomComponentConstructor *>(&constructor)->get_component(0))

#define playFile(audioComponent, fname) get_audio_component(audioComponent)->play_file(fname)
#define isPlaying(audioComponent) get_audio_component(audioComponent)->isPlay()

#define I2SO_DATA 2

class ESPAudio : public Component {
    private:
        AudioGeneratorMP3 *mp3 = NULL;  
        AudioGeneratorWAV *wav = NULL;
        bool sd_init = false;
        
        #ifdef ESP32
            
            #ifdef SDCARD
                AudioFileSourceSD *file = NULL;
            #else
                AudioFileSourceSPIFFS *file = NULL;
            #endif
            AudioOutputI2S *out = NULL;
        #else
            AudioFileSourceLittleFS *file = NULL;
            AudioOutputI2SNoDAC *out = NULL;
        #endif
        
        AudioFileSourceHTTPStream *stream = NULL;
        AudioFileSourceBuffer *buff = NULL;

        void stop () {
            if (this->wav) this->wav->stop();
            if (this->mp3) this->mp3->stop();
            if (this->file) this->file->close();
            delete(this->wav); this->wav=NULL;
            delete(this->out); this->out=NULL;
            delete(this->file); this->file=NULL;
            delete(this->buff); this->buff=NULL;
            delete(this->stream); this->stream=NULL;
            
        #ifdef ESP8266
            pinMode(I2SO_DATA, OUTPUT);
        #endif
        
            DEBUGa("Stopped playing");
        }
        
        bool play_prepare(const char* fileName) {
            #ifdef ESP32
                this->out = new AudioOutputI2S(0, 1);
                #ifdef SDCARD
                    if (!sd_init && !SD.begin()) {
                        DEBUGa ("SD could not be opened!");
                        return false;
                    } else {
                        sd_init = true;
                        this->file = new AudioFileSourceSD(fileName);
                    }
                #else
                    this->file = new AudioFileSourceSPIFFS(fileName);
                #endif
            #else
                this->out = new AudioOutputI2SNoDAC();
                this->file = new AudioFileSourceLittleFS(fileName);
            #endif
            if (!this->file) {
                DEBUGa("Can't open audio file %s", fileName);
                return false;
            }
            return true;
        }

        bool start_file_wav(const char* fileName) {
            if (!play_prepare(fileName)) return false;  
            this->wav = new AudioGeneratorWAV();
            return(this->wav->begin(this->file, this->out));
        }

        bool start_file_mp3(const char* fileName) {
            if (!play_prepare(fileName)) return false;
            this->mp3 = new AudioGeneratorMP3();
            return(this->mp3->begin(this->file, this->out));
        }

        bool stream_prepare(const char* URL) {
            #ifdef ESP32
                this->out = new AudioOutputI2S(0, 1);
            #else
                this->out = new AudioOutputI2SNoDAC();
            #endif
            this->stream = new AudioFileSourceHTTPStream(URL);
            if (!this->stream) {
                DEBUGa("Can't open http stream %s", URL);
                return false;
            }
            this->buff = new AudioFileSourceBuffer(this->stream, 2048);
            return true;
        }

        bool start_stream_wav(const char* URL) {
            if (!stream_prepare(URL)) return false; 
            this->wav = new AudioGeneratorWAV();
            return(this->wav->begin(this->buff, this->out));
        }
        
        bool start_stream_mp3(const char* URL) {
            if (!stream_prepare(URL)) return false;
            this->mp3 = new AudioGeneratorMP3();
            return (this->mp3->begin(this->buff, this->out));
        }        

        const char *get_filename_ext(const char *filename) {
            const char *dot = strrchr(filename, '.');
            if (!dot) return "";
            return dot + 1;
        }
        

    public:
        ESPAudio() : mp3(NULL), wav(NULL), file(NULL), out(NULL),  stream(NULL), buff(NULL)  {}

        void setup() override {
            
        #ifdef ESP32
            #ifdef SDCARD
            #else
                if (!SPIFFS.begin()) DEBUGa ("SPIFFS could not be opened! Budet panika pri play audio files!");
            #endif
        #else
            if (!LittleFS.begin()) DEBUGa ("LITTLEFS could not be opened! Budet panika pri play audio files!");
        #endif
        }
        
        void loop() override {
            if (this->wav && this->wav->isRunning()) {
                if (!this->wav->loop()) stop();
            }
            if (this->mp3 && this->mp3->isRunning()) {
                if (!this->mp3->loop()) stop();
            }
        }
    
        void play_file(const char* fName) {
            if (this->file || this->stream) stop();
            bool play_result = false;
            const char *fext = get_filename_ext(fName);
            char * http = strstr(fName, "http://");
            char * https = strstr(fName, "https://");
            bool play_stream = (http && (http-fName)==0) || (https && (https-fName==0));
            if (strcmp(fext, "mp3") == 0) play_result = play_stream?start_stream_mp3(fName):start_file_mp3(fName);
            else if (strcmp(fext, "wav") == 0) play_result = play_stream?start_stream_wav(fName):start_file_wav(fName);
            else DEBUGa("Unknown audio file type %s", fName);
            if (play_result) DEBUGa("Started playing audio file %s", fName);
            else { DEBUGa("Failed to play audio file %s", fName);stop();}   
        }

        bool isPlay() {
            return (this->wav && this->wav->isRunning()) || (this->mp3 && this->mp3->isRunning());
        }
    
};
