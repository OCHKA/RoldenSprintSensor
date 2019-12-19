#pragma once

#include <esp_err.h>

namespace coap_server {

esp_err_t init();
esp_err_t send_block(uint8_t* buffer, size_t size);

}  // namespace coap_server
