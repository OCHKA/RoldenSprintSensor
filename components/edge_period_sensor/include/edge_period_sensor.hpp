#pragma once

#include <esp_err.h>

#include <optional>

namespace edge_period_sensor {

using edge_timestamp_t = uint64_t;

esp_err_t init();

struct EdgeEvent {
  size_t sensor;
  edge_timestamp_t timestamp;
};

std::optional<EdgeEvent> get_event();

}  // namespace edge_period_sensor
