INCLUDES= 
CC=g++
CFLAGS= -Wall -I$(INCLUDES) -g -std=c++17 -fpermissive

SRCS = base.cpp
LIBS = 
OBJS = $(SRCS:.cpp=.o)


all: test.out
	@echo "test has been compiled"

test.out: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS)  -o $@  $(LFLAGS) $(LIBS) 
.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@



.PHONY: clean test
test: all
	@echo "Begin test..."
	./test.out ./dcfs
	fusermount3 -u ./dcfs

clean:
	rm -f *.o test

depend: $(SRCS)
	makedepend $(INCLUDES) $^
