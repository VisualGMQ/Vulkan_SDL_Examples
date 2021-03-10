VULKAN_INCLUDE_DIR = /usr/local/include
VULKAN_LIBDIR = /usr/local/lib
VULKAN_LIBRARY = vulkan

SDL_INCLUDE_DIR = `sdl2-config --cflags`
SDL_LIBDIR = `sdl2-config --libs`

GLM_INCLUDE_DIR = `pkg-config --cflags glm`

LIB_INCLUDE_DIRS = -I${VULKAN_LIBDIR} ${SDL_INCLUDE_DIR} ${GLM_INCLUDE_DIR}
LIB_LIBDIR = -L${VULKAN_LIBDIR} -l${VULKAN_LIBRARY} ${SDL_LIBDIR}

GLSLC = glslc
