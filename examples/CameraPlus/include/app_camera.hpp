#pragma once

#include <list>

#include "esp_camera.h"
#include "__base__.hpp"

#define CAMERA_MODULE_NAME "T-CameraPlus"

#define CAMERA_PIN_PWDN     (-1)
#define CAMERA_PIN_RESET    (-1)
#define CAMERA_PIN_XCLK     (4)

#define CAMERA_PIN_SIOD     (18)
#define CAMERA_PIN_SIOC     (23)
#define CAMERA_PIN_D7       (36)
#define CAMERA_PIN_D6       (37)
#define CAMERA_PIN_D5       (38)
#define CAMERA_PIN_D4       (39)
#define CAMERA_PIN_D3       (35)
#define CAMERA_PIN_D2       (26)
#define CAMERA_PIN_D1       (13)
#define CAMERA_PIN_D0       (34)
#define CAMERA_PIN_VSYNC    (5)
#define CAMERA_PIN_HREF     (27)
#define CAMERA_PIN_PCLK     (25)


#define XCLK_FREQ_HZ        15000000


enum CameraDirection {
    CAMERA_Positive,
    CAMERA_Reverse
};

class AppCamera : public Frame
{
public:
    AppCamera(const pixformat_t pixel_fromat,
              const framesize_t frame_size,
              const uint8_t fb_count,
              CameraDirection dir,
              QueueHandle_t queue_o = nullptr);

    void run();
};
