# SmartIntercom

![](images/main.jpg)

Умный домофон на ESP
Устройство поддерживает передачу аудио потока на блок вызова. Перед использованием необходимо записать аудио файлы из папка data в LittleFS.
Для воспроизведения аудио файлов используется библиотека https://github.com/earlephilhower/ESP8266Audio
При использовании ESPHome воспроизведение mp3 файлов невозможно из-за нехватки оперативной памяти МК, по-этому используем WAV (PCM 22кГц 16 Бит (signed) Моно).

При использовании Arduino IDE (Тестовый скеч для проверки функциональности):
1. Установить поддержку плат ESP8266.
2. Установить модуль загрузки LittleFS https://github.com/earlephilhower/arduino-esp8266littlefs-plugin/releases
3. Установить библиотеку ESP8266Audio
4. Открыть проект.
5. Загрузить файлы из data в LittleFS https://arduino-esp8266.readthedocs.io/en/latest/filesystem.html
6. Прошить МК.

При использовании ESPHome
1. В рабочую папку ESPHome (/Config) скопировать папку lib с библиотекой "ESPAudio.h" и "ESPFtp.h"из проекта. 
2. Прошить smartintercom_e8db849c6ee5.yaml (Все необходимые библиотеки ESPHome загрузит сам)
3. Для загрузки аудио файлов, нужно подключиться к плате по FTP (пассивный режим, анонимное соединение без пароля), и скопировать файлы из папки data в корень FTP
4. Добавить карточку устройства на панель lovalace из файла card.yaml проекта. ![](images/card.png)

При использовании ESP Download Tools (ESP8266)
1. /bin/smartintercom_e8db849c6ee5.bin - Прошивать в адрес 0x0
2. /bin/mklittlefs_0x200000.bin - Прошивать в адрес 0x200000

Добавление управления через Телеграм:
https://github.com/SCratORS/SmartIntercom/issues/3
https://github.com/SCratORS/SmartIntercom/issues/6

Управление через Алису:
https://github.com/SCratORS/SmartIntercom/issues/7
https://github.com/SCratORS/SmartIntercom/issues/9

Автоматическое отключение однократного открытия домофона:
https://github.com/SCratORS/SmartIntercom/issues/8

"Афтар! Чо так сложно?! А можно проще?" - Можно проще, идём сюда:
https://wiki.smartintercom.ru/ru/home

Схема устройсва и описание:
https://easyeda.com/scrators/intercom

Телеграм канал для обсуждения:
https://t.me/smartintercom
