#define CONFIG_CHIP_DEVICE_PRODUCT_NAME "SmartIntercom"
#define CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION "Bluestreak 1.0.0-Web Insider Preview 11.2023"
#define DISCOVERY_DELAY 500
#define led_status    16        // Индикатор статуса API, GPIO2 - это встроенный синий светодиод на ESP12
#define led_indicator 13        // Дополнительный индикатор, который будет показывать режимы и прочее.
#define detect_line   12        // Пин детектора вызова
#define button_boot   0         // Кнопка управления платой и перевода в режим прошивки
#define relay_line    14        // Пин "Переключение линии, плата/трубка"
#define switch_open   17        // Пин "Открытие двери"
#define switch_phone  4         // Пин "Трубка положена/поднята"
#define ACCEPT_FILENAME "/user_access_allowed.mp3" 
#define REJECT_FILENAME "/access_denied.mp3" 
#define DELIVERY_FILENAME "/delivery_access_allowed.mp3" 

#define l_status_call "Вызов"
#define l_status_answer "Ответ"
#define l_status_open "Открытие двери"
#define l_status_reject "Сброс вызова"
#define l_status_close "Закрыто"
#define SSL_RESPONSE_TIMEOUT 30000
#define STACK_SIZE 10000
#define TIME_SERVER "pool.ntp.org"
#include "nvs_flash.h"
#include "nvs.h"
#include "soc/rtc_wdt.h"
#include "SPIFFS.h"
#include "SD.h"
#include "AudioOutputI2S.h"
#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorMP3.h" 
#include <EEPROM.h>
#include <list>
#include <map>
#include <PubSubClient.h>
#include <GyverPortal.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <FTPServer.h>

String mqtt_entity_id;
const String dev_name = "smartintercom";
WiFiClientSecure client;
UniversalTelegramBot bot("", client);
WiFiClient ns_client;
PubSubClient mqtt_client(ns_client);
FTPServer *ftpSrv;
GyverPortal webUI_(&SPIFFS);

struct DevInfo {
    String name;
    String manufacturer;
    String product;
    String firmware;
    String control;
};


class Entity {
  public:
    String name;
    String friendly_name;
    String short_name;
    String ent_cat;
    String ic;
    String callback_topic;
    String state_topic;
    uint8_t category;
    PubSubClient* client;
    const String stat_t = "/state";
    const String cmd_t = "/set";
    Entity(String name, PubSubClient* client) {this->name = name;this->client = client;}
    virtual void callback(String message) {}
    virtual void mqttDiscovery(DevInfo* dev_info) {}
    virtual void webUpdate() {}
};

class Sensor: public Entity {
  public:
    String value = ""; 
    Sensor(String name, PubSubClient* client):Entity(name, client){}   
   
    void setValue(String value) {
      this->value = value;
      this->client->publish(this->state_topic.c_str(), this->value.c_str());
    }
    void webUpdate() {webUI_.updateString(this->name, this->value);}

    void mqttDiscovery(DevInfo* dev_info) {
      String name = this->name;
      String home_topic = "homeassistant/sensor/" + dev_name + "/" + name;
      String discovery_topic = home_topic + "/config";
      DynamicJsonDocument doc(1024);
      String txt;
      DynamicJsonDocument json(1024);
      json["~"] = home_topic;
      json["name"] = this->friendly_name==""?name:this->friendly_name;
      json["obj_id"] = dev_name + "_" + name;
      json["uniq_id"] = dev_name + "_" + name;
      json["stat_t"] = "~"+this->stat_t;
      json["avty_t"] = dev_name + "/status";
      if (this->ic != "") json["ic"] = this->ic;
      if (this->ent_cat != "") json["ent_cat"] = this->ent_cat;
      JsonObject dev = json.createNestedObject("dev");
      dev["ids"] = mqtt_entity_id;
      dev["name"] = dev_info->name;
      dev["sw"] = dev_info->firmware;
      dev["mdl"] = dev_info->product;
      dev["mf"] = dev_info->manufacturer;
      dev["cu"] = dev_info->control;
      serializeJson(json, txt);
      this->state_topic = home_topic + this->stat_t;
      this->client->beginPublish(discovery_topic.c_str(), txt.length(), false);
      this->client->print(txt);
      this->client->endPublish();
      delay(DISCOVERY_DELAY); 
      setValue(value);
    }
};

class BinarySensor: public Entity {
  public:
    bool value = false; 
    BinarySensor(String name, PubSubClient* client):Entity(name, client){}    
   
    void setValue(bool value) {
      this->value = value;
      this->client->publish(this->state_topic.c_str(), value?"ON":"OFF");
    }

    void webUpdate() {webUI_.updateBool(this->name, this->value);}

    void mqttDiscovery(DevInfo* dev_info) {
      String name = this->name;
      String home_topic = "homeassistant/binary_sensor/" + dev_name + "/" + name;
      String discovery_topic = home_topic + "/config";
      DynamicJsonDocument doc(1024);
      String txt;
      DynamicJsonDocument json(1024);
      json["~"] = home_topic;
      json["name"] = this->friendly_name==""?name:this->friendly_name;
      json["obj_id"] = dev_name + "_" + name;
      json["uniq_id"] = dev_name + "_" + name;
      json["stat_t"] = "~"+this->stat_t;
      json["avty_t"] = dev_name + "/status";
      if (this->ic != "") json["ic"] = this->ic;
      if (this->ent_cat != "") json["ent_cat"] = this->ent_cat;
      JsonObject dev = json.createNestedObject("dev");
      dev["ids"] = mqtt_entity_id;
      dev["name"] = dev_info->name;
      dev["sw"] = dev_info->firmware;
      dev["mdl"] = dev_info->product;
      dev["mf"] = dev_info->manufacturer;
      dev["cu"] = dev_info->control;
      serializeJson(json, txt);
      this->state_topic = home_topic + this->stat_t;
      this->client->beginPublish(discovery_topic.c_str(), txt.length(), false);
      this->client->print(txt);
      this->client->endPublish();
      delay(DISCOVERY_DELAY); 
      setValue(value);
    }
};

