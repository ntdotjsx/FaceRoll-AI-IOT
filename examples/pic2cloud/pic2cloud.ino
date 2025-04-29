
#define TINY_GSM_MODEM_SIM7600
// #define DUMP_AT_COMMANDS

#include "esp_camera.h"
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <PubSubClient.h>
#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
#endif
#include <TinyGsmClient.h>
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//
//            You must select partition scheme from the board menu that has at least 3MB APP space.
//            Face Recognition is DISABLED for ESP32 and ESP32-S2, because it takes up from 15
//            seconds to process single frame. Face Detection is ENABLED if PSRAM is enabled as well
#include "camera_pins.h"
/**
 * Enter your APP credentials
 *
 * Refer to (documentation)[https://cloud.bemfa.com/docs/#/],
 * create an application, obtain the following information and fill in it.
 */

/**********************************************************/
// User private key, obtained from Bafa Cloud console
const char *uid = "********************************";
// Theme name, which can be created in the console
const char *topic = "picture";
// If it is not empty, it will be pushed to wechat and can be modified at will to modify it to the message you need to send
const char *wechatMsg = "Hello +T-SIMCAM";
// If it is not empty, it will be pushed to the enterprise wechat. The message pushed to the enterprise wechat can be modified at will and modified to
// the message that needs to be sent
const char *wecomMsg = "";
// If the value is not empty, a custom image link will be generated to define the image URL returned after the image is uploaded. The first part of
// the URL is the Bafa cloud domain name, the second part is the MD5 value of the private key + subject name, and the third part is the set image link
// value.
const char *urlPath = "";
/**********************************************************/
// MQTT details
const char *mqtt_uid = "********************************";
const char *broker = "bemfa.com";

const char *topicMsg = "msg";
/* Need to create a topic application named 'topicMsg' */

const char *take_cmd = "take";

/**********************************************************/

static bool take_state = false;

#if !defined(TINY_GSM_RX_BUFFER)
#define TINY_GSM_RX_BUFFER 650
#endif
#define TINY_GSM_USE_GPRS true
// Your GPRS credentials, if any
const char apn[] = "YourAPN";
const char gprsUser[] = "";
const char gprsPass[] = "";
// Server details
const char *server_url = "images.bemfa.com"; // Default Upload Address
const char *post_url = "/upload/v1/upimages.php";
const int port = 80;
#define Serial Serial

#ifdef DUMP_AT_COMMANDS
StreamDebugger debugger(Serial1, Serial);
TinyGsm modem(Sedebuggerrial1);
#else
TinyGsm modem(Serial1);
#endif

TinyGsmClient client(modem);
HttpClient http(client, server_url, port);
PubSubClient mqtt(client);

void upload_pic(uint8_t *pic_buf, size_t len, const char *topic, const char *wechatMsg, const char *wecomMsg, const char *urlPath);

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    Serial.println();
#if defined(CAMERA_MODEL_TTGO_T_CAM_SIM)
    pinMode(PWR_ON_PIN, OUTPUT);
    digitalWrite(PWR_ON_PIN, HIGH);
#endif

    Serial1.begin(115200, SERIAL_8N1, PCIE_RX_PIN, PCIE_TX_PIN);
    pinMode(PCIE_PWR_PIN, OUTPUT);
    digitalWrite(PCIE_PWR_PIN, 1);
    delay(500);
    digitalWrite(PCIE_PWR_PIN, 0);
    Serial.println("Wait...");
    // TinyGsmAutoBaud(Serial1, 9600, 115200);
    delay(3000);
    Serial.println("Initializing modem...");
    modem.init();
    String modemInfo = modem.getModemInfo();
    Serial.print("Modem Info: ");
    Serial.println(modemInfo);

    camera_config_t config;
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
    config.frame_size = FRAMESIZE_UXGA;
    config.pixel_format = PIXFORMAT_JPEG; // for streaming
    // config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
    // for larger pre-allocated frame buffer.
    if (config.pixel_format == PIXFORMAT_JPEG) {
        if (psramFound()) {
            config.jpeg_quality = 10;
            config.fb_count = 2;
            config.grab_mode = CAMERA_GRAB_LATEST;
        } else {
            // Limit the frame size when PSRAM is not available
            config.frame_size = FRAMESIZE_SVGA;
            config.fb_location = CAMERA_FB_IN_DRAM;
        }
    } else {
        // Best option for face detection/recognition
        config.frame_size = FRAMESIZE_UXGA;
#if CONFIG_IDF_TARGET_ESP32S3
        config.fb_count = 2;
#endif
    }

