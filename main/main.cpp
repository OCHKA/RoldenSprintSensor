#include <esp_event.h>
#include <nvs_flash.h>
#include <sys/unistd.h>

#include <charconv>
#include <cstdio>

#include "coap_server.hpp"
#include "edge_period_sensor.hpp"
#include "network.h"

namespace {
std::optional<uint32_t> period_req_handler(std::string_view query) {
  int index;
  auto conv_result =
      std::from_chars(query.data(), query.data() + query.size(), index);

  auto max_index = edge_period_sensor::sensors_count() - 1;
  if (conv_result.ec != std::errc() || index > max_index || index < 0) {
    return {};
  }

  // average pending periods before returning them
  uint64_t sum = 0;
  size_t count = 0;
  while (auto period = edge_period_sensor::get_period(index)) {
    sum += period.value();
    count++;
  };

  if (count) {
    return sum / count;
  } else {
    return {};
  }
}
}  // namespace

extern "C" {
void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  ESP_ERROR_CHECK(network_init());
  ESP_ERROR_CHECK(coap_server::init());
  coap_server::get_req_cb_register(&period_req_handler);

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