class Switch: public Entity {
  public:
    bool value = false; 
    Switch(String name, PubSubClient* client):Entity(name, client){}  
    void setValue(bool value) {
      this->value = value;
      this->client->publish(this->state_topic.c_str(), value?"ON":"OFF");
    } 
    bool getValue() {return this->value;} 

    void webUpdate() {webUI_.updateBool(this->name, this->value);}

    void callback(String message) {setValue(message == "ON");}
    void mqttDiscovery(DevInfo* dev_info) {
      String name = this->name;
      String home_topic = "homeassistant/switch/" + dev_name + "/" + name;
      String discovery_topic = home_topic + "/config";
      DynamicJsonDocument doc(1024);
      String txt;
      DynamicJsonDocument json(1024);
      json["~"] = home_topic;
      json["name"] = this->friendly_name==""?name:this->friendly_name;
      json["obj_id"] = dev_name + "_" + name;
      json["uniq_id"] = dev_name + "_" + name;
      json["stat_t"] = "~"+this->stat_t;
      json["cmd_t"] = "~"+this->cmd_t;
      json["avty_t"] = dev_name + "/status";
      if (this->ic != "") json["ic"] = this->ic;
      if (this->ent_cat != "") json["ent_cat"] = this->ent_cat;
      JsonObject dev = json.createNestedObject("dev");
      dev["ids"] = mqtt_entity_id;
      dev["name"] = dev_info->name;
      dev["sw"] = dev_info->firmware;
      dev["mdl"] = dev_info->product;
      dev["mf"] = dev_info->manufacturer;
      dev["cu"] = dev_info->control;
      serializeJson(json, txt);
      this->callback_topic = home_topic + this->cmd_t;
      this->state_topic = home_topic + this->stat_t;
      this->client->beginPublish(discovery_topic.c_str(), txt.length(), false);
      this->client->print(txt);
      this->client->endPublish();
      this->client->subscribe(callback_topic.c_str());
      delay(DISCOVERY_DELAY); 
      setValue(value);
    }
};

class Select: public Entity {
  public:
    String value = ""; 
    std::map<uint8_t, String> items_list;
    uint8_t current_item = 0;
    Select(String name, PubSubClient* client):Entity(name, client){}  
    void setValue(String value) {
      this->value = value;
      this->client->publish(this->state_topic.c_str(), this->value.c_str());
    }
    void setItem(uint8_t index) {
      this->current_item = index;
      setValue(items_list[this->current_item]);
    }
    String getValue() {return this->value;} 

    void webUpdate() {webUI_.updateInt(this->name, this->current_item);}

    void callback(String message) {
      for (auto& item: items_list) {
        if (item.second == message) {
          this->current_item = item.first;
          break;
        }
      }
      setValue(message);
    }
    String getItemsString() {
      String result = "";
      for (auto& item: items_list) result = result + item.second + ",";
      if (result.length() > 0 ) result.remove(result.length()-1);
      return result;
    }
    void mqttDiscovery(DevInfo* dev_info) {
      String name = this->name;
      String home_topic = "homeassistant/select/" + dev_name + "/" + name;
      String discovery_topic = home_topic + "/config";
      DynamicJsonDocument doc(1024);
      String txt;
      DynamicJsonDocument json(1024);
      json["~"] = home_topic;
      json["name"] = this->friendly_name==""?name:this->friendly_name;
      json["obj_id"] = dev_name + "_" + name;
      json["uniq_id"] = dev_name + "_" + name;
      json["stat_t"] = "~"+this->stat_t;
      json["cmd_t"] = "~"+this->cmd_t;
      json["avty_t"] = dev_name + "/status";
      if (this->ic != "") json["ic"] = this->ic;
      if (this->ent_cat != "") json["ent_cat"] = this->ent_cat;
      JsonArray ops = json.createNestedArray("ops");
      for (auto& item: items_list) ops.add(item.second);
      JsonObject dev = json.createNestedObject("dev");
      dev["ids"] = mqtt_entity_id;
      dev["name"] = dev_info->name;
      dev["sw"] = dev_info->firmware;
      dev["mdl"] = dev_info->product;
      dev["mf"] = dev_info->manufacturer;
      dev["cu"] = dev_info->control;
      serializeJson(json, txt);
      this->callback_topic = home_topic + this->cmd_t;
      this->state_topic = home_topic + this->stat_t;
      this->client->beginPublish(discovery_topic.c_str(), txt.length(), false);
      this->client->print(txt);
      this->client->endPublish();
      this->client->subscribe(callback_topic.c_str());
      delay(DISCOVERY_DELAY); 
      setValue(value);
    }
};

std::map<String, Entity*> callback_entity;
std::list<Entity*> entities;


String mqtt_modes[] = {"HomeAssistant"};
hw_timer_t *Timer0_Cfg = NULL;
TaskHandle_t Task4Bot;

const uint16_t call_end_detect_delay  = 5000;
uint16_t delay_before_answer    = 1000;
const uint16_t delay_before_open_door = 100;
uint16_t delay_open_on_time     = 600;
uint16_t delay_after_close_door = 2000;

const uint16_t DEBOUNCE_DELAY = 100;
const uint16_t LONGPRESS_DELAY = 5000;
uint32_t last_toggle;
bool btnPressFlag = false;
bool WIFIConnected = false;
const unsigned long BOT_MTBS = 1000; 
unsigned long tlg_lasttime;

unsigned long previousMillis = 0;
unsigned long previousMQTTMillis = 0;
unsigned long interval = 30000;

AudioFileSourceSPIFFS * audioFile;
AudioGeneratorMP3 * audioPlayer;
AudioOutputI2S * audioOut = new AudioOutputI2S(0, 1);

DevInfo dev_info;
Switch * accept_once = new Switch("accept_call", &mqtt_client);
Switch * reject_once = new Switch("reject_call", &mqtt_client);
Switch * delivery_once = new Switch("delivery_call", &mqtt_client);
Switch * sound = new Switch("sound", &mqtt_client);
Switch * led = new Switch("led", &mqtt_client);
Switch * mute = new Switch("mute", &mqtt_client);
Switch * phone_disable = new Switch("phone_disable", &mqtt_client);
BinarySensor * line_detect = new BinarySensor("line_detect", &mqtt_client);
Sensor * line_status = new Sensor("line_status", &mqtt_client);
Select * modes = new Select("modes", &mqtt_client);

