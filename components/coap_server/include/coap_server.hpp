#pragma once

#include <esp_err.h>

#include <string>
#include <optional>

namespace coap_server {

esp_err_t init();

/**
 * @param query Request params
 * @return Response data or nothing in case of invalid query
 */
using get_req_cb_t = std::optional<uint32_t> (*)(std::string_view query);
/**
 * Registers callback which will be called on GET request with RESOURCE_NAME
 * @param cb Callback function
 */
void get_req_cb_register(get_req_cb_t cb);

}  // namespace coap_server
