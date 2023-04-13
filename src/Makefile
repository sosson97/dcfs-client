TARGET=dcfs-client
DEBUG_TARGET=dcfs-client-debug
#
INCLUDES=-I./ -I/home/azureuser/fuse/libfuse-fuse-3.14.0/include -I/home/azureuser/fuse/libfuse-fuse-3.14.0/build 
CC=g++
CFLAGS=-Wall -I$(INCLUDES) -std=c++17 -fpermissive -O2
PROTOC=protoc

#
SRCDIR = src
SUBDIR = fs dc-client util
OUTDIR = ../bin
OBJDIR = ../build

#
FUSEFLAGS=`pkg-config fuse3 --cflags`
ZMQFLAGS=`pkg-config libzmq --cflags`
OPENSSLFLAGS=`pkg-config openssl --cflags`
PROTOFLAGS=`pkg-config protobuf --cflags`

FUSELIBS=`pkg-config fuse3 --libs`
ZMQLIBS=`pkg-config libzmq --libs`
PROTOLIBS=`pkg-config protobuf --libs`
OPENSSLLIBS=`pkg-config openssl --libs`
LIBS = $(FUSELIBS) $(ZMQLIBS) $(PROTOLIBS) $(OPENSSLLIBS)

INCS = $(wildcard *.h $(foreach fd, $(SUBDIR), $(fd)/*.hpp))
SRCS = $(wildcard *.c $(foreach fd, $(SUBDIR), $(fd)/*.cpp))
FS_SRCS = $(wildcard fs/*.cpp)
DC_CLIENT_SRCS = $(wildcard dc-client/*.cpp)
PROTO_SRCS = $(wildcard dc-client/proto/*.proto)
OBJS = $(addprefix $(OBJDIR)/, $(SRCS:.cpp=.o))
PROTO_OBJS = $(addprefix $(OBJDIR)/, $(PROTO_SRCS:.proto=.pb.o))

.PHONY: clean

all: $(TARGET)	
	@echo "$(TARGET) has been compiled"

debug: CFLAGS += -O0 -DDEBUG -g
debug: $(DEBUG_TARGET)
	@echo "$(DEBUG_TARGET) has been compiled for debug mode"

#Linking
$(TARGET): $(PROTO_OBJS) $(OBJS) 
	mkdir -p $(OUTDIR)
	$(CC) $(CFLAGS) $(PROTO_OBJS) $(OBJS) -o $(OUTDIR)/$@ $(LFLAGS) $(LIBS) 

$(DEBUG_TARGET): $(PROTO_OBJS) $(OBJS) 
	mkdir -p $(OUTDIR)
	$(CC) $(CFLAGS) $(PROTO_OBJS) $(OBJS) -o $(OUTDIR)/$@ $(LFLAGS) $(LIBS) 


#Proto files
$(OBJDIR)/dc-client/proto/capsule.pb.o: dc-client/capsule.pb.cc
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) $(PROTOLIBS) -c $< -o $@

$(OBJDIR)/dc-client/proto/request.pb.o: dc-client/request.pb.cc dc-client/capsule.pb.cc
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) $(PROTOLIBS) -c $< -o $@

dc-client/%.pb.cc: dc-client/proto/%.proto
	mkdir -p $(dir $@)
	$(PROTOC) -I=dc-client/proto --cpp_out=dc-client $<

#Source files
$(OBJDIR)/fs/%.o: fs/%.cpp $(INCS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) $(FUSEFLAGS) -c $< -o $@

$(OBJDIR)/dc-client/%.o: dc-client/%.cpp $(INCS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) $(ZMQFLAGS) -c $< -o $@

$(OBJDIR)/util/%.o: util/%.cpp $(INCS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) $(OPENSSLFLAGS) -c $< -o $@

#$(OBJDIR)/%.o: %.cpp $(INCS)
#	mkdir -p $(dir $@)
#	$(CC) $(CFLAGS) $(INCLUDES) $(FUSEFLAGS) -c $< -o $@

clean:
	rm dc-client/*.pb.*

depend: $(SRCS)
	makedepend $(INCLUDES) $^