void ICACHE_RAM_ATTR callDetector();
void ICACHE_RAM_ATTR mqtt_callback(char*, byte*, unsigned int);
enum {WAIT, CALLING, CALL, SWUP, VOICE, SWOPEN, DROP, ENDING, RESET};
uint8_t currentAction = WAIT;
uint32_t detectMillis = 0;
uint32_t audioLength = 0;
uint8_t ledIndicatorCounter = 0;
uint8_t ledStatusCounter = 0;
bool    accept_all, reject_all, tlg_enable, ftp_enable, mqtt_enable;
String ip;
char WIFI_SSID[32];
char WIFI_PASS[32];
char TOKEN[64];
char CHAT_ID[16];

char MQTT_SERVER[16];
uint16_t MQTT_PORT = 1883;
char MQTT_USER[32];
char MQTT_PASS[32];
uint8_t MQTT_MODE = 0;
bool questSenserProtect = false;
bool save_flag = false;

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message;
  String strTopic = String((char*)topic);
  for (int i = 0; i < length; i++) message += (char)payload[i];
  callback_entity[strTopic]->callback(message);
  reject_all = (modes->current_item == 1);
  accept_all = (modes->current_item == 2);
  if (strTopic == accept_once->callback_topic) {
    if (accept_once->value) reject_once->setValue(false);
    else delivery_once->setValue(false);
  }
  if (strTopic == delivery_once->callback_topic) {
    if (delivery_once->value) accept_once->setValue(true);
    else reject_once->setValue(false);
  }
  if (strTopic == reject_once->callback_topic) {
    if (reject_once->value) {
      accept_once->setValue(false);
      delivery_once->setValue(false);
    }
  }
  accept_once->webUpdate();
  delivery_once->webUpdate();
  reject_once->webUpdate();
}

void deviceDiscovery() {
  for (Entity* entity : entities) {
    entity->mqttDiscovery(&dev_info);
    if (entity->callback_topic != "") callback_entity.insert(std::make_pair(entity->callback_topic, entity));
  }
}
void deviceOnlinePublic() {mqtt_client.publish(String(dev_name+"/status").c_str(), "online");}

template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          EEPROM.write(ee++, *p++);
    return i;
}
template <class T> int EEPROM_readAnything(int ee, T& value) {
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          *p++ = EEPROM.read(ee++);
    return i;
}

void ReadSettings() {
    Serial.println("Read configuration.");
    uint8_t first = EEPROM.read(0x00);
    uint8_t second = EEPROM.read(0x01);
    accept_once->value = first & 0x01;
    reject_once->value = first & 0x02;
    delivery_once->value = first & 0x04;
    accept_all = first & 0x08;
    reject_all = first & 0x10;
    sound->value = second & 0x01;
    led->value = second & 0x02;
    mute->value = second & 0x04;
    phone_disable->value = second & 0x08;
    tlg_enable = second & 0x10;
    ftp_enable = second & 0x20;
    mqtt_enable = second & 0x40;
    modes->current_item = accept_all?2:(reject_all?1:0);
    EEPROM_readAnything(0x02, delay_before_answer);
    EEPROM_readAnything(0x04, delay_open_on_time);
    EEPROM_readAnything(0x06, delay_after_close_door);
    EEPROM_readAnything(0x08, MQTT_PORT);
    EEPROM_readAnything(0x10, WIFI_SSID);
    EEPROM_readAnything(0x30, WIFI_PASS);
    EEPROM_readAnything(0x50, TOKEN);
    EEPROM_readAnything(0x90, CHAT_ID);
    EEPROM_readAnything(0xA0, MQTT_SERVER);
    EEPROM_readAnything(0xB0, MQTT_USER);
    EEPROM_readAnything(0xD0, MQTT_PASS);
    EEPROM_readAnything(0xF0, MQTT_MODE);
}

void WriteSettings() {
    Serial.println("Save configuration.");
    uint8_t first = 0x00;
    first |= accept_once->value? 0x01:0x00;
    first |= reject_once->value? 0x02:0x00;
    first |= delivery_once->value? 0x04:0x00;
    first |= accept_all? 0x08:0x00;
    first |= reject_all? 0x10:0x00;
    uint8_t second = 0x00;
    second |= sound->value? 0x01:0x00;
    second |= led->value? 0x02:0x00;
    second |= mute->value? 0x04:0x00;
    second |= phone_disable->value? 0x08:0x00;
    second |= tlg_enable? 0x10:0x00;
    second |= ftp_enable? 0x20:0x00;
    second |= mqtt_enable? 0x40:0x00;
    if (first != EEPROM.read(0x00)) EEPROM.write(0x00, first);
    if (second != EEPROM.read(0x01)) EEPROM.write(0x01, second);   
    EEPROM_writeAnything(0x02, delay_before_answer);
    EEPROM_writeAnything(0x04, delay_open_on_time);
    EEPROM_writeAnything(0x06, delay_after_close_door);
    EEPROM_writeAnything(0x08, MQTT_PORT);
    EEPROM_writeAnything(0x10, WIFI_SSID);
    EEPROM_writeAnything(0x30, WIFI_PASS);
    EEPROM_writeAnything(0x50, TOKEN);
    EEPROM_writeAnything(0x90, CHAT_ID);
    EEPROM_writeAnything(0xA0, MQTT_SERVER);
    EEPROM_writeAnything(0xB0, MQTT_USER);
    EEPROM_writeAnything(0xD0, MQTT_PASS);
    EEPROM_writeAnything(0xF0, MQTT_MODE);
    EEPROM.commit();  
    save_flag = true;
}