#if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
#endif

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    sensor_t *s = esp_camera_sensor_get();
    // initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 0);       // flip it back
        s->set_brightness(s, 1);  // up the brightness just a bit
        s->set_saturation(s, -2); // lower the saturation
    }
    s->set_vflip(s, 1);
    // drop down frame size for higher initial frame rate
    if (config.pixel_format == PIXFORMAT_JPEG) {
        s->set_framesize(s, FRAMESIZE_UXGA);
    }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
    s->set_vflip(s, 1);
#endif

    // MQTT Broker setup
    mqtt.setServer(broker, 9501);
    mqtt.setCallback(mqttCallback);
}

void loop()
{
    Serial.print("Waiting for network...");
    if (!modem.waitForNetwork()) {
        Serial.println(" fail");
        delay(10000);
        return;
    }
    Serial.println(" success");

    if (modem.isNetworkConnected()) {
        Serial.println("Network connected");
    }

#if TINY_GSM_USE_GPRS
    // GPRS connection parameters are usually set after network registration
    Serial.print(F("Connecting to "));
    Serial.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        Serial.println(" fail");
        delay(10000);
        return;
    }
    Serial.println(" success");

    if (modem.isGprsConnected()) {
        Serial.println("GPRS connected");
    }
#endif

    if (!mqtt.connected()) {
        Serial.print("Connecting to ");
        Serial.print(broker);
        // Connect to MQTT Broker
        boolean status = mqtt.connect(mqtt_uid);
        if (status == false) {
            Serial.println(" fail");
            return;
        }
        Serial.println(" success");
        mqtt.subscribe(topicMsg);
    }
    Serial.printf("After initialization, send  \"%s\"  command to upload the picture\r\n", take_cmd);

    while (true) {
        // Do nothing. Everything is done in another task by the web server
        delay(10);
        mqtt.loop();
        if (take_state) {
            do { // Remove MQTT connection status to prevent incorrect disconnection during picture transfer
                // mqtt.unsubscribe(topicMsg);
                mqtt.disconnect();
                delay(10);
            } while (mqtt.connected());

            Serial.println("To upload pictures");
            camera_fb_t *pic = NULL;
            pic = esp_camera_fb_get();
            if (!pic) {
                Serial.println("Camera capture failed");
                break;
            }
            upload_pic(pic->buf, pic->len, topic, wechatMsg, wecomMsg, urlPath);
            esp_camera_fb_return(pic);
            do { // After uploading the image, reconnect to the MQTT server and report the information
                mqtt.connect(mqtt_uid);
                mqtt.subscribe(topicMsg);
            } while (!mqtt.connected());
            Serial.println("Pictures have been uploaded.");
            take_state = false;
        }
    }
}

void upload_pic(uint8_t *pic_buf, size_t len, const char *topic, const char *wechatMsg, const char *wecomMsg, const char *urlPath)
{
    Serial.println(F("Start uploading pictures... "));
    http.connectionKeepAlive();
    Serial.println(F("Performing HTTP POST request... "));
    Serial.println(F("Wait for upload to complete..."));
    http.beginRequest();
    http.post(post_url);
    http.sendHeader("Content-Type", "image/jpg");
    http.sendHeader("Authorization", uid);
    http.sendHeader("Authtopic", topic);
    http.sendHeader("wechatmsg", wechatMsg);
    http.sendHeader("wecommsg", wecomMsg);
    http.sendHeader("picpath", urlPath);
    // http.sendHeader("Accept-Encoding", "gzip, deflate, br");
    http.sendHeader("Content-Length", String(len));
    http.beginBody();
    uint32_t j = 0;
    uint32_t shard = 1426;
    for (int32_t i = len; i > 0;) {
        if (i >= shard) {
            http.write((const uint8_t *)(pic_buf + shard * j), shard);
            i -= shard;
            j++;
        } else {
            http.write((const uint8_t *)(pic_buf + shard * j), i);
            break;
        }
    }
    http.endRequest();
    // read the status code and body of the response
    int statusCode = http.responseStatusCode();
    String response = http.responseBody();
    Serial.print("Status code: ");
    Serial.println(statusCode);
    Serial.print("Response: ");
    Serial.println(response);
    // Shutdown
    http.stop();
    Serial.println(F("Server disconnected"));
}

void mqttCallback(char *topic, byte *payload, unsigned int len)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.write(payload, len);
    Serial.println();

    // Only proceed if incoming message's topic matches
    if (String(topic) == topicMsg) {
        if (String((char *)payload, len) == String(take_cmd)) {
            take_state = true;
        }
    }
}
