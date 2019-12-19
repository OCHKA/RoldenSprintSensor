idf_component_register(
	SRCS "src/edge_period_sensor.cpp"
	INCLUDE_DIRS "include"
	PRIV_INCLUDE_DIRS "private_include"
	REQUIRES coap
)
target_compile_options(${COMPONENT_LIB} PUBLIC -std=c++17)