void IRAM_ATTR Timer0_ISR()
{
    if (led->value) {
        if (currentAction != WAIT) {
            switch (ledIndicatorCounter++) {
                case 0: analogWrite(led_indicator, 240); break;
                case 1: analogWrite(led_indicator, 0); ledIndicatorCounter = 0;break;
                default: if (ledIndicatorCounter > 1) ledIndicatorCounter = 0; break;
            }
        } else if (accept_once->value) {
            switch (ledIndicatorCounter++) {
                case 0: analogWrite(led_indicator, 240); break;
                case 2: analogWrite(led_indicator, 0); break;
                case 40: ledIndicatorCounter = 0; break;
                default: if (ledIndicatorCounter > 40) ledIndicatorCounter = 0; break;
            }
        } else if (mute->value) {
            ledIndicatorCounter++;
            if (ledIndicatorCounter < 40) {
                analogWrite(led_indicator, ledIndicatorCounter * 6);
            } else if (ledIndicatorCounter < 80) {
                analogWrite(led_indicator, (80 - ledIndicatorCounter) * 6);
            } else ledIndicatorCounter = 0;
        } else {
            analogWrite(led_indicator, 0);
        }
        if (!WIFIConnected) {
            switch (ledStatusCounter++) {
                case 0: digitalWrite(led_status, 1); break;
                case 10: digitalWrite(led_status, 0); break;
                case 40: ledStatusCounter = 0; break;
                default: if (ledStatusCounter > 40) ledStatusCounter = 0;
            }    
        } else {
            if (reject_once->value) {
                switch (ledStatusCounter++) {
                    case 0: digitalWrite(led_status, 1); break;
                    case 1: digitalWrite(led_status, 0); break;
                    case 40: ledStatusCounter = 0; break;
                    default: if (ledStatusCounter > 40) ledStatusCounter = 0;
                }
            } else {
                digitalWrite(led_status, 0);
            }
        }
    } else {
        analogWrite(led_indicator, 0);
        if (!WIFIConnected) {
            switch (ledStatusCounter++) {
                case 0: digitalWrite(led_status, 1); break;
                case 10: digitalWrite(led_status, 0); break;
                case 40: ledStatusCounter = 0; break;
                default: if (ledStatusCounter > 40) ledStatusCounter = 0;
            }    
        } else digitalWrite(led_status, 0);
    }
}
void callDetector() {
    if (currentAction == WAIT) {
      if (mute->value) {
          digitalWrite(switch_phone, 1);  
          digitalWrite(relay_line, 1);
      }
      detectMillis = millis();
      questSenserProtect = false;
      currentAction = CALLING;
    }
}

void enableBot(){
    Serial.println("Telegram bot started.");
    bot.updateToken(String(TOKEN));
}

void FactoryReset(){
    Serial.println("Factory Reset!");
    digitalWrite(led_status, 1);
    delay(50);
    digitalWrite(led_status, 0);
    delay(50);
    digitalWrite(led_status, 1);
    delay(50);
    digitalWrite(led_status, 0);
    delay(50);
    digitalWrite(led_status, 1);
    delay(50);
    digitalWrite(led_status, 0);
    delay(50);
    digitalWrite(led_status, 1);

    uint16_t delay_before_answer_    = 1000;
    uint16_t delay_open_on_time_     = 600;
    uint16_t delay_after_close_door_ = 2000;
    uint16_t mqtt_port_ = 1883;
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND || err == ESP_ERR_NVS_NOT_ENOUGH_SPACE) nvs_flash_erase();

    char str64 [64];
    memset(str64, 0, 64);
    char str32 [32];
    memset(str32, 0, 32);
    char str16 [16];
    memset(str16, 0, 16);

    EEPROM_writeAnything(0x00, 0x00);
    EEPROM_writeAnything(0x01, 0x03);
    EEPROM_writeAnything(0x02, delay_before_answer_);
    EEPROM_writeAnything(0x04, delay_open_on_time_);
    EEPROM_writeAnything(0x06, delay_after_close_door_);
    EEPROM_writeAnything(0x08, mqtt_port_);
    EEPROM_writeAnything(0x10, str32); 
    EEPROM_writeAnything(0x30, str32); 
    EEPROM_writeAnything(0x50, str64);
    EEPROM_writeAnything(0x90, str16);
    EEPROM_writeAnything(0xA0, str16);
    EEPROM_writeAnything(0xB0, str32);
    EEPROM_writeAnything(0xD0, str32);
    EEPROM_writeAnything(0xF0, 0x00);
    EEPROM.commit();
    delay(2000);
    ESP.restart();
}

void phone_disable_action () {
    if (currentAction == WAIT) {
      digitalWrite(switch_phone, phone_disable->value); 
      digitalWrite(relay_line, phone_disable->value);
    }
}

void doAction(uint32_t timer) {
    switch (currentAction) {
    case WAIT:  break;
    case CALLING:   if (timer > call_end_detect_delay) currentAction = RESET;
                    if (accept_once->value || delivery_once->value || reject_once->value || accept_all || reject_all) currentAction = CALL;
                    if (!line_detect->value) {
                      line_detect->setValue(currentAction!=WAIT);
                      line_status->setValue(l_status_call);
                    }
                    break;
    case CALL:  if (timer > delay_before_answer) {
                    digitalWrite(switch_phone, 0);  
                    digitalWrite(relay_line, 1);
                    currentAction = SWUP;
                    line_status->setValue(l_status_answer);
                }
                break;
    case SWUP:  if (timer > delay_before_answer + delay_before_answer) {
                    if (sound->value || delivery_once->value) {
                        audioFile = new AudioFileSourceSPIFFS(delivery_once->value ? DELIVERY_FILENAME : accept_once->value ? ACCEPT_FILENAME : reject_once->value ? REJECT_FILENAME : accept_all ? ACCEPT_FILENAME : REJECT_FILENAME);
                        if (audioFile) {
                            audioPlayer = new AudioGeneratorMP3();
                            audioPlayer->begin(audioFile, audioOut);
                            audioLength = millis();
                            currentAction = VOICE;
                            break;
                        }
                    }
                    audioLength = 0;
                    currentAction = (delivery_once->value || accept_once->value || (accept_all && !reject_once->value)) ? SWOPEN : DROP;
                }
                break;
    case VOICE: if (!audioPlayer->loop()) {
                    audioPlayer->stop();
                    audioLength = millis() - audioLength;
                    delete audioPlayer;
                    delete audioFile;
                    currentAction = (delivery_once->value || accept_once->value || (accept_all && !reject_once->value)) ? SWOPEN : DROP;
                } 
                break;
    case SWOPEN:line_status->setValue(l_status_open); 
                digitalWrite(switch_open, 1);
                currentAction = DROP;
                break;
    case DROP: if (timer > delay_before_answer + delay_before_answer + audioLength + delay_open_on_time) {
                    digitalWrite(switch_open, 0);
                    digitalWrite(switch_phone, 1); 
                    currentAction = ENDING;
                    line_status->setValue(l_status_reject);
                }
                break;
    case ENDING:line_status->setValue(l_status_reject);   
                if (timer > delay_before_answer + delay_before_answer + audioLength + delay_open_on_time + delay_after_close_door) {
                    currentAction = RESET;
                }
                break;
    case RESET: digitalWrite(relay_line, phone_disable->value);
                digitalWrite(switch_phone, phone_disable->value); 
                digitalWrite(switch_open, 0);
                accept_once->setValue(false);
                delivery_once->setValue(false);
                reject_once->setValue(false);
                currentAction = WAIT;
                line_detect->setValue(false);
                line_status->setValue(l_status_close);
                break;
    }
}

