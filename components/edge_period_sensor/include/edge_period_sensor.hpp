#pragma once

#include <esp_err.h>

#include <optional>

namespace edge_period_sensor {

using period_t = uint32_t;

esp_err_t init();
size_t sensors_count();

/**
 * Loads last measured period of sensor from internal buffer
 * @param sensor Sensor index. Should be incremented until no value returned
 * @return period between last edge at current one or nothing
 */
std::optional<period_t> get_period(size_t sensor);

}  // namespace edge_period_sensor
