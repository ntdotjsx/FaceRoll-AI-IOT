#include <Arduino.h>

#include <WiFi.h>
#include <Wire.h>
#include "esp_camera.h"

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define DEFAULT_MEASUR_MILLIS 3000 /* Get sensor time by default (ms)*/

// When using timed sleep, set the sleep time here
#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5        /* Time ESP32 will go to sleep (in seconds) */

/***************************************
 *  WiFi
 **************************************/
#define WIFI_SSID "Bonus"
#define WIFI_PASSWD "123456789"

#define BUTTON_PIN 23

bool isSending = false;
camera_fb_t *pendingFrame = nullptr;

#include "select_pins.h"
#include <TJpg_Decoder.h>

#if defined(SOFTAP_MODE)
#endif
String macAddress = "";
String ipAddress = "";
String response = "PRESS START :)";  // กำหนดตัวแปร response ที่ระดับ global

#if defined(ENABLE_TFT)
// Depend TFT_eSPI library ,See  https://github.com/Bodmer/TFT_eSPI
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();
#endif

bool sendToAI(camera_fb_t *fb)
{
    if (!fb)
        return false;

    WiFiClient client;
    HTTPClient http;

    // URL ที่จะส่งข้อมูลไปยัง server
    http.begin(client, "http://185.84.161.154:8000/detect");
    http.addHeader("Content-Type", "multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW"); // กำหนด boundary

    // สร้าง body ของ request
    String body = "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n";
    body += "Content-Disposition: form-data; name=\"file\"; filename=\"image.jpg\"\r\n";
    body += "Content-Type: image/jpeg\r\n\r\n";

    // ส่งคำขอ POST
    int httpResponseCode = http.POST(body + String((char *)fb->buf, fb->len) + "\r\n------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n");

    if (httpResponseCode > 0)
    {
        String response = http.getString();
        Serial.println("🧠 Response from AI:");
        Serial.println(response);

        // ใช้ ArduinoJson เพื่อแปลงข้อมูล JSON
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error)
        {
            String result = doc["result"].as<String>();      // ดึงค่า "result"
            float confidence = doc["confidence"].as<float>(); // ดึงค่า "confidence"
            ::response = result + " " + String(confidence, 2); // สร้างข้อความที่ต้องการ
        }
        else
        {
            Serial.println("Failed to parse JSON");
        }
    }
    else
    {
        Serial.printf("❌ POST failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
    return httpResponseCode == 200;
}

void sendImageTask(void *parameter) {
    camera_fb_t *fb = (camera_fb_t *)parameter;

    if (fb) {
        sendToAI(fb);
        esp_camera_fb_return(fb);
    }

    isSending = false;
    vTaskDelete(NULL); // ลบ task ตัวเอง
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
        config.jpeg_quality = 1;
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

    s->set_contrast(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);

    s->set_hmirror(s, 1);
    s->set_framesize(s, FRAMESIZE_QQVGA); // 160x120 หรือ
    // s->set_framesize(s, FRAMESIZE_QVGA); // 320x240

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
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Serial.begin(115200);

    TJpgDec.setSwapBytes(true);
    TJpgDec.setCallback(tft_output);

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("PLEASE", tft.width() / 2, tft.height() / 2);
    tft.drawString("WAIT NETWORK", tft.width() / 2, tft.height() / 2 + 20);
    pinMode(TFT_BL_PIN, OUTPUT);
    digitalWrite(TFT_BL_PIN, HIGH);

#if defined(I2C_SDA) && defined(I2C_SCL)
    Wire.begin(I2C_SDA, I2C_SCL);
#endif

    bool status;

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

    setupNetwork();
}

void loopDisplay()
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("Camera capture failed");
        return;
    }

    // คำนวณตำแหน่งที่ให้ภาพอยู่กลางจอ
    int x_pos = (tft.width() - fb->width) / 2;   // คำนวณตำแหน่ง X (แนวนอน)
    int y_pos = (tft.height() - fb->height) / 2; // คำนวณตำแหน่ง Y (แนวตั้ง)

    tft.startWrite();
    TJpgDec.drawJpg(x_pos, y_pos, fb->buf, fb->len); // วาดภาพที่ตำแหน่งกลางจอ

    String wifiStatus;
    if (WiFi.status() == WL_CONNECTED)
    {
        wifiStatus = "WiFi: " + String(WIFI_SSID); // หรือใช้ "Connected" เฉย ๆ ก็ได้
    }
    else
    {
        wifiStatus = "WiFi: Disconnected";
    }

    tft.drawString(response , tft.width() / 2, 38);

    tft.drawString(wifiStatus, tft.width() / 2, tft.height() - tft.fontHeight());

    tft.endWrite();

    esp_camera_fb_return(fb);
}

// void loop()
// {
//     loopDisplay();
//     if (digitalRead(BUTTON_PIN) == LOW)
//     {
//         sensor_t *s = esp_camera_sensor_get();

//         camera_fb_t *fb = esp_camera_fb_get();
//         if (fb)
//         {
//             sendToAI(fb);
//             esp_camera_fb_return(fb);
//         }
//     }
// }
bool autoSendEnabled = false;
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 5000; // ทุก 5 วิ

unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 300; // ms

void checkToggleButton() {
    if (digitalRead(BUTTON_PIN) == LOW) {
        unsigned long now = millis();
        if (now - lastButtonPress > debounceDelay) {
            autoSendEnabled = !autoSendEnabled;
            Serial.println(autoSendEnabled ? "🟢 Auto Send: ON" : "🔴 Auto Send: OFF");

            // อัพเดตหน้าจอแสดงสถานะด้วยถ้ามีจอ
            #if defined(ENABLE_TFT)
            tft.fillRect(0, 0, tft.width(), 20, TFT_BLACK); // เคลียร์หัวจอ
            tft.setCursor(0, 0);
            if (autoSendEnabled) {
                tft.setTextColor(TFT_GREEN, TFT_BLACK); // ON = เขียว
                tft.print("Auto: ON");
            } else {
                tft.setTextColor(TFT_RED, TFT_BLACK); // OFF = แดง
                tft.print("Auto: OFF");
            }
            #endif

            lastButtonPress = now;
        }
    }
}

void loop() {
    loopDisplay();
    checkToggleButton();

    if (autoSendEnabled && !isSending) {
        unsigned long now = millis();
        if (now - lastSendTime >= sendInterval) {
            lastSendTime = now;

            camera_fb_t *fb = esp_camera_fb_get();
            if (fb) {
                isSending = true;
                xTaskCreatePinnedToCore(
                    sendImageTask,     // ฟังก์ชันที่รัน
                    "SendImageTask",   // ชื่อ task
                    8192,              // Stack size
                    fb,                // Parameter ส่งภาพ
                    1,                 // Priority
                    NULL,              // Task handle
                    1                  // รันบน Core 1
                );
            }
        }
    }
}
