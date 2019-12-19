#include "edge_period_sensor.hpp"

#include <driver/gpio.h>
#include <driver/timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>

#include <map>

namespace edge_period_sensor {

const auto EDGE_TIMESTAMP_SAMPLES = 256;

std::map<const gpio_num_t, RingbufHandle_t> sensors = {{GPIO_NUM_15, nullptr}};

void IRAM_ATTR gpio_isr(void* arg) {
  uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);
  uint32_t gpio_intr_status_h = READ_PERI_REG(GPIO_STATUS1_REG);
  SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);
  SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_h);

  for (auto& sensor : sensors) {
    auto& pin = sensor.first;
    auto& ringbuf = sensor.second;

    if (gpio_intr_status & BIT(pin)) {
      edge_timestamp_t timestamp;
      timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &timestamp);
      xRingbufferSendFromISR(ringbuf, &timestamp, sizeof(timestamp), nullptr);
    }
  }
}

esp_err_t init() {
  uint64_t pin_mask = 0;

  for (auto& sensor : sensors) {
    auto& pin = sensor.first;
    auto& ringbuf = sensor.second;

    pin_mask |= BIT(pin);

    auto required_size = sizeof(edge_timestamp_t) * EDGE_TIMESTAMP_SAMPLES;
    ringbuf = xRingbufferCreate(required_size, RINGBUF_TYPE_NOSPLIT);
    if (!ringbuf) {
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
  tim_cfg.divider = 2;
  tim_cfg.counter_dir = TIMER_COUNT_UP;
  tim_cfg.counter_en = TIMER_START;
  tim_cfg.alarm_en = TIMER_ALARM_DIS;
  tim_cfg.auto_reload = TIMER_AUTORELOAD_EN;
  timer_init(TIMER_GROUP_0, TIMER_0, &tim_cfg);

  return ESP_OK;
}

std::optional<EdgeEvent> get_event() {
  size_t index = 0;
  for (auto& sensor : sensors) {
    auto& ringbuf = sensor.second;

    size_t item_size;
    auto timestamp_ptr =
        (edge_timestamp_t*)xRingbufferReceive(ringbuf, &item_size, 0);

    if (timestamp_ptr) {
      EdgeEvent event = {};
      assert(item_size == sizeof(event.timestamp));

      event.timestamp = *timestamp_ptr;
      event.sensor = index;

      vRingbufferReturnItem(ringbuf, timestamp_ptr);
      return event;
    }

    index++;
  }

  return {};
}

}  // namespace edge_period_sensor