void build() {
    GP.BUILD_BEGIN(700);
    GP.PAGE_TITLE(CONFIG_CHIP_DEVICE_PRODUCT_NAME);  
    GP.THEME(GP_DARK);

    String updates = "";
    for (Entity* entity: entities) updates += (entity->name+",");
    updates = updates + "alert_save,b_answer,t_open,a_close,ssid,pass,stat,ip,token,ftp,chat_id,mqtt,mode_mqtt,srv_port,srv_mqtt,mqtt_login,mqtt_pass,tlg,save,reset";
    GP.UPDATE(updates);
    GP.ALERT("alert_save");
    GP.TITLE(CONFIG_CHIP_DEVICE_PRODUCT_NAME);

    GP.HR();
   
    GP.BLOCK_BEGIN(GP_TAB, "100%", "Управление", "#2888B3");
    M_BOX(GP.LABEL(modes->friendly_name);GP.SELECT(modes->name,modes->getItemsString(), modes->current_item););
    M_BOX(GP.LABEL(accept_once->friendly_name);GP.SWITCH(accept_once->name, accept_once->value, "#2888B3"););
    M_BOX(GP.LABEL(delivery_once->friendly_name);GP.SWITCH(delivery_once->name, delivery_once->value, "#2888B3"););
    M_BOX(GP.LABEL(reject_once->friendly_name);GP.SWITCH(reject_once->name, reject_once->value, "#2888B3"););
    GP.BLOCK_END();

    GP.BLOCK_BEGIN(GP_TAB, "100%", "Сенсоры", "#2888B3");
    M_BOX(GP.LABEL(line_status->friendly_name);GP.LABEL(line_status->value,line_status->name););
    M_BOX(GP.LABEL(line_detect->friendly_name);GP.LED_GREEN(line_detect->name, line_detect->value););
    GP.BLOCK_END();

    GP.BLOCK_BEGIN(GP_TAB, "100%", "Настройки", "#2888B3");
      M_BOX(GP.BOLD("Конфигурация устройства"););
      M_BOX(GP.LABEL("Задержка перед ответом");GP.SLIDER("b_answer", delay_before_answer, 600, 5000, 100, 0, "#2888B3"););
      M_BOX(GP.LABEL("Время удержания открытия");GP.SLIDER("t_open", delay_open_on_time, 100, 5000, 100, 0, "#2888B3"););
      M_BOX(GP.LABEL("Задержка после сброса");GP.SLIDER("a_close", delay_after_close_door, 600, 5000, 100, 0, "#2888B3"););
      M_BOX(GP.LABEL(led->friendly_name);GP.SWITCH(led->name, led->value, "#2888B3"););
      M_BOX(GP.LABEL(sound->friendly_name);GP.SWITCH(sound->name, sound->value, "#2888B3"););
      M_BOX(GP.LABEL(mute->friendly_name);GP.SWITCH(mute->name, mute->value, "#2888B3"););
      M_BOX(GP.LABEL(phone_disable->friendly_name);GP.SWITCH(phone_disable->name, phone_disable->value, "#2888B3"););

      GP.HR();
      M_BOX(GP.BOLD("Конфигурация сети"););
      M_BOX(GP.LABEL("SSID");GP.TEXT("ssid", "Имя сети Wi-Fi", WIFI_SSID););
      M_BOX(GP.LABEL("Пароль");GP.PASS("pass", "Пароль", WIFI_PASS););
      M_BOX(GP.LABEL("IP адрес");GP.LABEL(ip,"ip"););
      M_BOX(GP.LABEL("Доступ по FTP");GP.SWITCH("ftp", ftp_enable, "#2888B3"););

      GP.HR();
      M_BOX(GP.BOLD("Конфигурация MQTT"););
      M_BOX(GP.LABEL("Тип сервера MQTT");GP.SELECT("mode_mqtt", mqtt_modes[0] , MQTT_MODE););
      M_BOX(GP.LABEL("Сервер MQTT");GP.TEXT("srv_mqtt", "Адрес брокера", MQTT_SERVER););
      M_BOX(GP.LABEL("Порт MQTT");GP.NUMBER("srv_port", "Порт", MQTT_PORT););
      M_BOX(GP.LABEL("Логин MQTT");GP.TEXT("mqtt_login", "Логин", MQTT_USER););
      M_BOX(GP.LABEL("Пароль MQTT");GP.PASS("mqtt_pass", "Пароль", MQTT_PASS););
      M_BOX(GP.LABEL("Отправка в MQTT");GP.SWITCH("mqtt", mqtt_enable, "#2888B3"););

      GP.HR();
      M_BOX(GP.BOLD("Конфигурация Telegram"););
      M_BOX(GP.LABEL("Токен доступа");GP.TEXT("token", "Ключ доступа", TOKEN););
      M_BOX(GP.LABEL("ID Аккаунта");GP.TEXT("chat_id", "Кто может управлять", CHAT_ID););
      M_BOX(GP.LABEL("Отправка в Telegram");GP.SWITCH("tlg", tlg_enable, "#2888B3"););

      M_BOX(GP.HR(););
      M_BOX(GP.BUTTON("save","Сохранить настройки","","#2888B3");GP.BUTTON("reset","Перезагрузить","","#2888B3"););

    GP.BLOCK_END();

    GP.SPAN(CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION);
    GP.HR();
    GP.SPAN("При разработке данной прошивки использовались библиотеки:");
    GP.SPAN("GyverPortal (от AlexGyver)");
    GP.SPAN("Universal Telegram Bot (от Brian Lough), с изменениями (от SCratORS)");
    GP.SPAN("FTPClientServer (от Daniel Plasa), с изменениями (от SCratORS)");
    GP.SPAN("ESP8266Audio (от Earle F. Philhower, III)");

    GP.BUILD_END();
}

