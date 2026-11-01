# Disaster Rescue Path Planner — C++ backend
#
# Requires: g++ (C++17), libcurl-dev, nlohmann-json3-dev
#   Ubuntu/Debian: sudo apt-get install libcurl4-openssl-dev nlohmann-json3-dev
#   macOS (Homebrew): brew install curl nlohmann-json
#
# Build everything:  make
# Run the server:    ./server
# Run the client:    ./client   (in a second terminal, with server running)
# Clean:             make clean

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
