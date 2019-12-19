#include <esp_event.h>
#include <nvs_flash.h>
#include <stdio.h>
#include <sys/unistd.h>

#include <coap_server.hpp>
#include <edge_period_sensor.hpp>

extern "C" {
void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  //  ESP_ERROR_CHECK(network_init());
  ESP_ERROR_CHECK(coap_server::init());
  ESP_ERROR_CHECK(edge_period_sensor::init());
  //  rpm_sensor_init();

  while (true) {
    //    printf("GPIO: %llu\n", ticks_at_edge);
    sleep(1);
  }
}
}