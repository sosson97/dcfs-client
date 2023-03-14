INCLUDES= 
CC=g++
CFLAGS= -Wall -I$(INCLUDES) -g -std=c++17 -fpermissive

SRCS = base.cpp
LIBS =
OBJS = $(SRCS:.cpp=.o)


all: test
	@echo "test has been compiled"

test: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS)  -o test  $(LFLAGS) $(LIBS) 
.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@



.PHONY: clean

clean:
	rm -f *.o test

depend: $(SRCS)
	makedepend $(INCLUDES) $^