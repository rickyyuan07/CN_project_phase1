all: client.cpp server.cpp
	g++ -pthread -o server server.cpp -Wall -Wextra -std=gnu++17
	g++ -o client client.cpp -Wall -Wextra -std=gnu++17
client: client.cpp
	g++ -o client client.cpp -Wall -Wextra -std=gnu++17
server: server.cpp
	g++ -pthread -o server server.cpp -Wall -Wextra -std=gnu++17

clean:
	rm -f client server
