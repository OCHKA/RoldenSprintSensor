#include "edge_period_sensor.hpp"

#include <driver/gpio.h>
#include <driver/timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <vector>

namespace edge_period_sensor {

using edge_timestamp_t = uint64_t;

const auto TIMER_DIVIDER = 2;
const auto TIMER_SCALE = TIMER_BASE_CLK / TIMER_DIVIDER;

struct Sensor {
  Sensor(gpio_num_t pin) : pin(pin) {}

  const gpio_num_t pin;
  edge_timestamp_t prev_edge;

  struct Average {
    uint64_t sum = 0;
    size_t count = 0;
    SemaphoreHandle_t semaphore;
  } avg_period;
};

std::vector<Sensor> sensors = {GPIO_NUM_15};

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

      // Assumes that reader is locking on semaphore
      // After writing here we have a little bit of time before next ISR to
      // calculate average value
      auto& average = sensor.avg_period;
      average.sum += period;
      average.count++;

      if (uxSemaphoreGetCount(average.semaphore) == 0) {
        xSemaphoreGiveFromISR(average.semaphore, nullptr);
      }
    }
  }
}

esp_err_t init() {
  uint64_t pin_mask = 0;

  for (auto& sensor : sensors) {
    pin_mask |= BIT(sensor.pin);
    sensor.avg_period.semaphore = xSemaphoreCreateBinary();
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

std::optional<period_t> get_avg_period(size_t sensor_index, time_t timeout_ms) {
  if (sensor_index >= sensors.size()) {
    return {};
  }

  auto& average = sensors[sensor_index].avg_period;
  if (xSemaphoreTake(average.semaphore, pdMS_TO_TICKS(timeout_ms))) {
    // Hope for the god of timings that this will go before next ISR

    if (!average.count) { // in case of ISR from ringing it may die here
      return {};
    }

    period_t avg_ticks = average.sum / average.count;
    average.sum = 0;
    average.count = 0;

    return avg_ticks / (TIMER_SCALE / 1e6);  // convert to microseconds
  } else {
    return {};
  }
}

}  // namespace edge_period_sensor
