idf_component_register(
	SRCS "src/selftest_gen.cpp"
	INCLUDE_DIRS "include"
	REQUIRES coap
)
target_compile_options(${COMPONENT_LIB} PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>)
