/*
 * basic SDL2 vulkan structure
 *
 * This file don't contain any vulkan struct and functions.This is only a basic pure SDL framework.
 * We will add vulkan code on this framework.
 */
#include <string>

#include "SDL.h"

#include "log.hpp"

constexpr int WindowWidth = 1024;
constexpr int WindowHeight = 720;

class App {
 public:
    App():should_close_(false) {
        initSDL();
        initVulkan();
    }

    ~App() {
        quitVulkan();
        quitSDL();
    }

    void SetTitle(std::string title) {
        SDL_SetWindowTitle(window_, title.c_str());
    }

    void Exit() {
        should_close_ = true;
    }

    bool ShouldClose() {
        return should_close_;
    }

    void Run() {
        while (!ShouldClose()) {
            pollEvent();
            SDL_Delay(60);
        }
    }

 private:
    SDL_Window* window_;
    SDL_Event event;
    bool should_close_;

    void initSDL() {
        SDL_Init(SDL_INIT_EVERYTHING);
        window_ = SDL_CreateWindow(
                "",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                WindowWidth, WindowHeight,
                SDL_WINDOW_SHOWN|SDL_WINDOW_VULKAN
                );
        assertm("can't create window", window_ != nullptr);
    }

    void pollEvent() {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                Exit();
            }
        }
    }

    void quitSDL() {
        SDL_Quit();
    }

    void initVulkan() { // will fill in latter

    }

    void quitVulkan() { // will fill in latter

    }
};

int main(int argc, char** argv) {
    App app;
    app.SetTitle("00 basic structure");
    app.Run();
    return 0;
}
