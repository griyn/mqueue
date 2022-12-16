mqueue:
	g++ example/mqueue_example.cpp -g -lpthread -Isrc/
pistol:
	g++ example/pistol_example.cpp src/*.cpp -g -lpthread -Isrc/
