# 1 "C:\\Users\\USERX\\AppData\\Local\\Temp\\tmp28n967hr"
#include <Arduino.h>
# 1 "C:/Users/USERX/Downloads/LilyGo-Camera-Series/examples/sketch/sketch.ino"
# 14 "C:/Users/USERX/Downloads/LilyGo-Camera-Series/examples/sketch/sketch.ino"
#include <WiFi.h>
#include <Wire.h>
#include "esp_camera.h"
# 37 "C:/Users/USERX/Downloads/LilyGo-Camera-Series/examples/sketch/sketch.ino"
#define DEFAULT_MEASUR_MILLIS 3000


#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP 5




#define WIFI_SSID "Bonus"
#define WIFI_PASSWD "123456789"

#include "select_pins.h"

#if defined(SOFTAP_MODE)
#endif
String macAddress = "";
String ipAddress = "";

extern void startCameraServer();

#if defined(BUTTON_1)

#include <OneButton.h>
OneButton button(BUTTON_1, true);
#endif

#if defined(ENABLE_BEM280)

#include "SparkFunBME280.h"
BME280 sensor;
String temp, pressure, altitude, humidity;
#endif

#if defined(SSD130_MODLE_TYPE)

#include "SSD1306.h"
#include "OLEDDisplayUi.h"
#define SSD1306_ADDRESS 0x3c
SSD1306 oled(SSD1306_ADDRESS, I2C_SDA, I2C_SCL, (OLEDDISPLAY_GEOMETRY)SSD130_MODLE_TYPE);
OLEDDisplayUi ui(&oled);
#endif

#if defined(ENABLE_TFT)

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
#endif
bool setupSensor();
void readSensor();
bool deviceProbe(uint8_t addr);
bool setupDisplay();
void loopDisplay();
bool setupPower();
bool setupSDCard();
bool setupCamera();
void setupNetwork();
void setupButton();
void setup();
void loop();
#line 86 "C:/Users/USERX/Downloads/LilyGo-Camera-Series/examples/sketch/sketch.ino"
bool setupSensor()
{
#if defined(ENABLE_BEM280)
    bool status = sensor.beginI2C();
    if (!status)
        return false;
    sensor.setMode(MODE_SLEEP);
    sensor.setFilter(1);
    sensor.setStandbyTime(0);

    sensor.setTempOverSample(1);
    sensor.setPressureOverSample(1);
    sensor.setHumidityOverSample(1);

    sensor.setMode(MODE_SLEEP);
#endif

#if defined(AS312_PIN)
    pinMode(AS312_PIN, INPUT);
#endif
    return true;
}

void readSensor()
{
#if defined(ENABLE_BEM280)

    static long lastMillis;

    if (millis() - lastMillis < DEFAULT_MEASUR_MILLIS)
        return;
    sensor.setMode(MODE_FORCED);

    long startTime = millis();
    while (sensor.isMeasuring() == false)
        ;
    while (sensor.isMeasuring() == true)
        ;
    long endTime = millis();


    Serial.print(" Measure time(ms): ");
    Serial.print(endTime - startTime);

    Serial.print(" Humidity: ");
    humidity = sensor.readFloatHumidity();
    Serial.print(humidity);

    Serial.print(" Pressure: ");
    pressure = sensor.readFloatPressure();
    Serial.print(pressure);

    Serial.print(" Alt: ");

    altitude = String(sensor.readFloatAltitudeFeet());
    Serial.print(altitude);

    Serial.print(" Temp: ");

    temp = String(sensor.readTempC());
    Serial.print(temp);

    Serial.println();

#endif
}

