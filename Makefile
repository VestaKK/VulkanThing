CC        := gcc
SHDC	  := $(VULKAN_SDK)/Bin/glslc.exe	
CCFLAGS   := -g -Wall
INCFLAGS  := -Ilib -I$(VULKAN_SDK)/Include -Isrc
LINKFLAGS += -Llib/GLFW/lib-mingw-w64 -static -lglfw3 -lgdi32
LINKFLAGS += -L$(VULKAN_SDK)/Lib -lvulkan-1

SHD_DIR := res/shaders

BIN       := bin/main.exe
SPV 	  := $(SHD_DIR)/vert.spv $(SHD_DIR)/frag.spv

.PHONY: clean all run shaders

all: dirs $(BIN) $(SPV)

dirs:
	-@mkdir -p bin

shaders: $(SPV)

run:
	./$(BIN)

$(BIN): src/main.c src/defines.h
	$(CC) src/main.c $(CCFLAGS) -o $@ $(INCFLAGS) $(LINKFLAGS)

$(SPV): $(SHD_DIR)/shader.vert $(SHD_DIR)/shader.frag
	$(SHDC) $(SHD_DIR)/shader.vert -o $(SHD_DIR)/vert.spv
	$(SHDC) $(SHD_DIR)/shader.frag -o $(SHD_DIR)/frag.spv

clean:
	@$(RM) -rv $(wildcard bin/*.exe) $(wildcard bin/*.dll)