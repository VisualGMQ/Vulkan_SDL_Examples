# SDL Vulkan Example

This repo has some example by [Vulkan Tutorial](https://vulkan-tutorial.com/), and use SDL2 instead of GLFW.  

Make sure your compiler support C++17.  

These examples are compiled on MacOSX, and should compile on other platforms successfully.  

**NOTIC: on MacOSX, when using `validation layer`, you must add extra extensions(see `hello_world/02_validation_layer.cpp`)**  

To run examples, please `cd` into dir and `make`.  

# find Vulkan and SDL

If compile failed, please modify `LivConfig.mk` to ensure `make` find all third libs.

# menu

* [hello\_world](./hello_world): about how to draw a triangle on screen
* [vertex\_input](./vertex_input): about how to transform vertex information to GPU, and index buffer