bool deviceProbe(uint8_t addr)
{
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

bool setupDisplay()
{

#if defined(ENABLE_TFT)
#if defined(CAMERA_MODEL_TTGO_T_CAMERA_PLUS)
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("I GET", tft.width() / 2, tft.height() / 2);
    tft.drawString("HEE KUY TAD", tft.width() / 2, tft.height() / 2 + 20);
    pinMode(TFT_BL_PIN, OUTPUT);
    digitalWrite(TFT_BL_PIN, HIGH);
#endif

#elif defined(SSD130_MODLE_TYPE)
    static FrameCallback frames[] = {
        [](OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y)
        {
            display->setTextAlignment(TEXT_ALIGN_CENTER);
            display->setFont(ArialMT_Plain_10);
#if (SSD130_MODLE_TYPE)
            display->drawString(64 + x, 0 + y, macAddress);
            display->drawString(64 + x, 10 + y, ipAddress);
#else
            display->drawString(64 + x, 9 + y, macAddress);
            display->drawString(64 + x, 25 + y, ipAddress);
#endif

#if defined(AS312_PIN)
            if (digitalRead(AS312_PIN)) {
                display->drawString(64 + x, 40 + y, "AS312 Trigger");
            }
#endif
        },
        [](OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y)
        {
#if defined(ENABLE_BME280)
            display->setFont(ArialMT_Plain_16);
            display->setTextAlignment(TEXT_ALIGN_LEFT);
            display->drawString(0 + x, 0 + y, temp);
            display->drawString(0 + x, 16 + y, pressure);
            display->drawString(0 + x, 32 + y, altitude);
            display->drawString(0 + x, 48 + y, humidity);
#else
            display->setTextAlignment(TEXT_ALIGN_CENTER);
            display->setFont(ArialMT_Plain_10);

#if (SSD130_MODLE_TYPE)

            display->drawString(64 + x, 0 + y, "Camera Ready! Use");
            display->drawString(64 + x, 10 + y, "http://" + ipAddress);
            display->drawString(64 + x, 16 + y, "to connect");

#else
            display->drawString(64 + x, 5 + y, "Camera Ready! Use");
            display->drawString(64 + x, 25 + y, "http://" + ipAddress);
            display->drawString(64 + x, 45 + y, "to connect");

#endif

#endif
        }
    };

    if (!deviceProbe(SSD1306_ADDRESS))
        return false;
    oled.init();

    oled.setFont(ArialMT_Plain_16);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);

    oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2 - 10, "LilyGo CAM");
    oled.display();
    ui.setTargetFPS(30);
    ui.setIndicatorPosition(BOTTOM);
    ui.setIndicatorDirection(LEFT_RIGHT);
    ui.setFrameAnimation(SLIDE_LEFT);
    ui.setFrames(frames, sizeof(frames) / sizeof(frames[0]));
    ui.setTimePerFrame(6000);
    ui.disableIndicator();
#endif
    return true;
}

void loopDisplay()
{
#if defined(SSD130_MODLE_TYPE)
    if (ui.update()) {
#endif

#if defined(BUTTON_1)
        button.tick();
#endif

#if defined(SSD130_MODLE_TYPE)
    }
#elif defined(ENABLE_TFT)



#endif
}

bool setupPower()
{
#if defined(ENABLE_IP5306)
#define IP5306_ADDR 0X75
#define IP5306_REG_SYS_CTL0 0x00
    if (!deviceProbe(IP5306_ADDR))
        return false;
    bool en = true;
    Wire.beginTransmission(IP5306_ADDR);
    Wire.write(IP5306_REG_SYS_CTL0);
    if (en)
        Wire.write(0x37);
    else
        Wire.write(0x35);
    return Wire.endTransmission() == 0;

#elif defined(ENABLE_AXP192)
#define AXP192_ADDRESS 0x34
    if (!deviceProbe(AXP192_ADDRESS))
        return false;

    uint8_t val;
    Wire.beginTransmission(AXP192_ADDRESS);
    Wire.write(0x30);
    Wire.endTransmission();
    Wire.requestFrom(AXP192_ADDRESS, 1);
    val = Wire.read();

    Wire.beginTransmission(AXP192_ADDRESS);
    Wire.write(0x30);
    Wire.write(val & 0xFC);
    Wire.endTransmission();

    Wire.beginTransmission(AXP192_ADDRESS);
    Wire.write(0x12);
    Wire.endTransmission();
    Wire.requestFrom(AXP192_ADDRESS, 1);
    val = Wire.read();

    Wire.beginTransmission(AXP192_ADDRESS);
    Wire.write(0x12);
    Wire.write(val & 0b10100010);
    Wire.endTransmission();
#endif
#if defined(CAMERA_MODEL_TTGO_T_CAMERA_MINI)

    pinMode(POWER_CONTROL_PIN, OUTPUT);
    digitalWrite(POWER_CONTROL_PIN, HIGH);
#endif

    return true;
}

#if defined(SDCARD_CS_PIN)
#include <SD.h>
#endif
bool setupSDCard()
{







#if defined(SDCARD_CS_PIN)
    if (!SD.begin(SDCARD_CS_PIN)) {
        tft.setTextColor(TFT_RED);
        tft.drawString("SDCard begin failed", tft.width() / 2, tft.height() / 2 - 20);
        tft.setTextColor(TFT_WHITE);
        return false;
    } else {
        String cardInfo = String(((uint32_t)SD.cardSize() / 1024 / 1024));
        tft.setTextColor(TFT_GREEN);
        tft.drawString("SDcardSize=[" + cardInfo + "]MB", tft.width() / 2, tft.height() / 2 + 92);
        tft.setTextColor(TFT_WHITE);

        Serial.print("SDcardSize=[");
        Serial.print(cardInfo);
        Serial.println("]MB");
    }
#endif
    return true;
}

