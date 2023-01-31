default:
	g++ -std=c++17 main.cc utils.cc -o main

debug:
	g++ -std=c++17 main.cc utils.cc -g -O0 -o main

server:
	g++ -std=c++17 -c server.cc utils.cc

client:
	g++ -std=c++17 -c client.cc utils.cc
