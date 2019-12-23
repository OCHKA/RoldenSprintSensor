#pragma once

#include <esp_err.h>

#include <optional>

namespace edge_period_sensor {

using period_t = uint32_t;

esp_err_t init();
size_t sensors_count();
std::optional<period_t> get_avg_period(size_t sensor, time_t timeout_ms);

}  // namespace edge_period_sensor
