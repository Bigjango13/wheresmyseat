CFLAGS = -std=c++23 -Wall -Wextra -pedantic `pkg-config --cflags sdl2`
CPP = g++
LD_FLAGS = `pkg-config --libs sdl2` -lSDL2_image
LD = $(CPP)

CPP_IN_FILES = $(shell find src/ -type f -iname '*.cpp')
OBJS = $(CPP_IN_FILES:.cpp=.o)

all: build
build: $(OBJS)
	$(LD) $(OBJS) $(LD_FLAGS) -g -o wims
%.o: %.cpp
	$(CPP) $(CFLAGS) -c $< -g -o $@

clean:
	rm -f $(OBJS) wims
