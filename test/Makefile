INCLUDES=
CC=g++
CFLAGS= -Wall -I$(INCLUDES) -g -std=c++17 -fpermissive

BASE_SRCS = base.cpp 
BASE_LIBS = 
BASE_OBJS = $(BASE_SRCS:.cpp=.o)

CRYPTO_SRCS = cryptotest.cpp ../src/util/crypto.cpp
CRYPTO_LIBS = -lssl -lcrypto -lpthread
CRYPTO_OBJS = $(CRYPTO_SRCS:.cpp=.o)

all: test.out cryptotest.out
	@echo "tests have been compiled"

test.out: $(BASE_OBJS)
	$(CC) $(CFLAGS) $(BASE_OBJS)  -o $@  $(LFLAGS) $(BASE_LIBS) 
cryptotest.out: $(CRYPTO_OBJS)
	$(CC) $(CFLAGS) $(CRYPTO_OBJS) -o $@ $(LFLAGS) $(CRYPTO_LIBS)
.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: clean test
test: all
	@echo "Begin test..."
	./test.out ./dcfs
	./cryptotest.out
	#fusermount3 -u ./dcfs

clean:
	rm -f *.o test
	rm -f /tmp/dcfs.log
	rm -rf /tmp/dcfs/

depend: $(SRCS)
	makedepend $(INCLUDES) $^
