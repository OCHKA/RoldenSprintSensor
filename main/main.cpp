#include <esp_event.h>
#include <mqtt_client.h>
#include <nvs_flash.h>
#include <sys/unistd.h>

#include <string>
#include <vector>

#include "network.h"
#include "sensor.hpp"

#define DEBUG_PRINT 0

static void mqtt_event_handler(void* handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void* event_data) {
  auto event = reinterpret_cast<esp_mqtt_event_handle_t>(event_data);
  //  auto client = event->client;

  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
      //      esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
      break;
    case MQTT_EVENT_DATA:
      //      printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
      //      printf("DATA=%.*s\r\n", event->data_len, event->data);
      break;
    default:
      break;
  }
}

extern "C" {
void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  ESP_ERROR_CHECK(network_init());

  ESP_ERROR_CHECK(sensor::init());

  esp_mqtt_client_config_t mqtt_cfg = {};
  mqtt_cfg.uri = CONFIG_MQTT_BROKER_URI;
  mqtt_cfg.username = "roldensprint";
  mqtt_cfg.password = "roldensprint";

  esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(
      client, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID),
      mqtt_event_handler, client);
  esp_mqtt_client_start(client);

  //  size_t prev_rotations = 0;
  while (true) {
    //    auto rotations = sensor::rotations_count(0);
    //    printf("%u\n", rotations - prev_rotations);
    //    prev_rotations = rotations;

    std::vector<std::string> topics = {"racer0/rotations",
                                       "racer1/rotations"};

    size_t index = 0;
    for (auto& topic : topics) {
      auto rotations = std::to_string(sensor::rotations_count(index));
      esp_mqtt_client_publish(client, topic.c_str(), rotations.c_str(),
                              rotations.size(), 0, 0);
      index++;
    }

    usleep(1000 * 100);
  }
}
}
