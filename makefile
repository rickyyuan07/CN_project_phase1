all: client.cpp server.cpp
	g++ -std=c++17 ./client.cpp -o client -Wall -Wextra
	g++ -std=c++17 ./server.cpp -o server -lpthread -Wall -Wextra
client: client.cpp
	g++ -std=c++17 ./client.cpp -o client -Wall -Wextra
server: server.cpp
	g++ -std=c++17 ./server.cpp -o server -lpthread -Wall -Wextra
clean:
	rm -f client server
