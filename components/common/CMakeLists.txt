idf_component_register(
  SRCS "common.c"
  INCLUDE_DIRS "include"
  REQUIRES nvs_flash bt
  esp_system
)