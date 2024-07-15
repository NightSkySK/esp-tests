
#include "driver/gpio.h"
#include <stdio.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "motor_lock.h"
#include "mspacelock.h"

void dc_lock_init(void) {
  gpio_set_direction(DC_PWM, GPIO_MODE_OUTPUT);
  gpio_set_direction(DC_DIRECTION, GPIO_MODE_OUTPUT);
  gpio_set_direction(DC_MODE, GPIO_MODE_OUTPUT);
  gpio_set_direction(DC_FAILURE, GPIO_MODE_INPUT);
  gpio_set_direction(LOCK_OPEN, GPIO_MODE_INPUT);
  gpio_set_direction(LOCK_CLOSE, GPIO_MODE_INPUT);

  gpio_pullup_en(LOCK_OPEN);
  gpio_pullup_en(LOCK_CLOSE);
}

void operate_lock(bool lock_state) {

  ESP_LOGI("motor_lock", "Lock state: %d", lock_state);
  ESP_LOGI("motor_lock", "LOCK_OPEN: %d", gpio_get_level(LOCK_OPEN));
  ESP_LOGI("motor_lock", "LOCK_CLOSE: %d", gpio_get_level(LOCK_CLOSE));

  int64_t t_start = esp_timer_get_time(); // Get current time in microseconds

  if (lock_state == 1) {
    while (gpio_get_level(LOCK_OPEN) == 1) {
      gpio_set_level(DC_DIRECTION, 1);
      gpio_set_level(DC_PWM, 1);
      vTaskDelay(10 / portTICK_PERIOD_MS); // delay for 10 milliseconds
      if ((esp_timer_get_time() - t_start) > 10000 * 1000) {
        ESP_LOGI("motor_lock", "Timeout");
        gpio_set_level(DC_PWM, 0);
        gpio_set_level(DC_MODE, 1);
        break;
      }
    }
    gpio_set_level(DC_PWM, 0);
    gpio_set_level(DC_MODE, 1);
  } else // if (lock_state == 0)
  {
    while (gpio_get_level(LOCK_CLOSE) == 1) {
      gpio_set_level(DC_DIRECTION, 0);
      gpio_set_level(DC_PWM, 1);
      vTaskDelay(10 / portTICK_PERIOD_MS); // delay for 10 milliseconds
      if ((esp_timer_get_time() - t_start) > 10000 * 1000) {
        ESP_LOGI("motor_lock", "Timeout");
        gpio_set_level(DC_PWM, 0);
        gpio_set_level(DC_MODE, 1);
        break;
      }
    }
    gpio_set_level(DC_PWM, 0);
    gpio_set_level(DC_MODE, 1);
  }
}