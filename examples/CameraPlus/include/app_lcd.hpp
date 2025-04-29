#pragma once

#include "screen_driver.h"

#include "__base__.hpp"
#include "app_camera.hpp"
#include "app_button.hpp"
#include "app_speech.hpp"

#define BOARD_LCD_MOSI 19
#define BOARD_LCD_MISO 22
#define BOARD_LCD_SCK 21
#define BOARD_LCD_CS 12
#define BOARD_LCD_DC 15
#define BOARD_LCD_RST -1
#define BOARD_LCD_BL 2
#define BOARD_LCD_PIXEL_CLOCK_HZ (20 * 1000000)
#define BOARD_LCD_BK_LIGHT_ON_LEVEL 1
#define BOARD_LCD_BK_LIGHT_OFF_LEVEL !BOARD_LCD_BK_LIGHT_ON_LEVEL
#define BOARD_LCD_H_RES 240
#define BOARD_LCD_V_RES 240
#define BOARD_LCD_CMD_BITS 8
#define BOARD_LCD_PARAM_BITS 8
#define LCD_HOST SPI2_HOST

class AppLCD : public Observer, public Frame
{
private:
    AppButton *key;
    AppSpeech *speech;

public:
    scr_driver_t driver;
    bool switch_on;
    bool paper_drawn;

    AppLCD(AppButton *key,
           AppSpeech *speech,
           QueueHandle_t xQueueFrameI = nullptr,
           QueueHandle_t xQueueFrameO = nullptr,
           void (*callback)(camera_fb_t *) = esp_camera_fb_return);

    void draw_wallpaper();
    void draw_color(int color);

    void update();

    void run();
};
