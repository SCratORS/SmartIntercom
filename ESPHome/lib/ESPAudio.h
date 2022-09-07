#include "esphome.h"
#include "LittleFS.h"
#include "AudioFileSourceLittleFS.h"
#include "AudioOutputI2SNoDAC.h"

#include "AudioGeneratorMP3.h"
#include "AudioGeneratorWAV.h"
#include "AudioFileSourceHTTPStream.h"
#include "AudioFileSourceBuffer.h"

#define DEBUGa(format, ...) ESP_LOGD("audio", format "\n", ##__VA_ARGS__)

void statusCBFn(void *cbData, int code, const char *message) {
        // PROGMEM vs RAM
        if (message >= (const char *)0x40000000) {
                char *s=(char *) alloca(strlen_P(message));
                strcpy_P(s, message);
                message=s;
        }
        DEBUGa("'%s': code:%d statusCBFn: %s", (char *) cbData, code, message);
}

#define get_audio_component(constructor) static_cast<ESPAudio *> \
  (const_cast<custom_component::CustomComponentConstructor *>(&constructor)->get_component(0))

#define playFile(audioComponent, fname) get_audio_component(audioComponent)->play_file(fname)
#define isPlaying(audioComponent) get_audio_component(audioComponent)->isPlay()

#define I2SO_DATA 2

class ESPAudio : public Component {
    private:
        AudioGeneratorMP3 *mp3;  
        AudioGeneratorWAV *wav;
        AudioFileSourceLittleFS *file;
        AudioOutputI2SNoDAC *out;       
        AudioFileSourceHTTPStream *stream;
        AudioFileSourceBuffer *buff;

        void stop () {
            if (this->wav) this->wav->stop();
            if (this->mp3) this->mp3->stop();
            delete(this->wav); this->wav=NULL;   
            delete(this->mp3); this->mp3=NULL;   
            delete(this->out); this->out=NULL;
            delete(this->file); this->file=NULL;
            delete(this->buff); this->buff=NULL;
            delete(this->stream); this->stream=NULL;            
            pinMode(I2SO_DATA, OUTPUT);
            DEBUGa("Stopped playing");
        }
        
        bool play_prepare(const char* fileName) {
            this->out = new AudioOutputI2SNoDAC();
            this->out->RegisterStatusCB(statusCBFn, (void *) "AudioOutputI2SNoDAC");
            this->file = new AudioFileSourceLittleFS(fileName);
            this->file->RegisterStatusCB(statusCBFn, (void *) "AudioFileSourceLittleFS");
            if (!this->file) {
                DEBUGa("Can't open audio file %s", fileName);
                return false;
            }
            return true;
        }

        bool start_file_wav(const char* fileName) {
            if (!play_prepare(fileName)) return false;
            this->wav = new AudioGeneratorWAV();
            this->wav->RegisterStatusCB(statusCBFn, (void *) "AudioGeneratorWAV");
            return(this->wav->begin(this->file, this->out));
        }

        bool start_file_mp3(const char* fileName) {
            if (!play_prepare(fileName)) return false;   
            this->mp3 = new AudioGeneratorMP3();
            this->mp3->RegisterStatusCB(statusCBFn, (void *) "AudioGeneratorMP3");
            return(this->mp3->begin(this->file, this->out));
        }

        bool stream_prepare(const char* URL) {
            this->stream = new AudioFileSourceHTTPStream(URL);
            this->stream->RegisterStatusCB(statusCBFn, (void *) "AudioFileSourceHTTPStream");
            if (!this->stream) {
                DEBUGa("Can't open http stream %s", URL);
                return false;
            }
            this->buff = new AudioFileSourceBuffer(this->stream, 2048);
            this->buff->RegisterStatusCB(statusCBFn, (void *) "AudioFileSourceBuffer");  
            this->out = new AudioOutputI2SNoDAC();
            this->out->RegisterStatusCB(statusCBFn, (void *) "AudioOutputI2SNoDAC");
            return true;
        }

        bool start_stream_wav(const char* URL) {
            if (!stream_prepare(URL)) return false;   
            this->wav = new AudioGeneratorWAV();
            this->wav->RegisterStatusCB(statusCBFn, (void *) "AudioGeneratorWAV");
            return(this->wav->begin(this->buff, this->out));
        }
        
        bool start_stream_mp3(const char* URL) {
            if (!stream_prepare(URL)) return false;
            this->mp3 = new AudioGeneratorMP3();
            this->mp3->RegisterStatusCB(statusCBFn, (void *) "AudioGeneratorMP3");
            return (this->mp3->begin(this->buff, this->out));
        }        

        const char *get_filename_ext(const char *filename) {
            const char *dot = strrchr(filename, '.');
            if (!dot) return "";
            return dot + 1;
        }
        

    public:
        ESPAudio() : mp3(NULL), wav(NULL), file(NULL), out(NULL), stream(NULL), buff(NULL)  {}

        void setup() override {
            if (!LittleFS.begin()) DEBUGa ("LITTLEFS could not be opened! Budet panika pri play audio files!");
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
            if(this->file || this->stream) stop();
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
