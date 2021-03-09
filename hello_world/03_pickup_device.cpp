/*
 * this file include how to pick up a physical device
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

// use macro to enable validation
#define ENABLE_VALIDATION

#ifdef ENABLE_VALIDATION
constexpr bool EnableValidation = true;
#else
constexpr bool EnableValidation = false;
#endif

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
    VkPhysicalDevice physical_device_;

    void initVulkan() {
        createInstance();
        Log("created instance");
        pickupPhysicalDevice();
        Log("pick up physical device");
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
        instance_create_info.enabledExtensionCount = extensions.size();
        instance_create_info.ppEnabledExtensionNames = extensions.data();
        instance_create_info.pApplicationInfo = &app_info;
        instance_create_info.flags = 0;
        instance_create_info.pNext = nullptr;

        // add validation layers
        vector<const char*> validation_names = {"VK_LAYER_KHRONOS_validation"};
        if (EnableValidation && checkValidationLayersSupport(validation_names)) {
            instance_create_info.enabledLayerCount = validation_names.size();
            instance_create_info.ppEnabledLayerNames = validation_names.data();
        } else {
            Log("validation not support");
            instance_create_info.enabledLayerCount = 0;
            instance_create_info.ppEnabledLayerNames = nullptr;
        }

        VkResult result = vkCreateInstance(&instance_create_info, nullptr, &instance_);
        assertm("instance create failed",
                result == VK_SUCCESS);
 
        printAllSupportExtension();
        printAllSupportValidationLayer();
    }

    bool checkValidationLayersSupport(const vector<const char*>& layers) {
        uint32_t count;
        vkEnumerateInstanceLayerProperties(&count, nullptr);
        vector<VkLayerProperties> properties(count);
        vkEnumerateInstanceLayerProperties(&count, properties.data());

        for (const char* layer_name: layers) {
            bool support = false;
            for (auto& property: properties) {
                if (strcmp(layer_name, property.layerName) == 0) {
                    support = true;
                    break; 
                }
            }
            if (!support) {
                return false;
            }
        }
        return true;
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

    void printAllSupportValidationLayer() {
        uint32_t count;
        vkEnumerateInstanceLayerProperties(&count, nullptr);
        vector<VkLayerProperties> properties(count);
        vkEnumerateInstanceLayerProperties(&count, properties.data());

        cout << "all supported validation layers:" << endl;
        for (auto& property: properties) {
            cout << "\t" << property.layerName << endl;
        }
    }

    void pickupPhysicalDevice() {
        uint32_t count;
        vkEnumeratePhysicalDevices(instance_, &count, nullptr);
        assertm("you don't have any GPU support Vulkan", count != 0);
        vector<VkPhysicalDevice> physical_devices(count);
        vkEnumeratePhysicalDevices(instance_, &count, physical_devices.data());
        physical_device_ = physical_devices.at(0);  // I assume you only have one GPU, so pick up this GPU

        printPhysicalDeviceInfo(physical_device_);
    }

    void printPhysicalDeviceInfo(VkPhysicalDevice& device) {
        VkPhysicalDeviceProperties property;
        vkGetPhysicalDeviceProperties(physical_device_, &property);
        cout << "physic device property:" << endl;
        cout << "\tname: " << property.deviceName << endl;
        cout << "\tintergrated?: " << (property.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU?"YES":"NO") << endl;
        printf("\tapi version: %d.%d.%d\n",
                VK_VERSION_MAJOR(property.apiVersion),
                VK_VERSION_MINOR(property.apiVersion),
                VK_VERSION_PATCH(property.apiVersion)
                );
        printf("\tdriver version: %d.%d.%d\n",
                VK_VERSION_MAJOR(property.driverVersion),
                VK_VERSION_MINOR(property.driverVersion),
                VK_VERSION_PATCH(property.driverVersion)
                );
    }

    void quitVulkan() {
        vkDestroyInstance(instance_, nullptr);
    }
};

int main(int argc, char** argv) {
    App app;
    app.SetTitle("03 pick up physicall device");
    app.Run();
    return 0;
}
