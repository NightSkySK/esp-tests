#include <stdio.h>
#include <stdlib.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_oneshot.h"
#else
#include "driver/adc.h"
#include "esp_adc_cal.h"
#endif

#include "mspacelock.h"

static const char *TAG = "ADC";

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)

static adc_cali_handle_t adc_cali_handle;
static adc_oneshot_unit_handle_t adc_handle;
static const adc_oneshot_chan_cfg_t channel_config = {
    .bitwidth = ADC_BITWIDTH_12,
    .atten = ADC_ATTEN_DB_0,
};

#else
// ADC Calibration
#if CONFIG_IDF_TARGET_ESP32
#define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_VREF
#elif CONFIG_IDF_TARGET_ESP32S2
#define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP
#elif CONFIG_IDF_TARGET_ESP32C3
#define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP
#elif CONFIG_IDF_TARGET_ESP32S3
#define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_TP_FIT
#endif

static esp_adc_cal_characteristics_t adc1_chars;
#if !CONFIG_IDF_TARGET_ESP32C3
// ESP32C3 ADC2 single mode is no longer supported
static esp_adc_cal_characteristics_t adc2_chars;
#endif

#endif

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten,
                                 adc_cali_handle_t *out_handle) {
  adc_cali_handle_t handle = NULL;
  esp_err_t ret = ESP_FAIL;
  bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
  if (!calibrated) {
    ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
  if (!calibrated) {
    ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

  *out_handle = handle;
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "Calibration Success");
  } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
    ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
  } else {
    ESP_LOGE(TAG, "Invalid arg or no memory");
  }

  // If calibration failed, then this board just doesn't support it.
  // Return `true` in order to prevent a memory leak from constantly
  // reinitializing the ADC.
  return true;
}

static bool adc_init(adc_cali_handle_t *adc_cali_handle,
                     adc_oneshot_unit_handle_t *adc_handle, adc_unit_t unit) {
  const adc_oneshot_unit_init_cfg_t init_config = {
      .unit_id = unit,
  };
  if (!adc_calibration_init(unit, ADC_ATTEN_DB_0, adc_cali_handle)) {
    return false;
  }
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, adc_handle));
  return true;
}

#else
static bool adc_calibration_init(void) {
  esp_err_t ret;
  bool cali_enable = false;

  ret = esp_adc_cal_check_efuse(ADC_EXAMPLE_CALI_SCHEME);
  if (ret == ESP_ERR_NOT_SUPPORTED) {
    ESP_LOGW(TAG, "Calibration scheme not supported, skip software "
                  "calibration");
  } else if (ret == ESP_ERR_INVALID_VERSION) {
    ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
  } else if (ret == ESP_OK) {
    cali_enable = true;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, 0,
                             &adc1_chars);
#if !CONFIG_IDF_TARGET_ESP32C3
    esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, 0,
                             &adc2_chars);
#endif
  } else {
    ESP_LOGE(TAG, "Invalid arg");
  }

  return cali_enable;
}
#endif

void voltage_check(void) {
  // Enable BAT_ADC_EN
  gpio_set_direction(BAT_ADC_EN, GPIO_MODE_OUTPUT);
  gpio_set_level(BAT_ADC_EN, 1);

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)

  int raw_voltage;
  int m_voltage = 0;

  adc_channel_t channel = ADC_CHANNEL_7;

  adc_init(&adc_cali_handle, &adc_handle, ADC_UNIT_1);
  adc_oneshot_config_channel(adc_handle, channel, &channel_config);

  vTaskDelay(pdMS_TO_TICKS(1000));
  ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, channel, &raw_voltage));
  ESP_LOGD(TAG, "raw  data: %d", raw_voltage);

  ESP_ERROR_CHECK(
      adc_cali_raw_to_voltage(adc_cali_handle, raw_voltage, &m_voltage));
  ESP_LOGI(TAG, "ADC%" PRIu8 " Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1,
           ADC_CHANNEL_7, m_voltage);
  voltage = 10.9 * (float)m_voltage / 1000.0;
  ESP_LOGI(TAG, "Voltage adjusted read: %f", voltage);

#else

  uint16_t raw_voltage;
  uint32_t m_voltage = 0;

  bool cali_enable = adc_calibration_init();

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_0);
  vTaskDelay(pdMS_TO_TICKS(1000));
  raw_voltage = adc1_get_raw(ADC1_CHANNEL_7);

  ESP_LOGI(TAG, "raw  data: %" PRIu16, raw_voltage);
  if (cali_enable) {
    m_voltage = esp_adc_cal_raw_to_voltage(raw_voltage, &adc1_chars);
    ESP_LOGD(TAG, "cali data: %" PRIu32 " mV", m_voltage);
    voltage = 10.96 * (float)m_voltage / 1000.0;
    ESP_LOGI(TAG, "Voltage adjusted read: %f", voltage);
  } else {
    voltage = 10.96 * ((float)raw_voltage * 1.1 / 4095.0);
    ESP_LOGI(TAG, "Voltage unadjusted read: %f", voltage);
  }

#endif
  gpio_set_level(BAT_ADC_EN, 0);
}