

all:
	gcc server.c userpass_map.c -o http_server -lssl -lcrypto
	chmod +x test_http_server.sh

test:
	./test_http_server.sh