void action(){
    if (webUI_.update()) {
      if (save_flag && webUI_.update("alert_save")) {
        save_flag = false;
        webUI_.answer(String("Настройки сохранены!"));
      }
      for (Entity* entity: entities) entity->webUpdate();
      webUI_.updateInt("b_answer", delay_before_answer);
      webUI_.updateInt("t_open", delay_open_on_time);
      webUI_.updateInt("a_close", delay_after_close_door);
    }
    if (webUI_.click()) {
        if (webUI_.clickInt(modes->name, modes->current_item)) {
            modes->setItem(modes->current_item);
            reject_all = (modes->current_item == 1);
            accept_all = (modes->current_item == 2);
        }
        if (webUI_.clickBool(accept_once->name, accept_once->value)) {
          accept_once->setValue(accept_once->value); //for public MQTT
          if (accept_once->value) reject_once->setValue(false);
          else delivery_once->setValue(false);
          reject_once->webUpdate();
          delivery_once->webUpdate();
        }
        if (webUI_.clickBool(delivery_once->name, delivery_once->value)) {
          delivery_once->setValue(delivery_once->value);
          if (delivery_once->value) {
              accept_once->setValue(true);
              reject_once->setValue(false);
          }
          accept_once->webUpdate();
          reject_once->webUpdate();
        }
        if (webUI_.clickBool(reject_once->name, reject_once->value)) {
          reject_once->setValue(reject_once->value);
          if (reject_once->value) {
              accept_once->setValue(false);
              delivery_once->setValue(false);
          }
          accept_once->webUpdate();
          delivery_once->webUpdate();
        }
        webUI_.clickInt("b_answer", delay_before_answer);
        webUI_.clickInt("t_open", delay_open_on_time);
        webUI_.clickInt("a_close", delay_after_close_door);
        if (webUI_.clickBool(led->name, led->value)) led->setValue(led->value);
        if (webUI_.clickBool(mute->name, mute->value)) mute->setValue(mute->value);
        if (webUI_.clickBool(sound->name, sound->value)) sound->setValue(sound->value);
        if (webUI_.clickBool(phone_disable->name, phone_disable->value)) {phone_disable->setValue(phone_disable->value); phone_disable_action();}
        webUI_.clickBool("ftp", ftp_enable);
        webUI_.clickStr("ssid", WIFI_SSID);
        webUI_.clickStr("pass", WIFI_PASS);
        if (webUI_.clickBool("mqtt", mqtt_enable)) if (!mqtt_enable) mqtt_client.disconnect();
        webUI_.clickInt("mode_mqtt", MQTT_MODE);
        webUI_.clickInt("srv_port", MQTT_PORT);
        webUI_.clickStr("srv_mqtt", MQTT_SERVER);
        webUI_.clickStr("mqtt_login", MQTT_USER);
        webUI_.clickStr("mqtt_pass", MQTT_PASS);       
        webUI_.clickStr("token", TOKEN);
        webUI_.clickStr("chat_id", CHAT_ID);
        if (webUI_.clickBool("tlg", tlg_enable)) if (tlg_enable) enableBot();         
        if (webUI_.click("save")) WriteSettings();
        if (webUI_.click("reset")) ESP.restart();
    }
}

String kbGenerate(){
  String keyboardJson = "[[{ \"text\" : \"Режим работы: "+modes->value+"\", \"callback_data\" : \"/"+modes->name+"\" }],";
  keyboardJson = keyboardJson + "[{ \"text\" : \""+(accept_once->value?"🟢":"⚫️")+" "+accept_once->friendly_name+"\", \"callback_data\" : \"/"+accept_once->name+"\" },";
  keyboardJson = keyboardJson + "{ \"text\" : \""+(delivery_once->value?"🟢":"⚫️")+" "+delivery_once->friendly_name+"\", \"callback_data\" : \"/"+delivery_once->name+"\" },";
  keyboardJson = keyboardJson + "{ \"text\" : \""+(reject_once->value?"🟢":"⚫️")+" "+reject_once->friendly_name+"\", \"callback_data\" : \"/"+reject_once->name+"\" }],";
  keyboardJson = keyboardJson + "[{ \"text\" : \""+(mute->value?"🟢":"⚫️")+" "+mute->friendly_name+"\", \"callback_data\" : \"/"+mute->name+"\" },";
  keyboardJson = keyboardJson + "{ \"text\" : \""+(sound->value?"🟢":"⚫️")+" "+sound->friendly_name+"\", \"callback_data\" : \"/"+sound->name+"\" }],";
  keyboardJson = keyboardJson + "[{ \"text\" : \""+(led->value?"🟢":"⚫️")+" "+led->friendly_name+"\", \"callback_data\" : \"/"+led->name+"\" },";
  keyboardJson = keyboardJson + "{ \"text\" : \""+(phone_disable->value?"🟢":"⚫️")+" "+phone_disable->friendly_name+"\", \"callback_data\" : \"/"+phone_disable->name+"\" }]]";
  return keyboardJson;
}

String kbSetModeGenerate(){
  String keyboardJson = "[[{ \"text\" : \""+modes->items_list[0]+"\", \"callback_data\" : \"/mode_0\" }],";
  keyboardJson = keyboardJson + "[{ \"text\" : \""+modes->items_list[1]+"\", \"callback_data\" : \"/mode_1\" }],";
  keyboardJson = keyboardJson + "[{ \"text\" : \""+modes->items_list[2]+"\", \"callback_data\" : \"/mode_2\" }]]";
  return keyboardJson;
}

