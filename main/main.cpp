#include <esp_event.h>
#include <nvs_flash.h>
#include <sys/unistd.h>

#include "coap_server.hpp"
#include "sensor.hpp"
#include "network.h"

#define DEBUG_PRINT 0

extern "C" {
void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  ESP_ERROR_CHECK(network_init());
  ESP_ERROR_CHECK(coap_server::init());

  ESP_ERROR_CHECK(sensor::init());

//  size_t prev_rotations = 0;
  while (true) {
//    auto rotations = sensor::rotations_count(0);
//    printf("%u\n", rotations - prev_rotations);
//    prev_rotations = rotations;

    usleep(1000 * 1000);
  }
}
}
