# SmartIntercom

![](images/main.jpg)

Умный домофон на ESP

(Устройство удаленного управления абонентской линией координатных домофонных систем)

Поддерживается передача аудио потока на блок вызова. Перед использованием этой функции необходимо передать плате аудио файлы для воспроизведения:
- Для плат на базе ESP8266: записать аудио файлы из папки data в LittleFS.
- Для плат на базе ESP32: Для воспроизведения по HTTP,- подготовить вспомогательные компоненты HomeAssistant [input_text.yaml](ESPHome/input_text.yaml) в которых прописываются ссылки на аудио файлы. Для воспроизведения с SD карты, - записать аудио файлы из папки data в корень SD карты.

Для воспроизведения аудио файлов используется библиотека https://github.com/earlephilhower/ESP8266Audio.

При использовании ESPHome воспроизведение mp3 файлов на платах ESP8266 невозможно из-за нехватки оперативной памяти МК, поэтому используем WAV (PCM 22кГц 16 Бит (signed) Моно).

## Хотите купить:
Купить готовое устройство можно в https://smartintercom.ru

У Вас уже есть готовое и прошитое устройство, которое вы приобрели, и вы не знаете как начать с ним работать? - Почитайте [userguide.pdf](userguide.pdf)

## Нужно прошить/перепрошить устройство:
### При использовании ESPHome (на примере HomeAssistant и устройств на базе ESP8266)
1. Установить из магазина дополнений "ESPHome" и "FileEditor"
2. Запустить дополнение FileEditor и перейти к рабочий каталог /сonfig/esphome/
3. Используя меню "File Upload" загрузить файл [smartintercom-esp8266.yaml](ESPHome/smartintercom-esp8266.yaml)
4. Используя меню "Create Folder" создать папку lib и перейти в неё
5. Используя меню "File Upload" загрузить файлы библиотек [ESPAudio.h](ESPHome/lib/ESPAudio.h) и [ESPFtp.h](ESPHome/lib/ESPFtp.h)
6. Запустить дополнение ESPHome, - там появится проект "smartintercom" в статусе OFFLINE
7. Для того чтобы статус устройства стал ONLINE, проект должен быть скомпилирован.
8. Нажать на проекте "...", выбрать меню "INSTALL", выбрать способ прошивки (просто для компиляции можно выбрать "Manual Download - Modern Format"). Во время компиляции все необходимые библиотеки ESPHome загрузит сам
9. После успешной прошивки, и выхода устройства в Онлайн, нужно загрузить аудио файлы, для этого подключаемся к плате по FTP (пассивный режим, анонимное соединение без пароля), и копируем файлы из папки [data](data/) в корень FTP
10. По желанию добавить карточку устройства на панель lovelace из файла [card.yaml](ESPHome/card.yaml) проекта. ![](images/card.png)
### При использовании ESP Download Tools (ESP8266)
1. Основной файл прошивки [smartintercom.bin](bin/smartintercom.bin) - Прошивать в адрес 0x0
2. Аудиофайлы в виде образа [mklittlefs_0x200000.bin](bin/mklittlefs_0x200000.bin) - Прошивать в адрес 0x200000
## Дополнительно:
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
https://oshwlab.com/scrators/intercom-v4b

Телеграм канал для обсуждения:
https://t.me/smartintercom
