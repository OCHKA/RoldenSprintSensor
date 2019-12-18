#include <coap.h>

#include <esp_log.h>

#include "components.h"

static const char *TAG = "coap";

static char response_buffer[64];

static void get_req_handler(coap_context_t *ctx, coap_resource_t *resource,
                            coap_session_t *session, coap_pdu_t *request,
                            coap_binary_t *token, coap_string_t *query,
                            coap_pdu_t *response) {
  int length = snprintf(response_buffer, sizeof(response_buffer), "hello");

  coap_add_data_blocked_response(resource, session, request, response, token,
                                 COAP_MEDIATYPE_TEXT_PLAIN, 0, length,
                                 (const u_char *)response_buffer);
}

static void server_task(void *p) {
  /* Prepare the CoAP server socket */
  coap_address_t serv_addr;
  coap_address_init(&serv_addr);
  serv_addr.addr.sin.sin_family = AF_INET;
  serv_addr.addr.sin.sin_addr.s_addr = INADDR_ANY;
  serv_addr.addr.sin.sin_port = htons(COAP_DEFAULT_PORT);

  coap_context_t *ctx = coap_new_context(NULL);
  if (!ctx) {
    ESP_LOGE(TAG, "coap_new_context() failed");
    return;
  }

  coap_endpoint_t *ep = coap_new_endpoint(ctx, &serv_addr, COAP_PROTO_UDP);
  if (!ep) {
    ESP_LOGE(TAG, "udp: coap_new_endpoint() failed");
    return;
  }

  coap_resource_t *resource =
      coap_resource_init(coap_make_str_const(""), 0);
  if (!resource) {
    ESP_LOGE(TAG, "coap_resource_init() failed");
    return;
  }
  coap_register_handler(resource, COAP_REQUEST_GET, get_req_handler);
  /* We possibly want to Observe the GETs */
  coap_resource_set_get_observable(resource, 1);
  coap_add_resource(ctx, resource);

  unsigned wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;

  while (1) {
    int result = coap_run_once(ctx, wait_ms);
    if (result < 0) {
      ESP_LOGE(TAG, "coap run_once() failed");
      break;
    } else if (result && (unsigned)result < wait_ms) {
      /* decrement if there is a result wait time returned */
      wait_ms -= result;
    }
    if (result) {
      /* result must have been >= wait_ms, so reset wait_ms */
      wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;
    }
  }
}

esp_err_t coap_server_init() {
  xTaskCreate(server_task, TAG, 8 * 1024, NULL, 5, NULL);

  return ESP_OK;
}