VULKAN_INCLUDE_DIR = /usr/local/include
VULKAN_LIBDIR = /usr/local/lib
VULKAN_LIBRARY = vulkan
SDL_DEPS = `sdl2-config --libs --cflags`
GLSLC = glslc
DEBUG =

SRC = $(wildcard *.cpp)
BINS = $(patsubst %.cpp, %.out, ${SRC})

all:${BINS}

%.out:%.cpp
	$(CXX) $< -o $@ ${DEBUG} -I../ -I${VULKAN_INCLUDE_DIR} -L${VULKAN_LIBDIR} -l${VULKAN_LIBRARY} ${SDL_DEPS} -std=c++17

09_shader.out:09_shader.cpp shader/vert.spv shader/frag.spv

shader/vert.spv:shader/shader.vert
	$(GLSLC) $^ -o $@

shader/frag.spv:shader/shader.frag
	$(GLSLC) $^ -o $@


.PHONY:clean
clean:
	-rm *.out
