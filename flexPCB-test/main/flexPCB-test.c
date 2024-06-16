
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "hal/gpio_types.h"

#include "blink_codes.h"
#include "buzzer.h"
#include "mspacelock.h"

void app_main(void) {

  esp_log_level_set("*", ESP_LOG_INFO);

  // Initialize the buzzer
  app_buzzer_cfg();

  // Enable 12V
  gpio_set_direction(EN_12V, GPIO_MODE_OUTPUT);
  gpio_set_level(EN_12V, 1);

  // Initialize the lock driver
  gpio_set_direction(EN_LOCK, GPIO_MODE_OUTPUT);
  gpio_set_level(EN_LOCK, 0);

  gpio_set_direction(LED_ON, GPIO_MODE_OUTPUT);
  gpio_set_level(LED_ON, 1);

  gpio_set_direction(LOCK_OPEN, GPIO_MODE_INPUT);
  gpio_set_direction(LOCK_CLOSE, GPIO_MODE_INPUT);

  gpio_pullup_en(LOCK_OPEN);
  gpio_pullup_en(LOCK_CLOSE);

  gpio_set_direction(FRONT_BUT, GPIO_MODE_INPUT);
  gpio_set_direction(BACK_BUT, GPIO_MODE_INPUT);

  // Play a sound
  vTaskDelay(500 / portTICK_PERIOD_MS);

  ESP_LOGD("flexpcb", "FRONT_BUT: %d ", gpio_get_level(FRONT_BUT));
  ESP_LOGD("flexpcb", "BACK_BUT: %d ", gpio_get_level(BACK_BUT));

  while (1) {

    if (gpio_get_level(LOCK_OPEN) == 0) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      beep(1, 1000, 300);
      //   ESP_LOGI("flexpcb", "LOCK_OPEN: %d ", gpio_get_level(LOCK_OPEN));
    } else if (gpio_get_level(LOCK_CLOSE) == 0) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      beep(1, 500, 300);
      //   ESP_LOGI("flexpcb", "LOCK_CLOSE: %d ", gpio_get_level(LOCK_CLOSE));
    } else if (gpio_get_level(FRONT_BUT) == 0) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      beep(1, 500, 300);
      //   ESP_LOGI("flexpcb", "FRONT_BUT: %d ", gpio_get_level(FRONT_BUT));
    } else if (gpio_get_level(BACK_BUT) == 0) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      beep(1, 500, 300);
      //   ESP_LOGI("flexpcb", "BACK_BUT: %d ", gpio_get_level(BACK_BUT));
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}
