#include "esphome.h"

#if defined(ESP32) && !defined(USE_ESP32_VARIANT_ESP32C3)
    #include "FS.h"
    #include "AudioOutputI2S.h"
    #define aAudioOutput() AudioOutputI2S(0, 1/*INTERNAL_DAC*/)
    #ifdef SDCARD
        #include "SD.h"
        #include "AudioFileSourceSD.h"
        #define aFS SD
        #define aFS_STR "SD";
        using aAudioFileSource = AudioFileSourceSD;
    #else
        #include "SPIFFS.h"
        #include "AudioFileSourceSPIFFS.h"
        #define aFS SPIFFS
        #define aFS_STR "SPIFFS"
        using aAudioFileSource = AudioFileSourceSPIFFS;
    #endif
#else
    #include "LittleFS.h"
    #include "AudioFileSourceLittleFS.h"
    #include "AudioOutputI2SNoDAC.h"

    #define aAudioOutput() AudioOutputI2SNoDAC()
    #define aFS LittleFS
    #define aFS_STR "LittleFS"
    using aAudioFileSource = AudioFileSourceLittleFS;
#endif

#include "AudioGeneratorMP3.h"
#include "AudioGeneratorWAV.h"
#include "AudioFileSourceHTTPStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioFileSourcePROGMEM.h"

#define DEBUGa(format, ...) ESP_LOGD("audio", format, ##__VA_ARGS__)

#define get_audio_component(constructor) static_cast<ESPAudio *> \
  (const_cast<custom_component::CustomComponentConstructor *>(&constructor)->get_component(0))

#define playFile(audioComponent, fname) get_audio_component(audioComponent)->play(fname)
#define playData(audioComponent, data) get_audio_component(audioComponent)->play_data(data, sizeof(data), ESPAudio::MP3)
#define stopFile(audioComponent) get_audio_component(audioComponent)->stop()
#define isPlaying(audioComponent) get_audio_component(audioComponent)->is_playing()

#define I2SO_DATA 2

class ESPAudio : public Component {
public:
    enum GeneratorType {WAV, MP3, UNK};

    void setup() override {
        this->fs_initialized_ = aFS.begin();
    }

    void loop() override {
        if (this->gen_ && this->gen_->isRunning()) {
            if (!this->gen_->loop()) {
                this->stop();
            }
        }
    }

    void stop() {
        if (this->gen_) {
            this->gen_->stop(); // also stops output (out) and file (src)
        }
        delete this->gen_;
        this->gen_ = nullptr;
        delete this->out_;
        this->out_ = nullptr;
        delete this->buf_;
        this->buf_ = nullptr;
        delete this->src_;
        this->src_ = nullptr;
        
    #ifdef ESP8266
        pinMode(I2SO_DATA, OUTPUT);
    #endif

        DEBUGa("Stopped playing");
    }

    bool play_file(const char *filename, GeneratorType type) {
        this->stop();
        return this->play_(this->open_file_(filename), type);
    }
    
    bool play_stream(const char *url, GeneratorType type) {
        this->stop();
        return this->play_(this->open_stream_(url), type);
    }        

    bool play_data(const uint8_t *data, size_t size, GeneratorType type) {
        this->stop();
        return this->play_(this->open_data_(data, size), type);
    }

    void play(const char *filename) {
        auto type = this->get_file_type_(filename);
        
        bool play_result = false;
        if (type == MP3) {
            play_result = this->is_stream_(filename) ? 
                this->play_stream(filename, type) : 
                this->play_file(filename, type);
        } else if (type == WAV) {
            play_result = this->is_stream_(filename) ? 
                this->play_stream(filename, type) : 
                this->play_file(filename, type);
        } else { 
            DEBUGa("Unknown audio file type %s", filename);
        }

        if (play_result) {
            DEBUGa("Started playing audio file %s", filename);
        } else { 
            DEBUGa("Failed to play audio file %s", filename);
            this->stop();
        }   
    }

    bool is_playing() {
        return this->gen_ && this->gen_->isRunning();
    }

protected:
    AudioOutput *out_{};
    AudioGenerator *gen_{};
    AudioFileSource *src_{};
    AudioFileSourceBuffer *buf_{};
    bool fs_initialized_{};

    AudioFileSource *open_file_(const char *filename) {
        if (!this->fs_initialized_) {
            // try mount again
            this->fs_initialized_ = aFS.begin();
            if (!this->fs_initialized_) {
                DEBUGa (aFS_STR " is not initialized");
                return nullptr;
            }
        }
        this->src_ = new aAudioFileSource();
        if (!this->src_->open(filename)) {
            DEBUGa("Can't open file %s", filename);
            return nullptr;
        }
        return this->src_;
    }

    AudioFileSource *open_stream_(const char *url) {
        this->src_ = new AudioFileSourceHTTPStream();
        if (!this->src_->open(url)) {
            DEBUGa("Can't open http stream %s", url);
            return nullptr;
        }
        this->buf_ = new AudioFileSourceBuffer(this->src_, 2048);
        return this->buf_;
    }

    AudioFileSource *open_data_(const uint8_t *data, size_t size) {
        this->src_ = new AudioFileSourcePROGMEM(data, size);
        if (!this->src_->isOpen()) {
            DEBUGa("Can't read data array");
            return nullptr;
        }
        return this->src_;
    }

    bool play_(AudioFileSource *src, GeneratorType type) {
        if (!src) {
            return false;
        }
        if (type == WAV) {
            this->gen_ = new AudioGeneratorWAV();
        } else if (type == MP3) {
            this->gen_ = new AudioGeneratorMP3();
        } else {
            return false;
        }
        this->out_ = new aAudioOutput();  
        return this->gen_->begin(src, this->out_);
    }

    const GeneratorType get_file_type_(const char *filename) {
        const char *dot = strrchr(filename, '.');
        if (!dot) {
            return UNK;
        }
        dot++;
        if ((dot[0] == 'w' || dot[0] == 'w') &&
            (dot[1] == 'a' || dot[1] == 'A') &&
            (dot[2] == 'v' || dot[2] == 'W') &&
            (dot[3] == 0)) {
            return WAV;
        }
        if ((dot[0] == 'm' || dot[0] == 'M') &&
            (dot[1] == 'p' || dot[1] == 'P') && 
            (dot[2] == '3') && 
            (dot[3] == 0)) {
            return MP3;
        }
        return UNK;
    }

    bool is_stream_(const char *filename) {
        const char *http = strstr(filename, "http://");
        const char *https = strstr(filename, "https://");
        return (http && (http - filename) == 0) || (https && (https - filename) == 0);
    }
};
