#pragma once

#include <esp_err.h>

esp_err_t network_init();
esp_err_t coap_server_init();
void rpm_sensor_init();