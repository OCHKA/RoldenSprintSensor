#include "edge_period_sensor.hpp"

#include <driver/gpio.h>
#include <driver/timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <vector>

namespace edge_period_sensor {

using edge_timestamp_t = uint64_t;

const auto MAX_EDGE_FREQ = 1000e3;
const auto TIMER_FREQ = MAX_EDGE_FREQ * 2;  // Nyquist frequency
const auto TIMER_DIVIDER = TIMER_BASE_CLK / TIMER_FREQ;
const auto TIMER_SCALE = TIMER_BASE_CLK / TIMER_DIVIDER / 10;

struct Sensor {
  Sensor(gpio_num_t pin) : pin(pin) {}

  const gpio_num_t pin;
  edge_timestamp_t prev_edge = 0;
  edge_timestamp_t prev_negative_edge = 0;

  SemaphoreHandle_t semaphore;

  struct PeriodHistory {
    static const auto size = 10;
    period_t periods[size] = {};
    size_t position;
  } history;
};

std::vector<Sensor> sensors = {GPIO_NUM_15};

void IRAM_ATTR gpio_isr(void* arg) {
  uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);
  uint32_t gpio_intr_status_h = READ_PERI_REG(GPIO_STATUS1_REG);
  SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);
  SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_h);

  edge_timestamp_t edge;
  timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &edge);

  for (auto& sensor : sensors) {
    if (gpio_intr_status & BIT(sensor.pin)) {
      period_t period = edge - sensor.prev_edge;
      period_t negative_period = sensor.prev_edge - sensor.prev_negative_edge;

      sensor.prev_negative_edge = sensor.prev_edge;
      sensor.prev_edge = edge;

      auto& history = sensor.history;
      if (history.position + 1 >= history.size) {
        history.position = 0;
      }
      history.periods[history.position] = period + negative_period;

      // Assumes that reader is locking on semaphore
      // After writing here we have a little bit of time before next ISR to
      // calculate average value
      xSemaphoreGiveFromISR(sensor.semaphore, nullptr);
    }
  }
}

esp_err_t init() {
  uint64_t pin_mask = 0;

  for (auto& sensor : sensors) {
    pin_mask |= BIT(sensor.pin);
    sensor.semaphore = xSemaphoreCreateBinary();
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

std::optional<period_t> get_avg_period(size_t sensor_index) {
  if (sensor_index >= sensors.size()) {
    return {};
  }

  auto& sensor = sensors[sensor_index];
  if (xSemaphoreTake(sensor.semaphore, 0)) {
    // Hope for the god of timings that this will go before next ISR

    auto& history = sensor.history;
    uint64_t avg_sum = 0;
    size_t avg_count = 0;

    for (auto i = 0; i < history.size; i++) {
      avg_sum += history.periods[i];
      avg_count++;
    }

    auto avg_period = avg_sum / avg_count;
    return avg_period / (TIMER_SCALE / 1e6);  // convert to microseconds
  }

  return {};
}

}  // namespace edge_period_sensor
