CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Isrc
LIBS = -lcurl -lpthread

all: server client

server: src/server.cpp
	$(CXX) $(CXXFLAGS) src/server.cpp $(LIBS) -o server

client: src/client.cpp
	$(CXX) $(CXXFLAGS) src/client.cpp -lcurl -o client

clean:
	rm -f server client
	rm -rf data

.PHONY: all clean
