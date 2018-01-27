all: server.out

server.out: server.cpp
	g++ server.cpp -lpthread -o server.out

clean:
	rm server.out

.PHONY:	all clean
