#include "handler.hpp"

static void coap_send_text(coap_resource_t* resource,
                           coap_session_t* session,
                           coap_pdu_t* request,
                           coap_binary_t* token,
                           coap_pdu_t* response,
                           const char* msg) {
  coap_add_data_blocked_response(resource, session, request, response, token,
                                 COAP_MEDIATYPE_TEXT_PLAIN, 0, strlen(msg),
                                 (const u_char*)msg);
}

void get_req_handler(coap_context_t* ctx,
                     coap_resource_t* resource,
                     coap_session_t* session,
                     coap_pdu_t* request,
                     coap_binary_t* token,
                     coap_string_t* query,
                     coap_pdu_t* response) {
  static char buffer[1024];

  if (query) {
    size_t length = 0;

    if (strcmp((char*)query->s, "0") == 0) {
      length = snprintf(buffer, sizeof(buffer), "hello");
    }
    if (strcmp((char*)query->s, "1") == 0) {
      length = snprintf(buffer, sizeof(buffer), "hello");
    }

    if (length > 0) {
      coap_add_data_blocked_response(resource, session, request, response,
                                     token,
                                     COAP_MEDIATYPE_APPLICATION_OCTET_STREAM, 0,
                                     length, (const u_char*)buffer);
    } else {
      coap_send_text(resource, session, request, token, response,
                     "failed to assemble response.");
    }
    return;
  }

  coap_send_text(resource, session, request, token, response,
                 "choose sensor with 0 or 1 query param.");
}
