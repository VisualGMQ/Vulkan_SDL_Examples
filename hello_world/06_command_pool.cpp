/*
 * this file include how to create command pool
 */
#include <string>
#include <vector>
#include <iostream>
#include <optional>
#include <array>
#include <set>

// include vulkan
#include "vulkan/vulkan.hpp"
#include "SDL.h"
// must explicit include SDL_vulkan.h, this file don't be included by SDL.h
#include "SDL_vulkan.h"

#include "log.hpp"

using std::cout;
using std::endl;
using std::vector;
using std::optional;
using std::array;
using std::set;

constexpr int WindowWidth = 1024;
constexpr int WindowHeight = 720;

// use macro to enable validation
#define ENABLE_VALIDATION

#ifdef ENABLE_VALIDATION
constexpr bool EnableValidation = true;
#else
constexpr bool EnableValidation = false;
#endif

struct QueueFamilyIdx {
    optional<uint32_t> present_queue_idx;
    optional<uint32_t> graphic_queue_idx;

    bool Valid() {
        return present_queue_idx.has_value() && graphic_queue_idx.has_value();
    }
};

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
    VkSurfaceKHR surface_;
    VkDevice device_;
    VkQueue graphic_queue_;
    VkQueue present_queue_;
    VkCommandPool commandpool_;
    vector<VkCommandBuffer> commands_;

    void initVulkan() {
        createInstance();
        Log("created instance");
        pickupPhysicalDevice();
        Log("pick up physical device");
        createSurface();
        Log("create surface");
        createLogicDevice();
        Log("create logic device");
        createCommandPool();
        Log("create command pool");
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

        // On MacOS, the validation layer rely on this extension, so we add it here.
        // NOTIC: if you don't have this extension, validation layer will not show error untill you create logic device.
        extensions.push_back("VK_KHR_get_physical_device_properties2");

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

    void createSurface() {
        bool result = SDL_Vulkan_CreateSurface(window_, instance_, &surface_);
        assertm("create surface failed", result == true);
    }

    void createLogicDevice() {
        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.pEnabledFeatures = 0;
        create_info.ppEnabledLayerNames = nullptr;

        // On MacOS, the validation layer rely on this device extension, so we must add it.
        vector<const char*> extensions;
        if (EnableValidation) {
            extensions.push_back("VK_KHR_portability_subset");
            create_info.enabledExtensionCount = extensions.size();
            create_info.ppEnabledExtensionNames = extensions.data();
        }

        auto family_idx = getQueueFamilyIdx();
        assertm("can't find appropriate queue familise", family_idx.Valid());

        float priority = 1.0f;

        // we find graphic queue idx and present queue idx, but they are the same index, so we can only create one queue.
        // if your graphic queue idx and present queue idx are not same, please create queue for each idx.
        VkDeviceQueueCreateInfo queue_create_info = {};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = family_idx.graphic_queue_idx.value();
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &priority;

        create_info.queueCreateInfoCount = 1;
        create_info.pQueueCreateInfos = &queue_create_info;

        assertm("can't create logic device", vkCreateDevice(physical_device_, &create_info, nullptr, &device_) == VK_SUCCESS);
        vkGetDeviceQueue(device_, family_idx.graphic_queue_idx.value(), 0, &graphic_queue_);
        vkGetDeviceQueue(device_, family_idx.present_queue_idx.value(), 0, &present_queue_);
    }

    QueueFamilyIdx getQueueFamilyIdx() {
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &count, nullptr);
        vector<VkQueueFamilyProperties> properties(count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &count, properties.data());

        QueueFamilyIdx family_idx;
        for (int i = 0; i < properties.size(); i++) {
            if (properties.at(i).queueFlags&VK_QUEUE_GRAPHICS_BIT) {
                family_idx.graphic_queue_idx = i;
                VkBool32 is_present = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_, i, surface_, &is_present);
                if (is_present) {
                    family_idx.present_queue_idx = i;
                    break;
                }
            }
        }
        return family_idx;
    }

    void createCommandPool() {
        VkCommandPoolCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.queueFamilyIndex = getQueueFamilyIdx().graphic_queue_idx.value();
        assertm("create command pool failed", vkCreateCommandPool(device_, &create_info, nullptr, &commandpool_) == VK_SUCCESS);
    }

    void quitVulkan() {
        vkDestroyCommandPool(device_, commandpool_, nullptr);
        vkDestroyDevice(device_, nullptr);
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        vkDestroyInstance(instance_, nullptr);
    }
};

int main(int argc, char** argv) {
    App app;
    app.SetTitle("06 command pool");
    app.Run();
    return 0;
}
