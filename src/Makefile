INCLUDES=-I/home/azureuser/fuse/libfuse-fuse-3.14.0/include -I/home/azureuser/fuse/libfuse-fuse-3.14.0/build 
CC=g++
CFLAGS= -Wall -I$(INCLUDES) -g -std=c++17 -fpermissive
FUSEFLAGS=`pkg-config fuse3 --cflags --libs`

SRCS = backend.cpp dcfs.cpp dir.cpp inode.cpp
LIBS =
OBJS = $(SRCS:.cpp=.o)


all: dcfs-client
	@echo "dcfs-client has been compiled"

dcfs-client: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(FUSEFLAGS)  -o dcfs-client  $(LFLAGS) $(LIBS) 
.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) $(FUSEFLAGS) -c $< -o $@



.PHONY: clean

clean:
	rm -f *.o dcfs-client

depend: $(SRCS)
	makedepend $(INCLUDES) $^
