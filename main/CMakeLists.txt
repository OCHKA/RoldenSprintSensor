set(COMPONENT_REQUIRES)
set(COMPONENT_PRIV_REQUIRES nvs_flash coap_server edge_period_sensor)

set(COMPONENT_SRCS main.cpp)
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()
