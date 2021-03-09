/*
 * this file include how to create vulkan instance
 */
#include <string>
#include <vector>
#include <iostream>

// include vulkan
#include "vulkan/vulkan.hpp"
#include "SDL.h"
// must explicit include SDL_vulkan.h, this file don't be included by SDL.h
#include "SDL_vulkan.h"

#include "log.hpp"

using std::cout;
using std::endl;
using std::vector;

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

    // vulkan code
    VkInstance instance_;

    void initVulkan() {
        createInstance();
    }

    void createInstance() {
        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pEngineName = "Vulkan Example";
        app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        app_info.engineVersion = VK_MAKE_VERSION(2, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;
        app_info.pApplicationName = "SDL";
        app_info.pNext = nullptr;

        // get SDL extensions
        uint32_t extension_count;
        SDL_Vulkan_GetInstanceExtensions(window_, &extension_count, nullptr);
        assertm("can't get extension from vulkan", extension_count != 0);
        vector<const char*> extensions(extension_count);
        SDL_Vulkan_GetInstanceExtensions(window_, &extension_count, extensions.data());

        cout << "SDL provide extensions:" << endl;
        for (const char* extension: extensions) {
            cout<< "\t" << extension << endl;
        }

        VkInstanceCreateInfo instance_create_info = {};
        instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_create_info.enabledLayerCount = 0;
        instance_create_info.ppEnabledLayerNames = nullptr;
        instance_create_info.enabledExtensionCount = extensions.size();
        instance_create_info.ppEnabledExtensionNames = extensions.data();
        instance_create_info.pApplicationInfo = &app_info;
        instance_create_info.flags = 0;
        instance_create_info.pNext = nullptr;

        VkResult result = vkCreateInstance(&instance_create_info, nullptr, &instance_);
        assertm("instance create failed",
                result == VK_SUCCESS);
 
        printAllSupportExtension();
    }

    void printAllSupportExtension() {
        uint32_t count;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
        vector<VkExtensionProperties> properties(count);
        vkEnumerateInstanceExtensionProperties(nullptr, &count, properties.data());
        cout << "all supported extensions:" << endl;
        for (auto& property: properties) {
            cout << "\t" << property.extensionName << endl;
        }
    }

    void quitVulkan() {
        vkDestroyInstance(instance_, nullptr);
    }
};

int main(int argc, char** argv) {
    App app;
    app.SetTitle("01 instance");
    app.Run();
    return 0;
}
