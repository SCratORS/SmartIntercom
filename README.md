# SmartIntercom
<p align="center" width="100%">
    <img width="100%" src="https://github.com/user-attachments/assets/3a0c55de-07b2-4b87-9379-20e4fee1e5f3"> 
</p>

### P.S. Данная прошивка подходит для устройств собраных на базе [Smart модуль для Laskomex LM-8D](https://shop.scrators.ru/index.php?route=product/product&language=ru-ru&product_id=56&path=61).


## Хотите купить:
Купить готовое устройство можно в https://shop.scrators.ru

У Вас уже есть готовое и прошитое устройство, которое вы приобрели, и вы не знаете как начать с ним работать? - Почитайте [Инструкцию пользователя](https://github.com/SCratORS/BlueStreak/blob/main/userguide.pdf)

## В данном репозитории находятся исходные коды для прошивки ESPHome. В настоящее время готовые устройства поставляются с прошивкой [BlueStreak](https://github.com/SCratORS/BlueStreak)

Далее речь пойдет для прошивки под управлением [ESPHome](https://esphome.io/). - DIY, если хочется.

## Что по функциям:
Разумеется банально открыть/сбросить вызов, отключить трубку. Воспроизведение MP3-аудиофайлов на блок вызова (аудиосообщения) для различных событий (открыть/сбросить/курьер). Работа с Яндекс.Алисой, Телеграммом и другими сторонними сервисами возможна, если Ваш сервер умного дома это умеет. 

## Как прошить устройство?

### Если прошита прошивка BlueStreak:

1. Переходим в Web-интерфейс, и в меню "Дополнительно", в разделе "Обновление прошивки" выбираем файл [esphome_1.7.6.esp32ota.bin](https://github.com/SCratORS/SmartIntercom/blob/main/bin/esphome_1.7.6.esp32ota.bin)
2. После окончания прошивки надо загрузить аудиофайлы в файловую систему, для этого:
3. Подключить устройство к Вашему WiFi, см. [Первое включение](https://github.com/SCratORS/SmartIntercom/wiki/%D0%9F%D0%BE%D0%B4%D0%BA%D0%BB%D1%8E%D1%87%D0%B5%D0%BD%D0%B8%D0%B5-%D1%83%D1%81%D1%82%D1%80%D0%BE%D0%B9%D1%81%D1%82%D0%B2%D0%B0)
4. Подключиться к устройству по FTP (пассивный режим, анонимное соединение без пароля и без TLS).
5. Скопировать mp3 файлы из [data](https://github.com/SCratORS/SmartIntercom/tree/main/data) в корень FTP.

### Если ничего не прошито, голая прошивка / аварийная перепрошивка:
Используя какой-нибудь USB-TTL программатор и далее с помощью ESP Download Tools для прошивки ESP32, подробнее см. [Перепрошивка устройства](https://github.com/SCratORS/SmartIntercom/wiki/%D0%9F%D0%B5%D1%80%D0%B5%D0%BF%D1%80%D0%BE%D1%88%D0%B8%D0%B2%D0%BA%D0%B0-%D1%83%D1%81%D1%82%D1%80%D0%BE%D0%B9%D1%81%D1%82%D0%B2%D0%B0):
1. Основной файл прошивки [ESP32_smartintercom.bin](bin/ESP32_smartintercom.bin) - Прошивать в адрес 0x0
2. Аудиофайлы в виде образа LittleFS [ESP32_mklittlefs_0x2B0000.bin](bin/ESP32_mklittlefs_0x2B0000.bin) - Прошивать в адрес 0x2B0000

### Самостоятельная компиляция прошивки ESPHome (на примере HomeAssistant):
1. Установить из магазина дополнений "ESPHome" и "FileEditor"
2. Запустить дополнение FileEditor и перейти к рабочий каталог /сonfig/esphome/
3. Используя меню "File Upload" загрузить файл [smartintercom-esp32.yaml](ESPHome/smartintercom-esp32.yaml)
4. Используя меню "File Upload" загрузить файл [partitions_esp32.csv](ESPHome/partitions_esp32.csv)
4. Используя меню "Create Folder" создать папку lib и перейти в неё
5. Используя меню "File Upload" загрузить файлы библиотек [ESPAudio.h](ESPHome/lib/ESPAudio.h) и [ESPUtils.h](ESPHome/lib/ESPUtils.h)
6. Запустить дополнение ESPHome, - там появится проект "smartintercom" в статусе OFFLINE
8. Нажать на проекте "...", выбрать меню "INSTALL", выбрать способ прошивки (для компиляции можно выбрать "Manual Download - Modern Format"). Во время компиляции все необходимые библиотеки ESPHome загрузит сам
9. После успешной компиляции, если устройство уже было прошито ESPHome и подключено к сети, то статус проекта станет ONLINE.
10. Если прошивается впервые, то можно тутже из ESPHome прошить через программатор.
11. Если прошивается впервые, то после успешной прошивки, и выхода устройства в Онлайн, нужно загрузить аудио файлы, для этого подключаемся к плате по FTP (пассивный режим, анонимное соединение без пароля и без TLS), и копируем файлы *.mp3 из папки [data](data/) в корень FTP
12. По желанию добавить карточку устройства на панель lovelace из файла [card.yaml](ESPHome/card.yaml) проекта. ![](images/card_2.png)

## Дополнительно:
Добавление управления через Телеграм:
https://github.com/SCratORS/SmartIntercom/issues/3
https://github.com/SCratORS/SmartIntercom/issues/6

Управление через Алису:
https://github.com/SCratORS/SmartIntercom/issues/7
https://github.com/SCratORS/SmartIntercom/issues/9

Автоматическое отключение однократного открытия домофона:
https://github.com/SCratORS/SmartIntercom/issues/8

Электрическая схема и печатная плата актуальной версии для самостоятельного производства (толщина текстолита используется 1.2мм - иначе в корпусе будет щель):
[https://oshwlab.com/scrators/smartintercom-nelma-ext-v1-1](https://oshwlab.com/scrators/smartintercom-nelma-ext-v1-1)

Телеграм канал для обсуждения:
https://t.me/smartintercom
