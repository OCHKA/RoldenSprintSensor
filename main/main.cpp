#include <esp_event.h>
#include <mqtt_client.h>
#include <nvs_flash.h>
#include <sys/unistd.h>

#include <atomic>
#include <sensor.hpp>
#include <string>
#include <vector>

#include "network.h"
#include "sensor.hpp"

std::atomic_bool is_mqtt_connected = false;

static void mqtt_event_handler(void* handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void* event_data) {
  auto event = reinterpret_cast<esp_mqtt_event_handle_t>(event_data);
  //  auto client = event->client;

  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
      is_mqtt_connected = true;
      //      esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
      break;
    case MQTT_EVENT_DISCONNECTED:
      is_mqtt_connected = false;
    case MQTT_EVENT_DATA:
      //      printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
      //      printf("DATA=%.*s\r\n", event->data_len, event->data);
      break;
    default:
      break;
  }
}

void send_sensor_samples(esp_mqtt_client_handle_t mqttc) {
  static const auto topics = {"racer0/rotations", "racer1/rotations"};

  size_t index = 0;
  for (auto& topic : topics) {
    auto sample = sensor::last_sample(index);

    char buffer[128];
    auto msg_len = snprintf(buffer, sizeof(buffer), "[%u,%llu]",
                            sample.rotations, sample.edge_timestamp);

    esp_mqtt_client_publish(mqttc, topic, buffer, msg_len, 0, 0);
    index++;
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

  esp_mqtt_client_handle_t mqttc = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(
      mqttc, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID),
      mqtt_event_handler, mqttc);
  esp_mqtt_client_start(mqttc);

  while (true) {
    if (is_mqtt_connected) {
      send_sensor_samples(mqttc);
      usleep(1000 * 100);
    } else {
      usleep(1000 * 1000);
    }
  }
}
}
