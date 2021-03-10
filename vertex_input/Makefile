include ../LibConfig.mk

DEBUG =

HEADER_INCLUDE_DIR = ../
SRC = $(wildcard *.cpp)
BINS = $(patsubst %.cpp, %.out, ${SRC})

all:${BINS}

%.out:%.cpp
	$(CXX) $< -o $@ ${DEBUG} -I${HEADER_INCLUDE_DIR} ${LIB_INCLUDE_DIRS} ${LIB_LIBDIR} ${SDL_DEPS} -std=c++17

vertex_input.out:vertex_input.cpp shader/vert.spv shader/frag.spv

shader/vert.spv:shader/shader.vert
	$(GLSLC) $^ -o $@

shader/frag.spv:shader/shader.frag
	$(GLSLC) $^ -o $@


.PHONY:clean
clean:
	-rm *.out
