all: player ringmaster

player: player.cpp
	g++ -std=c++11 -o player player.cpp

ringmaster: ringmaster.cpp
	g++ -std=c++11 -o ringmaster ringmaster.cpp
