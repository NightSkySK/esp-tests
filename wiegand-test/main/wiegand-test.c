#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>
#include <wiegand.h>

static const char *TAG = "wiegand_reader";

static wiegand_reader_t reader;
static QueueHandle_t queue = NULL;

// Single data packet
typedef struct {
  uint8_t data[CONFIG_EXAMPLE_BUF_SIZE];
  size_t bits;
} data_packet_t;

// Key mapping
typedef struct {
  uint8_t code;
  char key;
} key_mapping_t;

static key_mapping_t key_mapping[] = {
    {0x1e, '1'}, {0x2d, '2'}, {0x3c, '3'}, {0x4b, '4'},
    {0x5a, '5'}, {0x69, '6'}, {0x78, '7'}, {0x87, '8'},
    {0x96, '9'}, {0x0f, '0'}, {0xa5, 'E'}, {0xb4, 'T'} // E for ESC, T for ENT
};

// callback on new data in reader
static void reader_callback(wiegand_reader_t *r) {
  // you can decode raw data from reader buffer here, but remember:
  // reader will ignore any new incoming data while executing callback

  // create simple undecoded data packet
  data_packet_t p;
  p.bits = r->bits;
  memcpy(p.data, r->buf, CONFIG_EXAMPLE_BUF_SIZE);

  // Send it to the queue
  xQueueSendToBack(queue, &p, 0);
}

static void task(void *arg) {
  // Create queue
  queue = xQueueCreate(5, sizeof(data_packet_t));
  if (!queue) {
    ESP_LOGE(TAG, "Error creating queue");
    ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
  }

  // Initialize reader
  ESP_ERROR_CHECK(wiegand_reader_init(&reader, CONFIG_KEYPAD1_D0_GPIO,
                                      CONFIG_KEYPAD1_D1_GPIO, true,
                                      CONFIG_EXAMPLE_BUF_SIZE, reader_callback,
                                      WIEGAND_MSB_FIRST, WIEGAND_LSB_FIRST));

  data_packet_t p;
  char input[100] = {0}; // Input buffer
  while (1) {
    // ESP_LOGI(TAG, "Waiting for Wiegand data...");
    printf(".");
    xQueueReceive(queue, &p, portMAX_DELAY);

    // Check if the data is 8 bits and not 0x80
    if (p.bits >= 8 && p.data[0] != 0x80) {
      // Check each key mapping
      for (int i = 0; i < sizeof(key_mapping) / sizeof(key_mapping_t); i++) {
        if (p.data[0] == key_mapping[i].code) {
          // If the key ENT, process the input and clear it
          if (key_mapping[i].key == 'T') {
            printf("\nInput: %s\n", input);
            memset(input, 0, sizeof(input));
          }
          // If the key is ESC, just clear the input
          else if (key_mapping[i].key == 'E') {
            memset(input, 0, sizeof(input));
          } else {
            // Otherwise, add the key to the input
            char str[2] = {key_mapping[i].key, 0};
            strcat(input, str);
          }
          break;
        }
      }
    }
  }
}

void app_main() {
  xTaskCreate(task, TAG, configMINIMAL_STACK_SIZE * 4, NULL, 5, NULL);
}
