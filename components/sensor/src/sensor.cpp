#include "sensor.hpp"

#include <driver/gpio.h>
#include <driver/timer.h>

#include <array>
#include <atomic>

namespace sensor {

const auto MAX_EDGE_FREQ = 1000e3;
const auto TIMER_FREQ = MAX_EDGE_FREQ * 2;  // Nyquist frequency
const auto TIMER_DIVIDER = TIMER_BASE_CLK / TIMER_FREQ;
const auto TIMER_SCALE = TIMER_BASE_CLK / TIMER_DIVIDER;

struct Sensor {
  Sensor(gpio_num_t pin) : pin(pin), edges_count(0) {}
  Sensor(const Sensor& sensor)
      : pin(sensor.pin), edges_count(static_cast<size_t>(sensor.edges_count)) {}

  const gpio_num_t pin;
  volatile std::atomic_size_t edges_count;
  uint64_t last_edge_timestamp;
};

std::array<Sensor, 2> sensors = {GPIO_NUM_12, GPIO_NUM_14};

void IRAM_ATTR gpio_isr(void* arg) {
  uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);
  uint32_t gpio_intr_status_h = READ_PERI_REG(GPIO_STATUS1_REG);
  SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);
  SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_h);

  for (auto& sensor : sensors) {
    if (gpio_intr_status & BIT(sensor.pin)) {
      sensor.edges_count++;

      timer_get_counter_value(TIMER_GROUP_0, TIMER_0,
                              &sensor.last_edge_timestamp);
    }
  }
}

esp_err_t init() {
  uint64_t pin_mask = 0;

  for (auto& sensor : sensors) {
    pin_mask |= BIT(sensor.pin);
  }

  timer_config_t tim_cfg;
  tim_cfg.divider = TIMER_DIVIDER;
  tim_cfg.counter_dir = TIMER_COUNT_UP;
  tim_cfg.counter_en = TIMER_START;
  tim_cfg.alarm_en = TIMER_ALARM_DIS;
  tim_cfg.auto_reload = TIMER_AUTORELOAD_EN;
  timer_init(TIMER_GROUP_0, TIMER_0, &tim_cfg);

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

uint64_t ticks_to_usec(uint64_t ticks) {
  return ticks / (TIMER_SCALE / 1e6);
}

Sample last_sample(size_t sensor_index) {
  assert(sensor_index < sensors.size() && "Invalid sensor index");

  auto& sensor = sensors.at(sensor_index);

  uint64_t timestamp;
  auto rotations = sensor.edges_count / 2;
  timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &timestamp);

  return {rotations, ticks_to_usec(sensor.last_edge_timestamp)};
}
}  // namespace sensor