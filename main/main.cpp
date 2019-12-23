#include <esp_event.h>
#include <nvs_flash.h>
#include <sys/unistd.h>

#include <cstdio>

#include "coap_server.hpp"
#include "edge_period_sensor.hpp"
#include "network.h"

extern "C" {
void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  ESP_ERROR_CHECK(network_init());
  ESP_ERROR_CHECK(coap_server::init());

  ESP_ERROR_CHECK(edge_period_sensor::init());

  while (true) {
    for (auto col = 0; col < 80; col++) {
      printf(".");
      fflush(stdout);
      usleep(1000 * 1000);
    }
    printf("\n");
  }
}
}
