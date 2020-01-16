#include "selftest_gen.hpp"

#include <driver/gpio.h>
#include <driver/ledc.h>
#include <driver/timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <array>

namespace selftest_gen {

const char* TAG = "selftest_gen";

auto const OUT_PIN = GPIO_NUM_21;

auto const PWM_SPEED_MODE = LEDC_HIGH_SPEED_MODE;
auto const PWM_TIMER = LEDC_TIMER_0;
auto const PWM_CHANNEL = LEDC_CHANNEL_0;
auto const PWM_RESOLUTION = LEDC_TIMER_12_BIT;
auto const PWM_DUTY = BIT(PWM_RESOLUTION) / 8;

std::array freq_pattern = {50, 48, 49, 44, 42, 41, 43, 40, 38, 37, 36, 38, 35, 32, 30, 31, 28, 27, 25, 27, 23};

void selftest_task(void* p) {
  while (true) {
    for (auto new_freq : freq_pattern) {
      ledc_set_freq(PWM_SPEED_MODE, PWM_TIMER, new_freq);
      usleep(5 * 100 * 1000);
    }
  }
}

esp_err_t init() {
  gpio_config_t io_cfg = {};
  io_cfg.mode = GPIO_MODE_OUTPUT;
  io_cfg.pin_bit_mask = BIT(OUT_PIN);
  gpio_config(&io_cfg);

  ledc_timer_config_t tim_cfg = {};
  tim_cfg.duty_resolution = PWM_RESOLUTION;
  tim_cfg.freq_hz = freq_pattern.at(0);
  tim_cfg.speed_mode = PWM_SPEED_MODE;
  tim_cfg.timer_num = PWM_TIMER;
  tim_cfg.clk_cfg = LEDC_AUTO_CLK;
  ledc_timer_config(&tim_cfg);

  ledc_channel_config_t chan_cfg = {};
  chan_cfg.channel = PWM_CHANNEL;
  chan_cfg.duty = PWM_DUTY;
  chan_cfg.gpio_num = OUT_PIN;
  chan_cfg.timer_sel = PWM_TIMER;
  ledc_channel_config(&chan_cfg);

  xTaskCreate(selftest_task, TAG, 4 * 1024, nullptr, 1, nullptr);

  return ESP_OK;
}

}  // namespace selftest_gen
