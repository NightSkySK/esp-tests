
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "hal/gpio_types.h"

#include "blink_codes.h"
#include "buzzer.h"
#include "motor_lock.h"
#include "mspacelock.h"

const char *TAG = "mSpaceLatch";
TaskHandle_t error_task_handle = NULL;
int error_code = 0;

float voltage;

void app_main(void) {

  esp_log_level_set("*", ESP_LOG_DEBUG);

  // Initialize the buzzer
  app_buzzer_cfg();

  // Initialization error code tasks with 0 Code as initial value
  error_code = 0;
  xTaskCreate(&error_blink_task, "error_blink_task", 2048, &error_code, 5,
              &error_task_handle);

  // Enable 12V
  ESP_LOGI(TAG, "GPIO setup");
  gpio_set_direction(EN_12V, GPIO_MODE_OUTPUT);
  gpio_set_level(EN_12V, 1);

  // Initialize the lock driver
  gpio_set_direction(EN_LOCK, GPIO_MODE_OUTPUT);
  gpio_set_level(EN_LOCK, 0);

  gpio_set_direction(LED_ON, GPIO_MODE_OUTPUT);
  gpio_set_level(LED_ON, 1);

  gpio_set_direction(FRONT_BUT, GPIO_MODE_INPUT);
  gpio_set_direction(BACK_BUT, GPIO_MODE_INPUT);

  ESP_LOGI(TAG, "DC Init");
  dc_lock_init();

  // Play a sound
  vTaskDelay(500 / portTICK_PERIOD_MS);

  // Check the voltage
  voltage_check();
  ESP_LOGI(TAG, "Voltage: %f", voltage);
  if (voltage < 7.0) {
    error_code = 8;
    update_error_code();
    while (gpio_get_level(BACK_BUT)) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "Restarting");
    esp_restart();
  }

  ESP_LOGI(TAG, "FRONT_BUT: %d ", gpio_get_level(FRONT_BUT));
  ESP_LOGI(TAG, "BACK_BUT: %d ", gpio_get_level(BACK_BUT));
  ESP_LOGI(TAG, "LOCK_CLOSE: %d ", gpio_get_level(LOCK_CLOSE));
  ESP_LOGI(TAG, "LOCK_OPEN: %d ", gpio_get_level(LOCK_OPEN));

  // Wait for the front button to be pressed
  while (gpio_get_level(FRONT_BUT)) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  if (gpio_get_level(LOCK_CLOSE) && gpio_get_level(LOCK_OPEN)) {
    operate_lock(0);
    ESP_LOGI(TAG, "Lock should be closed, if not then error code 1");
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    if (gpio_get_level(LOCK_CLOSE) == 1) {
      error_code = 1;
      update_error_code();
      while (gpio_get_level(BACK_BUT)) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
      }
      ESP_LOGI(TAG, "Restarting");
      esp_restart();
    }
  }

  int8_t counter = 0;

  while (gpio_get_level(BACK_BUT) && counter <= 10) {

    ESP_LOGI(TAG, "Counter: %d", counter);

    if (gpio_get_level(LOCK_CLOSE) == 0) {
      ESP_LOGI(TAG, "Lock is closed, opening...");
      operate_lock(1);
      counter++;
      if (gpio_get_level(LOCK_OPEN) == 1) {
        error_code = 2;
        update_error_code();
        while (gpio_get_level(BACK_BUT)) {
          vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Restarting");
        esp_restart();
      }
    } else if (gpio_get_level(LOCK_OPEN) == 0) {
      operate_lock(0);
      counter++;
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      if (gpio_get_level(LOCK_CLOSE) == 1) {
        error_code = 3;
        update_error_code();
        while (gpio_get_level(BACK_BUT)) {
          vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Restarting");
        esp_restart();
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  if (gpio_get_level(LOCK_CLOSE) == 1) {
    operate_lock(0);
    counter++;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    if (gpio_get_level(LOCK_CLOSE) == 1) {
      error_code = 3;
      update_error_code();
      while (gpio_get_level(BACK_BUT)) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
      }
      ESP_LOGI(TAG, "Restarting");
      esp_restart();
    }
  }
  ESP_LOGI(TAG, "Restarting end");
  for (int i = 1; i < 5; i++) {

    bz_tone(300 * i, 200);
  }

  esp_restart();
}
