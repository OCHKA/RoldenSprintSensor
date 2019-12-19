#include "network.h"

#include <esp_wifi.h>

static SemaphoreHandle_t semaphore;

static void on_connect(void* arg,
                       esp_event_base_t event_base,
                       int32_t event_id,
                       void* event_data) {
  xSemaphoreGive(semaphore);
}

void wifi_connect() {
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  wifi_config_t wifi_config = {
      .sta = {.ssid = "***REMOVED***", .password = "***REMOVED***"}};

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());
}

esp_err_t network_init() {
  semaphore = xSemaphoreCreateBinary();

  esp_netif_init();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_WIFI_STA();
  esp_netif_t* netif = esp_netif_new(&netif_config);
  assert(netif);

  esp_netif_attach_wifi_station(netif);
  esp_wifi_set_default_wifi_sta_handlers();

  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &on_connect, NULL));

  wifi_connect();

  xSemaphoreTake(semaphore, portMAX_DELAY);

  return ESP_OK;
}
