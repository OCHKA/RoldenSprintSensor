#pragma once

#include <coap.h>

void get_req_handler(coap_context_t *ctx, coap_resource_t *resource,
                     coap_session_t *session, coap_pdu_t *request,
                     coap_binary_t *token, coap_string_t *query,
                     coap_pdu_t *response);