
#include <Arduino.h>
#include "app_lcd.hpp"
#include "app_camera.hpp"
#include "app_face.hpp"
#include "app_motion.hpp"

void setup()
{
    Serial.begin(115200);

    esp_log_level_set("*", ESP_LOG_DEBUG);

    QueueHandle_t xQueueFrame_0 = xQueueCreate(2, sizeof(camera_fb_t *));
    QueueHandle_t xQueueFrame_1 = xQueueCreate(2, sizeof(camera_fb_t *));
    QueueHandle_t xQueueFrame_2 = xQueueCreate(2, sizeof(camera_fb_t *));

    AppCamera *camera = new AppCamera(PIXFORMAT_RGB565,
                                      FRAMESIZE_240X240,
                                      2,
                                      CAMERA_Positive,
                                      xQueueFrame_0);


    AppFace *face = new AppFace(NULL, NULL, xQueueFrame_0, xQueueFrame_1);
    AppMotion *motion = new AppMotion(NULL, NULL, xQueueFrame_1, xQueueFrame_2);
    AppLCD *lcd = new AppLCD(NULL, NULL, xQueueFrame_2);
    lcd->switch_on = true;
    face->switch_on = true;

    lcd->run();
    motion->run();
    face->run();
    camera->run();
}


void loop()
{
    delay(10000);
}



















