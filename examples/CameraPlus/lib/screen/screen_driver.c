// Copyright 2020 Espressif Systems (Shanghai) Co. Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sdkconfig.h"
#include <string.h>
#include "screen_driver.h"
#include "esp_log.h"

static const char *TAG = "screen driver";

#define LCD_CHECK(a, str, ret)  if(!(a)) {                           \
        ESP_LOGE(TAG,"%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str);   \
        return (ret);                                                           \
    }
extern scr_driver_t lcd_st7789_default_driver;


esp_err_t scr_find_driver(scr_controller_t controller, scr_driver_t *out_screen)
{
    LCD_CHECK(NULL != out_screen, "Pointer of screen is invalid", ESP_ERR_INVALID_ARG);

    esp_err_t ret = ESP_OK;
    *out_screen = lcd_st7789_default_driver;
    return ret;
}
