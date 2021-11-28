all:
	g++ -std=c++17 ./client.cpp -o client.exe -Wall -Wextra
	g++ -std=c++17 ./server.cpp -o server.exe -lpthread -Wall -Wextra
