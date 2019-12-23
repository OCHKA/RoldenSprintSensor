#include "edge_period_sensor.hpp"

#include <driver/gpio.h>
#include <driver/timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>

#include <vector>

namespace edge_period_sensor {

using edge_timestamp_t = uint64_t;

const auto EDGE_TIMESTAMP_SAMPLES = 256;
const auto TIMER_DIVIDER = 2;
const auto TIMER_SCALE = TIMER_BASE_CLK / TIMER_DIVIDER;

struct Sensor {
  const gpio_num_t pin;
  RingbufHandle_t periods;
  edge_timestamp_t prev_edge;
};
std::vector<Sensor> sensors = {{GPIO_NUM_15, {}, {}}};

void IRAM_ATTR gpio_isr(void* arg) {
  uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);
  uint32_t gpio_intr_status_h = READ_PERI_REG(GPIO_STATUS1_REG);
  SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);
  SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_h);

  for (auto& sensor : sensors) {
    if (gpio_intr_status & BIT(sensor.pin)) {
      edge_timestamp_t edge;
      timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &edge);

      period_t period = edge - sensor.prev_edge;
      sensor.prev_edge = edge;

      xRingbufferSendFromISR(sensor.periods, &period, sizeof(period), nullptr);
    }
  }
}

esp_err_t init() {
  uint64_t pin_mask = 0;

  for (auto& sensor : sensors) {
    pin_mask |= BIT(sensor.pin);

    auto required_size = sizeof(period_t) * EDGE_TIMESTAMP_SAMPLES;
    sensor.periods = xRingbufferCreate(required_size, RINGBUF_TYPE_NOSPLIT);
    if (!sensor.periods) {
      return ESP_FAIL;
    }
  }

  gpio_config_t io_cfg;
  io_cfg.intr_type = GPIO_INTR_POSEDGE;
  io_cfg.mode = GPIO_MODE_INPUT;
  io_cfg.pin_bit_mask = pin_mask;
  io_cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;
  gpio_config(&io_cfg);
  gpio_isr_register(gpio_isr, nullptr, ESP_INTR_FLAG_IRAM, nullptr);

  timer_config_t tim_cfg;
  tim_cfg.divider = TIMER_DIVIDER;
  tim_cfg.counter_dir = TIMER_COUNT_UP;
  tim_cfg.counter_en = TIMER_START;
  tim_cfg.alarm_en = TIMER_ALARM_DIS;
  tim_cfg.auto_reload = TIMER_AUTORELOAD_EN;
  timer_init(TIMER_GROUP_0, TIMER_0, &tim_cfg);

  return ESP_OK;
}

size_t sensors_count() {
  return sensors.size();
}

std::optional<period_t> get_period(size_t sensor_index) {
  if (sensor_index >= sensors.size()) {
    return {};
  }

  auto& sensor = sensors[sensor_index];

  size_t item_size;
  auto ptr = (period_t*)xRingbufferReceive(sensor.periods, &item_size, 0);

  if (ptr) {
    assert(item_size == sizeof(period_t));

    auto ticks = *ptr;
    // convert ticks to microseconds
    auto period = ticks / (TIMER_SCALE / 1e6);

    vRingbufferReturnItem(sensor.periods, ptr);
    return period;
  }

  return {};
}

}  // namespace edge_period_sensor
