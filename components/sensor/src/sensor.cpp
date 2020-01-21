#include "sensor.hpp"

#include <driver/gpio.h>

#include <atomic>
#include <vector>

namespace sensor {

struct Sensor {
  Sensor(gpio_num_t pin) : pin(pin), edges_count(0) {}
  Sensor(const Sensor& sensor)
      : pin(sensor.pin), edges_count(static_cast<size_t>(sensor.edges_count)) {}

  const gpio_num_t pin;
  std::atomic_size_t edges_count;
};

std::vector<Sensor> sensors = {GPIO_NUM_15};

void IRAM_ATTR gpio_isr(void* arg) {
  uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);
  uint32_t gpio_intr_status_h = READ_PERI_REG(GPIO_STATUS1_REG);
  SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);
  SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_h);

  for (auto& sensor : sensors) {
    if (gpio_intr_status & BIT(sensor.pin)) {
      sensor.edges_count++;
    }
  }
}

esp_err_t init() {
  uint64_t pin_mask = 0;

  for (auto& sensor : sensors) {
    pin_mask |= BIT(sensor.pin);
  }

  gpio_config_t io_cfg;
  io_cfg.intr_type = GPIO_INTR_ANYEDGE;  // ESP32 seems to not be able to
                                         // trigger on specific edge
  io_cfg.mode = GPIO_MODE_INPUT;
  io_cfg.pin_bit_mask = pin_mask;
  io_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_cfg);
  gpio_isr_register(gpio_isr, nullptr, ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_EDGE,
                    nullptr);

  return ESP_OK;
}

size_t sensors_count() {
  return sensors.size();
}

size_t rotations_count(size_t sensor_index) {
  assert(sensor_index < sensors_count() && "Invalid sensor index");

  auto& edges = sensors.at(sensor_index).edges_count;
  return edges / 2;  // On each rotation we have rising and falling edge
}

}  // namespace sensor