String kbCallQuestion(){
  String keyboardJson = "[[{\"text\":\"✅ "+accept_once->friendly_name+"\",\"callback_data\":\"/"+accept_once->name+"_b\"},";
  keyboardJson = keyboardJson + "{\"text\":\"🚚 "+delivery_once->friendly_name+"\",\"callback_data\":\"/"+delivery_once->name+"_b\"},";
  keyboardJson = keyboardJson + "{\"text\":\"🚷 "+reject_once->friendly_name+"\",\"callback_data\":\"/"+reject_once->name+"_b\"}]]";
  return keyboardJson;
}

void sendKB(String m_chat_id){
  String welcome = "SmartIntercom - Домофон:\n";
  bot.sendMessageWithInlineKeyboard(m_chat_id, welcome, "", kbGenerate());
}
void sendQuestionKB(String m_chat_id){
  String welcome = "Входящий вызов в домофон!\n";
  bot.sendMessageWithInlineKeyboard(m_chat_id, welcome, "", kbCallQuestion());
}
void sendModeKB(String m_chat_id){
  String welcome = "Выбор постоянного режима работы:\n";
  bot.sendMessageWithInlineKeyboard(m_chat_id, welcome, "", kbSetModeGenerate());
}



void telegram_message(int numNewMessages) {
  for (int i=0; i<numNewMessages; i++) {
    String m_chat_id = String(bot.messages[i].chat_id);
    bool noKb = false;
    if (m_chat_id != String(CHAT_ID)){
      bot.sendMessage(m_chat_id, "Unauthorized user", "");
      continue;
    }
    
    String text = bot.messages[i].text;

    if (bot.messages[i].type == "callback_query") {
      if (text == "/modes") sendModeKB(m_chat_id);
      else {
        if (text == "/mode_0") {modes->setItem(0);accept_all=false;reject_all=false;}
        if (text == "/mode_1") {modes->setItem(1);accept_all=false;reject_all=true;}
        if (text == "/mode_2") {modes->setItem(2);accept_all=true;reject_all=false;}
        if (text == String("/"+accept_once->name+"_b") ||
            text == String("/"+delivery_once->name+"_b") ||
            text == String("/"+reject_once->name+"_b") ) noKb = true;

        if (text == String("/"+accept_once->name) || text == String("/"+accept_once->name+"_b") ) {
          accept_once->setValue(!accept_once->value);
          if (accept_once->value) reject_once->setValue(false);
          else delivery_once->setValue(false);
        }
        if (text == String("/"+delivery_once->name) || text == String("/"+delivery_once->name+"_b") ) {
          delivery_once->setValue(!delivery_once->value);
          if (delivery_once->value) {
            accept_once->setValue(true);
            reject_once->setValue(false);
          }
        }
        if (text == String("/"+reject_once->name) || text == String("/"+reject_once->name+"_b") ) {
          reject_once->setValue(!reject_once->value);
          if (reject_once->value) {
            accept_once->setValue(false);
            delivery_once->setValue(false);
          }
        }
        if (text == String("/"+mute->name)) mute->setValue(!mute->value);
        if (text == String("/"+sound->name)) sound->setValue(!sound->value);
        if (text == String("/"+led->name)) led->setValue(!led->value);
        if (text == String("/"+phone_disable->name)) {
          phone_disable->setValue(!phone_disable->value);
          phone_disable_action();
        }
        if (!noKb) sendKB(m_chat_id);
      }
    }
    
    if (text == "/start") sendKB(m_chat_id);
  }
}

