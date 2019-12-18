idf_component_register(
	SRCS "src/coap_server.cpp" "src/handler.cpp"
	INCLUDE_DIRS "include"
	PRIV_INCLUDE_DIRS "private_include"
	REQUIRES coap
)
