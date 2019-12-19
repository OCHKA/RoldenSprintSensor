#include "edge_period_sensor.hpp"

#include <driver/gpio.h>
#include <driver/timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>

#include <map>

static uint64_t ticks_at_edge = 0;

const auto EDGE_TIMESTAMP_SAMPLES = 256;
std::map<const gpio_num_t, RingbufHandle_t> sensors = {{GPIO_NUM_15, nullptr}};

namespace {

void IRAM_ATTR gpio_isr(void* arg) {
  uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);
  uint32_t gpio_intr_status_h = READ_PERI_REG(GPIO_STATUS1_REG);
  SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);
  SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_h);

  for () {
    if (gpio_intr_status & BIT(SENSOR_PINS[i])) {
      timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &ticks_at_edge);
    }
  }
}

}  // namespace

namespace edge_period_sensor {

esp_err_t init() {
  uint64_t pin_mask = 0;
  for (auto& sensor_item : sensors) {
    auto& pin = sensor_item.first;
    pin_mask |= BIT(pin);

    auto& ringbuf = sensor_item.second;
    ringbuf = xRingbufferCreate(sizeof(uint64_t) * EDGE_TIMESTAMP_SAMPLES,
                                RINGBUF_TYPE_NOSPLIT);
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
  gpio_isr_register(gpio_isr, NULL, ESP_INTR_FLAG_IRAM, NULL);

  timer_config_t tim_cfg;
  tim_cfg.divider = 2;
  tim_cfg.counter_dir = TIMER_COUNT_UP;
  tim_cfg.counter_en = TIMER_START;
  tim_cfg.alarm_en = TIMER_ALARM_DIS;
  tim_cfg.auto_reload = TIMER_AUTORELOAD_EN;
  timer_init(TIMER_GROUP_0, TIMER_0, &tim_cfg);

  return ESP_OK;
}

}  // namespace edge_period_sensor
