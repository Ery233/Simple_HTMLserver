all: server
LIBS = -lpthread

server: server.c
	g++ -g -W -Wall $(LIBS) -o $@ $<

clean:
	rm server