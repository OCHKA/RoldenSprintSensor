#pragma once

#include <esp_err.h>

#include <utility>

namespace sensor {

struct Sample {
  size_t rotations;
  uint64_t edge_timestamp;
};

esp_err_t init();
size_t sensors_count();
Sample last_sample(size_t sensor);

}  // namespace sensor
