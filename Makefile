OBJDIR = build
BINDIR = bin

TARGET = $(BINDIR)/dc-client
DEBUG_TARGET = $(BINDIR)/dc-client-debug

all: $(TARGET) 

$(TARGET): $(shell find src -name "*.cpp") $(shell find src -name "*.hpp") $(shell find src -name "*.proto")
	mkdir -p $(OBJDIR)
	mkdir -p $(BINDIR)
	$(MAKE) -C src

.PHONY: clean debug
debug: $(DEBUG_TARGET)

$(DEBUG_TARGET): $(shell find src -name "*.cpp") $(shell find src -name "*.hpp") $(shell find src -name "*.proto")
	mkdir -p $(OBJDIR)
	mkdir -p $(BINDIR)
	$(MAKE) -C src debug

clean:
	$(MAKE) -C src clean
	rm -rf  $(BINDIR) $(OBJDIR)