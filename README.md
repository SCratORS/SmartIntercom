# SmartIntercom

![](images/main.jpg)

Умный домофон на ESP
Устройство поддерживает передачу аудио потока на блок вызова. Перед использованием необходимо записать аудио файлы из папка data в LittleFS.
Для воспроизведения аудио файлов используется библиотека https://github.com/earlephilhower/ESP8266Audio
При использовании ESPHome воспроизведение mp3 файлов невозможно из-за нехватки оперативной памяти МК.

При использовании Arduino IDE (Тестовый скеч для проверки функциональности):
1. Установить поддержку плат ESP8266.
2. Установить модуль загрузки LittleFS https://github.com/earlephilhower/arduino-esp8266littlefs-plugin/releases
3. Установить библиотеку ESP8266Audio
4. Открыть проект.
5. Загрузить файлы из data в LittleFS https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html
6. Прошить МК.

При использовании ESPHome
1. Прошить smartintercom_e8db849c6ee5.yaml (Все необходимые библиотеки ESPHome загрузит сам)
2. Загрузить файлы из data в LittleFS. Можно используя ArduinoIDE (Шаги 1,2,4,5). Или любым другим удобным способом.
3. В файле конфигурации Home Assistant добавить вспомогательный компонент input_select для управления режимами устройства (Смотри файл configuration.yaml из проекта)
4. Добавить карточку устройства на панель lovalace из файла card.yaml проекта. ![](images/card.png =250x)


Схема устройсва и описание:
https://easyeda.com/scrators/intercom
