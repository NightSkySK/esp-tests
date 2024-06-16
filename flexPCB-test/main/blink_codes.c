
#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"

#include "blink_codes.h"
#include "buzzer.h"

void beep(int beeps, int frequency, int beep_duration) {
  // app_buzzer_cfg();
  for (int i = 0; i < beeps; i++) {
    bz_tone(frequency, beep_duration);
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }
}

void play(int soundName) {
  // app_buzzer_cfg();
  switch (soundName) {

  case S_CONNECTION:
    _tone(NOTE_E5, 50, 30);
    _tone(NOTE_E6, 55, 25);
    _tone(NOTE_A6, 60, 10);
    break;

  case S_DISCONNECTION:
    _tone(NOTE_E5, 50, 30);
    _tone(NOTE_A6, 55, 25);
    _tone(NOTE_E6, 50, 60);
    break;

  case S_BUTTON_PUSHED:
    bend_tones(NOTE_E6, NOTE_G6, 1.03, 200, 20);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    bend_tones(NOTE_E6, NOTE_D7, 1.04, 100, 20);
    break;

  case S_MODE1:
    bend_tones(NOTE_E4, NOTE_A5, 1.05, 120, 20); // 1318.51 to 1760
    break;

  case S_MODE2:
    bend_tones(NOTE_A5, NOTE_E4, 1.05, 120, 20); // 1567.98 to 2349.32
    break;

  case S_MODE3:
    _tone(NOTE_E6, 50, 100); // D6
    _tone(NOTE_G6, 50, 80);  // E6
    _tone(NOTE_D7, 300, 0);  // G6
    break;

  case S_SURPRISE:
    bend_tones(800, 2150, 1.02, 10, 1);
    bend_tones(2149, 800, 1.03, 7, 1);
    break;

  case S_JUMP:
    bend_tones(880, 2000, 1.04, 8, 3); // A5 = 880
    vTaskDelay(200 / portTICK_PERIOD_MS);
    break;

  case S_OHOOH:
    bend_tones(880, 2000, 1.04, 8, 3); // A5 = 880
    vTaskDelay(200 / portTICK_PERIOD_MS);

    for (int i = 880; i < 2000; i = i * 1.04) {
      _tone(NOTE_B5, 5, 10);
    }
    break;

  case S_OHOOH2:
    bend_tones(1880, 3000, 1.03, 8, 3);
    vTaskDelay(200 / portTICK_PERIOD_MS);

    for (int i = 1880; i < 3000; i = i * 1.03) {
      _tone(NOTE_C6, 10, 10);
    }
    break;

  case S_CUDDLY:
    bend_tones(700, 900, 1.03, 16, 4);
    bend_tones(899, 650, 1.01, 18, 7);
    break;

  case S_SLEEPING:
    bend_tones(100, 500, 1.04, 10, 10);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    bend_tones(400, 100, 1.04, 10, 1);
    break;

  case S_HAPPY:
    bend_tones(1500, 2500, 1.05, 20, 8);
    bend_tones(2499, 1500, 1.05, 25, 8);
    break;

  case S_SUPER_HAPPY:
    bend_tones(2000, 6000, 1.05, 8, 3);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    bend_tones(5999, 2000, 1.05, 13, 2);
    break;

  case S_HAPPY_SHORT:
    bend_tones(1500, 2000, 1.05, 15, 8);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    bend_tones(1900, 2500, 1.05, 10, 8);
    break;

  case S_SAD:
    bend_tones(880, 669, 1.02, 20, 200);
    break;

  case S_CONFUSED:
    bend_tones(1000, 1700, 1.03, 8, 2);
    bend_tones(1699, 500, 1.04, 8, 3);
    bend_tones(1000, 1700, 1.05, 9, 10);
    break;

  case S_FART1:
    bend_tones(1600, 3000, 1.02, 2, 15);
    break;

  case S_FART2:
    bend_tones(2000, 6000, 1.02, 2, 20);
    break;

  case S_FART3:
    bend_tones(1600, 4000, 1.02, 2, 20);
    bend_tones(4000, 3000, 1.02, 2, 20);
    break;

  case PIRATES:
    break;
  }
}