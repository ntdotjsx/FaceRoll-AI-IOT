idf_component_register(
  SRCS "main.c" "app_camera.c"
  INCLUDE_DIRS "." "include"
  REQUIRES esp32-camera nvs_flash esp_http_server
)