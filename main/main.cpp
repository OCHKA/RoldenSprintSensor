#include <esp_event.h>
#include <nvs_flash.h>
#include <sys/unistd.h>

#include "coap_server.hpp"
#include "edge_period_sensor.hpp"
#include "network.h"
#include "selftest_gen.hpp"

extern "C" {
void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  ESP_ERROR_CHECK(network_init());
  ESP_ERROR_CHECK(coap_server::init());

  ESP_ERROR_CHECK(edge_period_sensor::init());
  ESP_ERROR_CHECK(selftest_gen::init());

  while (true) {
//    auto period = edge_period_sensor::get_avg_period(0);
//    if (period) {
//      printf("%u\n", period.value());
//    }

    usleep(1000 * 1000);
  }
}
}
