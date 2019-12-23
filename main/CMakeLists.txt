idf_component_register(
	SRCS main.cpp network.c
	INCLUDE_DIRS "."
	PRIV_REQUIRES wpa_supplicant nvs_flash esp_netif coap_server edge_period_sensor
)
target_compile_options(${COMPONENT_LIB} PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>)