bool setupCamera()
{
    camera_config_t config;

#if defined(Y2_GPIO_NUM)
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;

    if (psramFound()) {
        config.frame_size = FRAMESIZE_UXGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_DRAM;
    }
#endif

#if defined(ESPRESSIF_ESP_EYE) || defined(CAMERA_MODEL_TTGO_T_CAMERA_V162) || defined(CAMERA_MODEL_TTGO_T_CAMERA_MINI)



    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
#endif


    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();

    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1);
        s->set_brightness(s, 1);
        s->set_saturation(s, -2);
    }

    s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_TTGO_T_CAMERA_V162)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
#endif
    return true;
}

void setupNetwork()
{
    macAddress = "LilyGo-CAM-";
#ifdef SOFTAP_MODE
    WiFi.mode(WIFI_AP);
    macAddress += WiFi.softAPmacAddress().substring(0, 5);
    WiFi.softAP(macAddress.c_str());
    ipAddress = WiFi.softAPIP().toString();
#else
    WiFi.begin(WIFI_SSID, WIFI_PASSWD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    ipAddress = WiFi.localIP().toString();
    macAddress += WiFi.macAddress().substring(0, 5);
#endif
#if defined(ENABLE_TFT)
#if defined(CAMERA_MODEL_TTGO_T_CAMERA_PLUS)
    tft.drawString("ipAddress:", tft.width() / 2, tft.height() / 2 + 50);
    tft.drawString(ipAddress, tft.width() / 2, tft.height() / 2 + 72);
#endif
#endif
}

void setupButton()
{
#if defined(BUTTON_1)
    button.attachClick([]() {
        static bool en = false;
        sensor_t *s = esp_camera_sensor_get();
        en = en ? 0 : 1;
        s->set_vflip(s, en);
#if defined(SSD130_MODLE_TYPE)
        if (en) {
            oled.resetOrientation();
        } else {
            oled.flipScreenVertically();
        }
#endif

    });

    button.attachDoubleClick([]() {
        if (PWDN_GPIO_NUM > 0) {
            pinMode(PWDN_GPIO_NUM, PULLUP);
            digitalWrite(PWDN_GPIO_NUM, HIGH);
        }

#if defined(SSD130_MODLE_TYPE)
        ui.disableAutoTransition();
        oled.setTextAlignment(TEXT_ALIGN_CENTER);
        oled.setFont(ArialMT_Plain_10);
        oled.clear();
#if defined(AS312_PIN) && defined(PIR_SUPPORT_WAKEUP)
        oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "Set to wake up from PIR");
#elif defined(BUTTON_1)
        oled.drawString(oled.getWidth() / 2 - 5, oled.getHeight() / 2 - 20, "Deepsleep");
        oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2 - 10, "Set to be awakened by");
        oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "a key press");
#else
        oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "Set to wake up by timer");
#endif
        oled.display();
        delay(3000);
        oled.clear();
        oled.displayOff();
#else
        delay(2000);
#endif

#if defined(AS312_PIN) && defined(PIR_SUPPORT_WAKEUP)
        esp_sleep_enable_ext1_wakeup(((uint64_t)(((uint64_t)1) << AS312_PIN)), ESP_EXT1_WAKEUP_ANY_HIGH);
#elif defined(BUTTON_1)

#if defined(CAMERA_MODEL_TTGO_T_CAMERA_MINI)
        esp_sleep_enable_ext1_wakeup(((uint64_t)(((uint64_t)1) << BUTTON_1)), ESP_EXT1_WAKEUP_ANY_HIGH);
#else
        esp_sleep_enable_ext1_wakeup(((uint64_t)(((uint64_t)1) << BUTTON_1)), ESP_EXT1_WAKEUP_ALL_LOW);
#endif
#else
        esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
#endif
        esp_deep_sleep_start();
    });
#if defined(CAMERA_MODEL_TTGO_T_CAMERA_MINI)
    button.setClickTicks(200);
    button.setDebounceTicks(0);
#endif
#endif
}

void setup()
{

    Serial.begin(115200);
#if defined(PWR_ON_PIN)
    pinMode(PWR_ON_PIN, OUTPUT);
    digitalWrite(PWR_ON_PIN, 1);
#endif
#if defined(I2C_SDA) && defined(I2C_SCL)
    Wire.begin(I2C_SDA, I2C_SCL);
#endif

    bool status;
    status = setupDisplay();
    Serial.print("setupDisplay status ");
    Serial.println(status);

    status = setupSDCard();
    Serial.print("setupSDCard status ");
    Serial.println(status);

    status = setupPower();
    Serial.print("setupPower status ");
    Serial.println(status);

    status = setupSensor();
    Serial.print("setupSensor status ");
    Serial.println(status);

    status = setupCamera();
    Serial.print("setupCamera status ");
    Serial.println(status);
    if (!status) {
        delay(10000);
        esp_restart();
    }

    setupButton();

    setupNetwork();

    startCameraServer();

    Serial.print("Camera Ready! Use 'http://");
    Serial.print(ipAddress);
    Serial.println("' to connect");
}

void loop()
{
    loopDisplay();
}