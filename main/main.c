#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <sys/unistd.h>

#include <coap.h>

#include <driver/gpio.h>
#include <driver/timer.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

const gpio_num_t GPIO_SENSOR_1 = GPIO_NUM_15;

static uint64_t ticks_at_edge = 0;

static void IRAM_ATTR sensor_isr(void *arg) {
  uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);
  uint32_t gpio_intr_status_h = READ_PERI_REG(GPIO_STATUS1_REG);
  SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);
  SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_h);

  if (gpio_intr_status & BIT(GPIO_SENSOR_1)) {
    timer_get_counter_value(0, TIMER_0, &ticks_at_edge);
  }
}

static void sensor_init(void) {
  gpio_config_t io_cfg;
  io_cfg.intr_type = GPIO_INTR_POSEDGE;
  io_cfg.mode = GPIO_MODE_INPUT;
  io_cfg.pin_bit_mask = 1ULL << GPIO_SENSOR_1;
  io_cfg.pull_down_en = 1;
  gpio_config(&io_cfg);
  gpio_isr_register(sensor_isr, NULL, ESP_INTR_FLAG_IRAM, NULL);

  timer_config_t tim_cfg;
  tim_cfg.divider = 2;
  tim_cfg.counter_dir = TIMER_COUNT_UP;
  tim_cfg.counter_en = TIMER_START;
  tim_cfg.alarm_en = TIMER_ALARM_DIS;
  tim_cfg.auto_reload = TIMER_AUTORELOAD_EN;
  timer_init(0, TIMER_0, &tim_cfg);
}

SemaphoreHandle_t net_semaphore;

static void on_net_connected(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data) {
  ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
  ESP_LOGI("net", "IPv4 address: " IPSTR, IP2STR(&event->ip_info.ip));
  xSemaphoreGive(net_semaphore);
}

esp_err_t net_init(void) {
  net_semaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(net_semaphore);

  esp_netif_init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_WIFI_STA();
  esp_netif_t *netif = esp_netif_new(&netif_config);

  esp_netif_attach_wifi_station(netif);
  esp_wifi_set_default_wifi_sta_handlers();

  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &on_net_connected, NULL));

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = "***REMOVED***",
              .password = "***REMOVED***",
          },
  };

  ESP_LOGI("net", "Connecting to %s...", wifi_config.sta.ssid);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());

  xSemaphoreTake(net_semaphore, portMAX_DELAY);
  return ESP_OK;
}

void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  sensor_init();
  net_init();

  while (true) {
    printf("GPIO: %llu\n", ticks_at_edge);
    sleep(1);
  }
}