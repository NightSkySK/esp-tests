#include "buzzer.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const static char *TAG = "APP_BEEP";

#define BEEP_TIMER LEDC_TIMER_0
#define BEEP_MODE LEDC_HIGH_SPEED_MODE
#define BEEP_CHANNEL LEDC_CHANNEL_0
#define BEEP_DUTY LEDC_TIMER_13_BIT

static ledc_timer_config_t ledc_timer = {
    .duty_resolution = BEEP_DUTY, // resolution of PWM duty
    .freq_hz = 3000,              // 5000,                      // frequency of PWM signal
    .speed_mode = BEEP_MODE,      // timer mode
    .timer_num = BEEP_TIMER       // timer index
};

static ledc_channel_config_t ledc_channel = {
    .channel = BEEP_CHANNEL,
    .duty = 0,
    .gpio_num = 16, // get_beep_io(),
    .speed_mode = BEEP_MODE,
    .timer_sel = BEEP_TIMER};

void app_buzzer_cfg()
{
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);

    ledc_channel_config(&ledc_channel);

    // Initialize fade service.
    ledc_fade_func_install(0);
}

void bz_tone(int freq, int duration)
{
    ESP_LOGD(TAG, "Tone: %d, %d", freq, duration);
    ledc_set_freq(ledc_channel.speed_mode, ledc_channel.channel, freq);
    ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, duration);
    ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
    vTaskDelay(duration / portTICK_PERIOD_MS);

    ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0);
    ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
}

void _tone(int freq, int duration, int silentDuration)
{
    bz_tone(freq, duration);
    vTaskDelay(silentDuration / portTICK_PERIOD_MS);
}

void bend_tones(float initFrequency, float finalFrequency, float prop, long noteDuration, int silentDuration)
{

    // Examples:
    //   bendTones (880, 2093, 1.02, 18, 1);
    //   bendTones (note_A5, note_C7, 1.02, 18, 0);

    if (silentDuration == 0)
    {
        silentDuration = 1;
    }

    if (initFrequency < finalFrequency)
    {
        for (int i = initFrequency; i < finalFrequency; i = i * prop)
        {
            bz_tone(i, noteDuration);
            vTaskDelay(silentDuration / portTICK_PERIOD_MS);
        }
    }
    else
    {

        for (int i = initFrequency; i > finalFrequency; i = i / prop)
        {
            bz_tone(i, noteDuration);
            vTaskDelay(silentDuration / portTICK_PERIOD_MS);
        }
    }
}