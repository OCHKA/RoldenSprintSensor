#include "network.h"

#include <esp_log.h>
#include <esp_wifi.h>
#include <string.h>

const char* TAG = "net";

static void event_handler(void* arg,
                          esp_event_base_t event_base,
                          int32_t event_id,
                          void* event_data) {
  if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t* event =
        (wifi_event_ap_staconnected_t*)event_data;
    ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac),
             event->aid);
  } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t* event =
        (wifi_event_ap_stadisconnected_t*)event_data;
    ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac),
             event->aid);
  }
}

static void wifi_init_softap() {
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  wifi_config_t wifi_config = {.ap = {.ssid = CONFIG_WIFI_SSID,
                                      .password = CONFIG_WIFI_PASSWORD,
                                      .ssid_len = strlen(CONFIG_WIFI_SSID),
                                      .max_connection = CONFIG_MAX_STA_CONN,
                                      .authmode = WIFI_AUTH_WPA_WPA2_PSK}};

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "SoftAP SSID: %s password: %s",
           CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);
}

esp_err_t network_init() {
  ESP_ERROR_CHECK(esp_netif_init());
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &event_handler, NULL));

  wifi_init_softap();

  return ESP_OK;
}
