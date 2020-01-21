idf_component_register(
	SRCS "src/sensor.cpp"
	INCLUDE_DIRS "include"
)

target_compile_options(${COMPONENT_LIB} PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>)
