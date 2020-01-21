#pragma once

#include <esp_err.h>

namespace sensor {

esp_err_t init();
size_t sensors_count();
size_t rotations_count(size_t sensor);

}  // namespace sensor
