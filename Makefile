CXXFLAGS =	-O2 -g -Wall -fmessage-length=0

OBJS2 =		player.o ReadAVI.o

LIBS = -lSDL2 -lpng16

all:	test MPNG_Player

test:	test.o ReadAVI.o
	$(CXX) -o test test.o ReadAVI.o
	mkdir -p out

MPNG_Player:	player.o ReadAVI.o
	$(CXX) -o MPNG_Player player.o ReadAVI.o $(LIBS)

clean:
	rm -f test.o ReadAVI.o player.o test MPNG_Player
