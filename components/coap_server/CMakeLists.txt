idf_component_register(
	SRCS "src/coap_server.cpp"
	INCLUDE_DIRS "include"
	REQUIRES coap edge_period_sensor
)

target_compile_options(${COMPONENT_LIB} PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>)
