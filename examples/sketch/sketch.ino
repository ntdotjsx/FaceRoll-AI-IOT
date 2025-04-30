#include <WiFi.h>
#include <Wire.h>
#include "esp_camera.h"

#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#define DEFAULT_MEASUR_MILLIS 3000 /* Get sensor time by default (ms)*/

// When using timed sleep, set the sleep time here
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5        /* Time ESP32 will go to sleep (in seconds) */

/***************************************
 *  WiFi
 **************************************/
#define WIFI_SSID "Bonus"
#define WIFI_PASSWD "123456789"

#include "select_pins.h"
#include <TJpg_Decoder.h>

#if defined(SOFTAP_MODE)
#endif
String macAddress = "";
String ipAddress = "";

#if defined(ENABLE_TFT)
// Depend TFT_eSPI library ,See  https://github.com/Bodmer/TFT_eSPI
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
#endif

bool sendToAI(camera_fb_t *fb)
{
    if (!fb) return false;

    WiFiClient client;
    HTTPClient http;

    http.begin(client, "http://2bf2-58-10-245-203.ngrok-free.app/detect");
    http.addHeader("Content-Type", "image/jpeg");

    int httpResponseCode = http.POST(fb->buf, fb->len);

    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("🧠 Response from AI:");
        Serial.println(response);
    } else {
        Serial.printf("❌ POST failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
    return httpResponseCode == 200;
}

bool setupSensor()
{
    return true;
}

bool deviceProbe(uint8_t addr)
{
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

bool setupDisplay()
{

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
    return true;
}

bool setupPower()
{
#define IP5306_ADDR 0X75
#define IP5306_REG_SYS_CTL0 0x00
    if (!deviceProbe(IP5306_ADDR))
        return false;
    bool en = true;
    Wire.beginTransmission(IP5306_ADDR);
    Wire.write(IP5306_REG_SYS_CTL0);
    if (en)
        Wire.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
    else
        Wire.write(0x35); // 0x37 is default reg value
    return Wire.endTransmission() == 0;

    return true;
}

#if defined(SDCARD_CS_PIN)
#include <SD.h>
#endif
bool setupSDCard()
{
    /*
        T-CameraPlus Board, SD shares the bus with the LCD screen.
        It does not need to be re-initialized after the screen is initialized.
        If the screen is not initialized, the initialization SPI bus needs to be turned on.
    */
    // SPI.begin(TFT_SCLK_PIN, TFT_MISO_PIN, TFT_MOSI_PIN);

#if defined(SDCARD_CS_PIN)
    if (!SD.begin(SDCARD_CS_PIN))
    {
        tft.setTextColor(TFT_RED);
        tft.drawString("SDCard begin failed", tft.width() / 2, tft.height() / 2 - 20);
        tft.setTextColor(TFT_WHITE);
        return false;
    }
    else
    {
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
    // init with high specs to pre-allocate larger buffers
    if (psramFound())
    {
        config.frame_size = FRAMESIZE_UXGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    }
    else
    {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
        config.fb_location = CAMERA_FB_IN_DRAM;
    }
#endif
    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    // initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV3660_PID)
    {
        s->set_vflip(s, 1);       // flip it back
        s->set_brightness(s, 1);  // up the blightness just a bit
        s->set_saturation(s, -2); // lower the saturation
    }
    // drop down frame size for higher initial frame rate
    s->set_framesize(s, FRAMESIZE_QVGA);

    return true;
}

void setupNetwork()
{
    macAddress = "LilyGo-CAM-";

    WiFi.begin(WIFI_SSID, WIFI_PASSWD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    ipAddress = WiFi.localIP().toString();
    macAddress += WiFi.macAddress().substring(0, 5);

#if defined(ENABLE_TFT)
#if defined(CAMERA_MODEL_TTGO_T_CAMERA_PLUS)
    tft.drawString("ipAddress:", tft.width() / 2, tft.height() / 2 + 50);
    tft.drawString(ipAddress, tft.width() / 2, tft.height() / 2 + 72);
#endif
#endif
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
    tft.pushImage(x, y, w, h, bitmap);
    return true;
}

void setup()
{
    Serial.begin(115200);

    TJpgDec.setSwapBytes(true);
    TJpgDec.setCallback(tft_output);

#if defined(I2C_SDA) && defined(I2C_SCL)
    Wire.begin(I2C_SDA, I2C_SCL);
#endif

    bool status;

    // ตั้งค่า SDCard
    status = setupSDCard();
    Serial.print("setupSDCard status ");
    Serial.println(status);

    // ตั้งค่าพลังงาน
    status = setupPower();
    Serial.print("setupPower status ");
    Serial.println(status);

    // ตั้งค่าเซ็นเซอร์
    status = setupSensor();
    Serial.print("setupSensor status ");
    Serial.println(status);

    // ตั้งค่ากล้อง
    status = setupCamera();
    Serial.print("setupCamera status ");
    Serial.println(status);

    // หาก setup ไม่สำเร็จ, ให้รีเซ็ตเครื่อง
    if (!status)
    {
        delay(10000);
        esp_restart();
    }

    // ตั้งค่าเครือข่าย
    setupNetwork();

    // ตรวจสอบสถานะกล้องว่าเตรียมพร้อมแล้วหรือไม่
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb)
    {
        sendToAI(fb);
        esp_camera_fb_return(fb); // คืนค่าภาพหลังส่ง
    }
    else
    {
        Serial.println("Failed to capture image from camera");
    }

    // ข้อความแสดงบน TFT
    Serial.print("Camera Ready! Use 'http://");
    Serial.print(ipAddress);
    Serial.println("' to connect");

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
#endif
}

void loopDisplay()
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("Camera capture failed");
        return;
    }

    tft.fillScreen(TFT_BLACK);
    // แสดง JPEG ที่ตำแหน่ง 0,0
    TJpgDec.drawJpg(0, 0, fb->buf, fb->len);

    esp_camera_fb_return(fb);

    delay(100); // ปรับเฟรมเรต
}

void loop()
{
    loopDisplay();
}