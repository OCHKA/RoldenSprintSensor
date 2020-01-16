#include <esp_event.h>
#include <nvs_flash.h>
#include <sys/unistd.h>

#include <cstdio>

#include "coap_server.hpp"
#include "edge_period_sensor.hpp"
#include "network.h"
#include "selftest_gen.hpp"

//#define DEBUG_PRINT_PERIODS

extern "C" {
void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  ESP_ERROR_CHECK(network_init());
  ESP_ERROR_CHECK(coap_server::init());

  ESP_ERROR_CHECK(edge_period_sensor::init());
  ESP_ERROR_CHECK(selftest_gen::init());

  auto col = 0;
  while (true) {
#ifdef DEBUG_PRINT_PERIODS
    auto period = edge_period_sensor::get_avg_period(0, 1000);
#else
    std::optional<bool> period;
    usleep(1000 * 1000);
#endif

    if (period) {
      if (col) {
        printf("\n");
      }

      printf("%u\n", period.value());
      col = 0;
    } else {
      if (col > 80) {
        col = 0;
        printf("\n");
      }

      printf(".");
      fflush(stdout);
      col++;
    }
  }
}
}
