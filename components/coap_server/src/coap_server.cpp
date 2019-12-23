#include "coap_server.hpp"

#include <coap/coap.h>
#include <esp_log.h>

namespace coap_server {

const char* TAG = "coap_srv";
const char* RESOURCE_NAME = "period";

get_req_cb_t get_req_callback = nullptr;

coap_str_const_t* coap_new_make_str_const(const char* string);

void send_text(coap_resource_t* resource,
               coap_session_t* session,
               coap_pdu_t* request,
               coap_binary_t* token,
               coap_pdu_t* response,
               const char* msg) {
  coap_add_data_blocked_response(resource, session, request, response, token,
                                 COAP_MEDIATYPE_TEXT_PLAIN, 0, strlen(msg),
                                 (const u_char*)msg);
}

void send_data(coap_resource_t* resource,
               coap_session_t* session,
               coap_pdu_t* request,
               coap_binary_t* token,
               coap_pdu_t* response,
               const uint8_t* buffer,
               size_t length) {
  coap_add_data_blocked_response(resource, session, request, response, token,
                                 COAP_MEDIATYPE_APPLICATION_OCTET_STREAM, 0,
                                 length, buffer);
}

void get_req_handler(coap_context_t* ctx,
                     coap_resource_t* resource,
                     coap_session_t* session,
                     coap_pdu_t* request,
                     coap_binary_t* token,
                     coap_string_t* query,
                     coap_pdu_t* response) {
  auto cb_result = get_req_callback({(char*)query->s, query->length});

  if (cb_result.has_value()) {
    auto& vector = cb_result.value();
    send_data(resource, session, request, token, response, vector.data(),
              vector.size());
  } else {
    send_text(resource, session, request, token, response, "invalid query");
  }
}

void server_task(void* p) {
  coap_address_t serv_addr;
  coap_address_init(&serv_addr);
  serv_addr.addr.sin.sin_family = AF_INET;
  serv_addr.addr.sin.sin_addr.s_addr = INADDR_ANY;
  serv_addr.addr.sin.sin_port = htons(COAP_DEFAULT_PORT);

  coap_context_t* ctx = coap_new_context(nullptr);
  if (!ctx) {
    ESP_LOGE(TAG, "coap_new_context() failed");
    return;
  }

  coap_endpoint_t* ep = coap_new_endpoint(ctx, &serv_addr, COAP_PROTO_UDP);
  if (!ep) {
    ESP_LOGE(TAG, "udp: coap_new_endpoint() failed");
    return;
  }

  coap_resource_t* resource =
      coap_resource_init(coap_new_make_str_const(RESOURCE_NAME), 0);
  if (!resource) {
    ESP_LOGE(TAG, "coap_resource_init() failed");
    return;
  }
  coap_register_handler(resource, COAP_REQUEST_GET, get_req_handler);
  coap_resource_set_get_observable(resource, 1);
  coap_add_resource(ctx, resource);

  unsigned wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;

  while (true) {
    int result = coap_run_once(ctx, wait_ms);
    if (result < 0) {
      ESP_LOGE(TAG, "coap run_once() failed");
      break;
    } else if (result && (unsigned)result < wait_ms) {
      // decrement if there is a result wait time returned
      wait_ms -= result;
    }
    if (result) {
      // result must have been >= wait_ms, so reset wait_ms
      wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;
    }
  }
}

esp_err_t init() {
  xTaskCreatePinnedToCore(server_task, TAG, 8 * 1024, nullptr, 5, nullptr, 1);
  return ESP_OK;
}

void get_req_cb_register(get_req_cb_t cb) {
  get_req_callback = cb;
}

// TODO: remove on update of libcoap component
#define COAP_MAX_STR_CONST_FUNC 2
coap_str_const_t* coap_new_make_str_const(const char* string) {
  static int ofs = 0;
  static coap_str_const_t var[COAP_MAX_STR_CONST_FUNC];
  if (++ofs == COAP_MAX_STR_CONST_FUNC)
    ofs = 0;
  var[ofs].length = strlen(string);
  var[ofs].s = (const uint8_t*)string;
  return &var[ofs];
}

}  // namespace coap_server
