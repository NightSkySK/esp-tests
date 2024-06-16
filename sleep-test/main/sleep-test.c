// #include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

static RTC_DATA_ATTR struct timeval sleep_enter_time;

void example_deep_sleep_register_ext0_wakeup(void) {
  const int ext_wakeup_pin_0 = 36;
  printf("Enabling EXT0 wakeup on pin GPIO%d\n", ext_wakeup_pin_0);
  ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(ext_wakeup_pin_0, 1));

  // Configure pullup/downs via RTCIO to tie wakeup pins to inactive level
  // during deepsleep. EXT0 resides in the same power domain (RTC_PERIPH) as the
  // RTC IO pullup/downs. No need to keep that power domain explicitly, unlike
  // EXT1.
  // ESP_ERROR_CHECK(rtc_gpio_pullup_dis(ext_wakeup_pin_0));
  // ESP_ERROR_CHECK(rtc_gpio_pulldown_en(ext_wakeup_pin_0));
}

void example_deep_sleep_register_ext1_wakeup(void) {
  const int ext_wakeup_pin_1 = 13;
  const int ext_wakeup_pin_2 = 14;
  const uint64_t ext_wakeup_pin_1_mask = 1ULL << ext_wakeup_pin_1;
  const uint64_t ext_wakeup_pin_2_mask = 1ULL << ext_wakeup_pin_2;
  printf("Enabling EXT1 wakeup on pins GPIO%d, GPIO%d\n", ext_wakeup_pin_1,
         ext_wakeup_pin_2);

  ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(
      ext_wakeup_pin_1_mask | ext_wakeup_pin_2_mask, ESP_EXT1_WAKEUP_ANY_HIGH));
}

// static void example_deep_sleep_register_rtc_timer_wakeup(void) {
//   const int wakeup_time_sec = 20;
//   printf("Enabling timer wakeup, %ds\n", wakeup_time_sec);
//   ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
// }

static void example_deep_sleep_register_rtc_timer_wakeup(void) {
  // Get the current time
  struct timeval now;
  gettimeofday(&now, NULL);
  struct tm *timeinfo = gmtime(&now.tv_sec);

  printf("Current time: %s", asctime(timeinfo));

  // Set the target wakeup time: 15:15
  struct tm target_time = *timeinfo;
  target_time.tm_hour = 13;
  target_time.tm_min = 15;
  target_time.tm_sec = 0;

  printf("Target time: %02d:%02d:%02d\n", target_time.tm_hour,
         target_time.tm_min, target_time.tm_sec);

  // Get the difference in seconds between now and the target time
  time_t diff_seconds = mktime(&target_time) - now.tv_sec;

  printf("Now (seconds since the Epoch): %lld\n", (long long)now.tv_sec);
  printf("Difference in seconds: %lld\n", (long long)diff_seconds);

  // If the target time is in the past (i.e., earlier today), add 24 hours to
  // the difference
  if (diff_seconds < 0) {
    diff_seconds += 24 * 60 * 60;
  }

  printf("Enabling timer wakeup, %lld seconds from now\n", diff_seconds);
  ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(diff_seconds * 1000000));
}

void app_main(void) {

  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  nvs_handle_t nvs_handle;
  err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
  } else {
    printf("Open NVS done\n");
  }

  // Get deep sleep enter time
  nvs_get_i32(nvs_handle, "slp_enter_sec", (int32_t *)&sleep_enter_time.tv_sec);
  nvs_get_i32(nvs_handle, "slp_enter_usec",
              (int32_t *)&sleep_enter_time.tv_usec);

  struct timeval now;
  gettimeofday(&now, NULL);
  int sleep_time_ms = (now.tv_sec - sleep_enter_time.tv_sec) * 1000 +
                      (now.tv_usec - sleep_enter_time.tv_usec) / 1000;

  switch (esp_sleep_get_wakeup_cause()) {
  case ESP_SLEEP_WAKEUP_TIMER: {
    printf("Wake up from timer. Time spent in deep sleep: %dms\n",
           sleep_time_ms);
    break;
  }

  case ESP_SLEEP_WAKEUP_EXT0: {
    printf("Wake up from ext0\n");
    break;
  }

  case ESP_SLEEP_WAKEUP_EXT1: {
    uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
    if (wakeup_pin_mask != 0) {
      int pin = __builtin_ffsll(wakeup_pin_mask) - 1;
      printf("Wake up from GPIO %d\n", pin);
    } else {
      printf("Wake up from GPIO\n");
    }
    break;
  }
  case ESP_SLEEP_WAKEUP_TOUCHPAD: {
    printf("Wake up from touch on pad %d\n",
           esp_sleep_get_touchpad_wakeup_status());
    break;
  }
  case ESP_SLEEP_WAKEUP_UNDEFINED:
  default:
    printf("Not a deep sleep reset\n");
  }

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  example_deep_sleep_register_ext0_wakeup();
  example_deep_sleep_register_ext1_wakeup();
  example_deep_sleep_register_rtc_timer_wakeup();
  printf("Entering deep sleep\n");

  // get deep sleep enter time
  gettimeofday(&sleep_enter_time, NULL);

  // record deep sleep enter time via nvs
  ESP_ERROR_CHECK(
      nvs_set_i32(nvs_handle, "slp_enter_sec", sleep_enter_time.tv_sec));
  ESP_ERROR_CHECK(
      nvs_set_i32(nvs_handle, "slp_enter_usec", sleep_enter_time.tv_usec));
  ESP_ERROR_CHECK(nvs_commit(nvs_handle));
  nvs_close(nvs_handle);
  esp_deep_sleep_start();
}