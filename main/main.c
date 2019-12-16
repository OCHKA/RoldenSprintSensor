#include <stdio.h>
#include <sys/unistd.h>

#include <driver/gpio.h>
#include <driver/timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

const gpio_num_t GPIO_SENSOR_1 = GPIO_NUM_15;

static uint64_t ticks_at_edge = 0;

static void IRAM_ATTR sensor_isr(void *arg) {
  uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);
  uint32_t gpio_intr_status_h = READ_PERI_REG(GPIO_STATUS1_REG);
  SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);
  SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_h);

  if (gpio_intr_status & BIT(GPIO_SENSOR_1)) {
     timer_get_counter_value(0, TIMER_0, &ticks_at_edge);
  }
}

void app_main(void) {
  gpio_config_t io_cfg;
  io_cfg.intr_type = GPIO_INTR_POSEDGE;
  io_cfg.mode = GPIO_MODE_INPUT;
  io_cfg.pin_bit_mask = 1ULL << GPIO_SENSOR_1;
  io_cfg.pull_down_en = 1;
  gpio_config(&io_cfg);
  gpio_isr_register(sensor_isr, NULL, ESP_INTR_FLAG_IRAM, NULL);

  timer_config_t tim_cfg;
  tim_cfg.divider = 2;
  tim_cfg.counter_dir = TIMER_COUNT_UP;
  tim_cfg.counter_en = TIMER_START;
  tim_cfg.alarm_en = TIMER_ALARM_DIS;
  tim_cfg.auto_reload = TIMER_AUTORELOAD_EN;
  timer_init(0, TIMER_0, &tim_cfg);

  while (true) {
    printf("GPIO: %llu\n", ticks_at_edge);
    sleep(1);
  }
}