void setup() {
    Serial.begin(115200);
    EEPROM.begin(0xF1);
    pinMode(led_status, OUTPUT);
    pinMode(led_indicator, OUTPUT);
    pinMode(detect_line, INPUT_PULLUP);
    pinMode(button_boot, INPUT);
    pinMode(relay_line, OUTPUT);
    pinMode(switch_open, OUTPUT);
    pinMode(switch_phone, OUTPUT);
    attachInterrupt(detect_line, callDetector, FALLING);
    audioOut->SetOutputModeMono(true);
    Timer0_Cfg = timerBegin(0, 80, true);
    timerAttachInterrupt(Timer0_Cfg, &Timer0_ISR, true);
    timerAlarmWrite(Timer0_Cfg, 50000, true);
    timerAlarmEnable(Timer0_Cfg);
    ReadSettings();
    line_status->value = l_status_close;   
    WIFIConnected = false;
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    client.setTimeout(SSL_RESPONSE_TIMEOUT);
    mqtt_entity_id = String(WiFi.macAddress());
    mqtt_entity_id.replace(":","");
    WiFi.mode(WIFI_STA);
    Serial.printf("Connect to: %s\n", WIFI_SSID);
    WiFi.begin(String(WIFI_SSID), String(WIFI_PASS));
    while (WiFi.status() != WL_CONNECTED && millis() < 15000 ) delay(500);
    if (WiFi.status() != WL_CONNECTED) {
        WIFIConnected = false;
        ip = "192.168.4.1";
        Serial.printf("Connecting ERROR! Activate software Wi-Fi AccessPoint.\n");
        Serial.printf("Wi-Fi AccessPoint name: %s\n", CONFIG_CHIP_DEVICE_PRODUCT_NAME);
        Serial.printf("IP address for WebUI: %s\n", ip);
        WiFi.disconnect();
        WiFi.mode(WIFI_AP);
        WiFi.softAP(CONFIG_CHIP_DEVICE_PRODUCT_NAME, NULL);
        
    } else {
      ip = WiFi.localIP().toString().c_str();
      Serial.printf("Successful. IP address: %s\n", ip);
      Serial.printf("Configurate time. Connect to: %s\n", TIME_SERVER);
      configTime(0, 0, TIME_SERVER); // get UTC time via NTP
      time_t now = time(nullptr);
      while (now < 24 * 3600) {
        Serial.print(".");
        delay(100);
        now = time(nullptr);
      }
      randomSeed(micros());
      Serial.printf("\nComplete. Current timestamp: %d\n", now);
      WIFIConnected = true;
      if (tlg_enable) enableBot(); 
    }

    if (SPIFFS.begin(true)) {
      ftpSrv = new FTPServer(SPIFFS);
      ftpSrv->begin("", "");
      Serial.println("FTP Server Started.");
    } else {
      Serial.println("File system could not be opened. FTP Server will not rabota!");
    };

/*Create Device and entity*/
    
    dev_info.name = CONFIG_CHIP_DEVICE_PRODUCT_NAME;
    dev_info.manufacturer = "SCratORS";
    dev_info.product = String(CONFIG_CHIP_DEVICE_PRODUCT_NAME) + " Nelma 4 ESP32";
    dev_info.firmware = CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION;
    dev_info.control = "http://"+ip;
    mqtt_client.setCallback(mqtt_callback);

    modes->items_list.insert(std::make_pair(0, "Не активен"));
    modes->items_list.insert(std::make_pair(1, "Сброс вызова"));
    modes->items_list.insert(std::make_pair(2, "Открывать всегда"));
    modes->value = modes->items_list[0];
    modes->ic = "mdi:deskphone";modes->friendly_name = "Режим работы";;entities.push_back(modes);

    line_status->ic = "mdi:bell";line_status->friendly_name = "Статус линии";entities.push_back(line_status);
    accept_once->ic = "mdi:door-open";accept_once->friendly_name = "Открыть дверь";entities.push_back(accept_once);
    reject_once->ic = "mdi:phone-hangup";reject_once->friendly_name = "Сбросить вызов";entities.push_back(reject_once);
    delivery_once->ic = "mdi:package";delivery_once->friendly_name = "Открыть курьеру";entities.push_back(delivery_once);
    line_detect->friendly_name = "Детектор вызова";entities.push_back(line_detect);
    sound->ent_cat = "config";sound->ic = "mdi:volume-high";sound->friendly_name = "Аудиосообщения";entities.push_back(sound);
    led->ent_cat = "config";led->ic = "mdi:led-on";led->friendly_name = "Светоиндикация";entities.push_back(led);
    mute->ent_cat = "config";mute->ic = "mdi:bell-off";mute->friendly_name = "Беззвучный режим";entities.push_back(mute);
    phone_disable->ent_cat = "config";phone_disable->ic = "mdi:phone-off";phone_disable->friendly_name = "Отключить трубку";entities.push_back(phone_disable);

    webUI_.attachBuild(build);
    webUI_.attach(action);
    webUI_.start();
    webUI_.enableOTA();
    webUI_.downloadAuto(true);

    
    if (currentAction == WAIT) digitalWrite(relay_line, phone_disable->value);  
    rtc_wdt_set_length_of_reset_signal(RTC_WDT_SYS_RESET_SIG, RTC_WDT_LENGTH_3_2us);
    rtc_wdt_set_stage(RTC_WDT_STAGE0, RTC_WDT_STAGE_ACTION_RESET_SYSTEM);
    rtc_wdt_set_time(RTC_WDT_STAGE0, 500);
    
    xTaskCreatePinnedToCore(NetworkUpdate, "NetworkUpdate", STACK_SIZE, NULL, tskIDLE_PRIORITY, &Task4Bot,  0); 
}

void mqtt_reconnect() {
  if (!mqtt_client.connected()) {
    String clientId = "smartintercom-" + WiFi.macAddress();
    mqtt_client.disconnect();
    mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
    Serial.printf("Attempting MQTT connection: %s:%d\n",MQTT_SERVER,MQTT_PORT);
    if (mqtt_client.connect(clientId.c_str(), MQTT_USER, MQTT_PASS) ) {
      Serial.println("MQTT connected");
      deviceDiscovery();
      deviceOnlinePublic();
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
    }
  }
}

bool its_online() {return WIFIConnected && (WiFi.status() == WL_CONNECTED);}

void loop() {
    if (currentAction != WAIT) doAction(millis()-detectMillis);
    bool btnState = !digitalRead(button_boot);
    if (btnState && !btnPressFlag && millis() - last_toggle > DEBOUNCE_DELAY) {
        btnPressFlag = true;
        last_toggle = millis();
        accept_once->setValue(!accept_once->value);
        if (accept_once->value) reject_once->setValue(false);
        else delivery_once->setValue(false);
    }
    if (btnState && btnPressFlag && millis() - last_toggle > LONGPRESS_DELAY) {
        last_toggle = millis();
        FactoryReset();
    }
    if (!btnState && btnPressFlag && millis() - last_toggle > DEBOUNCE_DELAY) {
        btnPressFlag = false;
        last_toggle = millis();
    }
    webUI_.tick();
    if (ftp_enable && ftpSrv && its_online()) ftpSrv->handleFTP();
    if (mqtt_enable && its_online() && mqtt_client.connected()) mqtt_client.loop();
    if (!mqtt_enable && mqtt_client.connected()) mqtt_client.disconnect();
    rtc_wdt_feed();
}

void NetworkUpdate( void * pvParameters ){
  while (true) {
    unsigned long currentMillis = millis();
    if (WIFIConnected && (WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
        mqtt_client.disconnect();
        WiFi.disconnect();
        WiFi.reconnect();
        previousMillis = currentMillis;
    }
    if (mqtt_enable && its_online() && !mqtt_client.connected() && (currentMillis - previousMQTTMillis >= 5000)) {
        mqtt_reconnect();
        previousMQTTMillis = currentMillis;
    }

    if (mqtt_enable && its_online() && mqtt_client.connected() && (currentMillis - previousMQTTMillis >= 15000)) {
        deviceOnlinePublic();
        previousMQTTMillis = currentMillis;
    }
 
    if (WIFIConnected && tlg_enable) {
      if (millis() - tlg_lasttime > BOT_MTBS) {
        if (currentAction == CALLING && !questSenserProtect) {
          questSenserProtect = true;
          sendQuestionKB(String(CHAT_ID));
        }
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        if (tlg_enable) tlg_enable = bot._lastError == 0;
        while (numNewMessages) {
          telegram_message(numNewMessages);
          numNewMessages = bot.getUpdates(bot.last_message_received + 1);
          if (tlg_enable) tlg_enable = bot._lastError == 0;
        }
        tlg_lasttime = millis();
      }
    }
    rtc_wdt_feed();
  }
}

