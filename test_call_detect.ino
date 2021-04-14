#include <ESP8266WiFi.h>
#include "AudioOutputI2SNoDAC.h"
#include <LittleFS.h>
#include "AudioFileSourceLittleFS.h"
#include "AudioGeneratorMP3.h"  
#include <Arduino.h>
#define LINE_SWITCH_PIN   14   //LINE SWITCH
#define DETECT_CALL_PIN   12   //GPIO Detect call
#define OPEN_DOOR_PIN     5    //GPIO Open the door
void ICACHE_RAM_ATTR callDetector();
const char* ssid = "YOUR SSID";
const char* password =  "YOUR PASSWORD";

int delaySWUP                     = 1000; 
int delayVOICE                    = 1000;
int delayOPEN                     = 3000;
long openDoorMillis = 0;


enum {WAIT, CALL, SWUP, VOICE, SWOPEN, RESET};

byte currentAction = WAIT;

AudioFileSourceLittleFS * user_access_allowed;
AudioGeneratorMP3 * mp3;
AudioOutputI2SNoDAC * audioOut = new AudioOutputI2SNoDAC();

void setup() {  
  // put your setup code here, to run once:
  delay(1000);
  Serial.begin(115200);
  delay(1000);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());
  Serial.println("OK");
  LittleFS.begin();
  delay(1000);
  pinMode(LINE_SWITCH_PIN, OUTPUT);  
  pinMode(OPEN_DOOR_PIN, OUTPUT); 
  pinMode (DETECT_CALL_PIN, INPUT);
  digitalWrite(OPEN_DOOR_PIN, 0);
  digitalWrite(LINE_SWITCH_PIN, 0);
  attachInterrupt(DETECT_CALL_PIN, callDetector, FALLING);
}

void callDetector() {
  if (currentAction == WAIT) {
    Serial.println("CALL DETECT");
    openDoorMillis = millis(); //Save time when call detector was fired 
    currentAction = CALL;
  }
}

void loop()
{
  switch (currentAction) {
    case WAIT:  break;
    case CALL:  if (millis() - openDoorMillis > delaySWUP) {
                    Serial.println("LINE SWITCH");
                    digitalWrite(OPEN_DOOR_PIN, 0);  
                    digitalWrite(LINE_SWITCH_PIN, 1);
                    currentAction = SWUP;
                }
                break;
    case SWUP:  if (millis() - openDoorMillis > delaySWUP + delayVOICE) {
                  Serial.println("UP");  
                  user_access_allowed = new AudioFileSourceLittleFS("/user_access_allowed.mp3");
                  mp3 = new AudioGeneratorMP3();
                  mp3->begin(user_access_allowed, audioOut);
                  currentAction = VOICE;
                  Serial.println("VOICE");  
                }
                break;   
    case VOICE: if (!mp3->loop()){
                currentAction = SWOPEN;
                mp3->stop();
                pinMode(2, OUTPUT);//что бы диод на rx-tx потух
                delete mp3;
                delete user_access_allowed;
                Serial.println("OPEN");  
                }
                break;   
    case SWOPEN: digitalWrite(OPEN_DOOR_PIN, 1);
                 Serial.println("OPEN ME");  
                 currentAction = RESET; 
                 break;
    case RESET: if (millis() - openDoorMillis > delaySWUP + delayVOICE + delayOPEN) {
                  Serial.println("RESET"); 
                  digitalWrite(OPEN_DOOR_PIN, 0);
                  digitalWrite(LINE_SWITCH_PIN, 0);
                  delay(1000);
                  currentAction = WAIT;
                }
                break;
  }
}
