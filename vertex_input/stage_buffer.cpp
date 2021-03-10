#include <string>
#include <vector>
#include <iostream>
#include <optional>
#include <array>
#include <set>
#include <streambuf>
#include <fstream>
#include <limits>

#include "vulkan/vulkan.hpp"
#include "SDL.h"
#include "SDL_vulkan.h"
#include "glm/glm.hpp"

#include "log.hpp"
#include "vulkan/vulkan_core.h"

using std::cout;
using std::endl;
using std::vector;
using std::optional;
using std::string;

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

string ReadShader(string filename) {
    std::ifstream file(filename, std::ios::binary);
    assertm((filename + " can't be open").c_str(), !file.fail());
    string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    return content;
}

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription GetBindingDescriptions() {
        VkVertexInputBindingDescription description = {};
        description.binding = 0;
        description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        description.stride = sizeof(Vertex);
        return description;
    }

    static std::array<VkVertexInputAttributeDescription, 2> GetAttribDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> descriptions;

        // for inPos attribute
        descriptions[0].binding = 0;
        descriptions[0].location = 0;
        descriptions[0].offset = offsetof(Vertex, pos);
        descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;

        // for inColor attribute
        descriptions[1].binding = 0;
        descriptions[1].location = 1;
        descriptions[1].offset = offsetof(Vertex, color);
        descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        
        return descriptions;
    }
};

const vector<Vertex> TriangleVertices = {
    {{0.0, -0.5}, {0, 0.5, 0}},
    {{0.5, 0.5}, {1, 0, 0}},
    {{-0.5, 0.5}, {0, 0, 0.3}}
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
            drawFrame();
            SDL_Delay(60);
        }
        vkDeviceWaitIdle(device_);
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
    VkSwapchainKHR swapchain_;
    vector<VkCommandBuffer> command_buffers_;
    vector<VkImage> images_;
    vector<VkImageView> imageviews_;
    VkPipeline pipeline_;
    VkPipelineLayout pipeline_layout_;
    VkRenderPass renderpass_;
    vector<VkFramebuffer> framebuffers_;
    VkSemaphore image_avaliable_semaphore_;
    VkSemaphore present_finish_semaphore_;
    VkBuffer vertex_buffer_;
    VkDeviceMemory vertex_buf_memory_;

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
        createSwapchain();
        Log("create swapchain");
        createImageViews();
        Log("create image views");
        createRenderPass();
        Log("render pass created");
        createGraphicPipeline();
        Log("create graphic pipeline");
        createFramebuffer();
        Log("create framebuffer");
        createVertexBuffer();
        Log("create vertex buffer");
        createCommandBuffer();
        Log("create command buffers");
        prepDraw();
        Log("prepared command buffer to draw");
        createSemaphores();
        Log("create semahpores ok");
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

        vector<const char*> extensions;
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        // On MacOS, the validation layer rely on this device extension, so we must add it.
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

    void createSwapchain() {
        VkSwapchainCreateInfoKHR create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

        create_info.surface = surface_;

        auto format = getSurfaceFormat();
        create_info.imageColorSpace = format.colorSpace;
        create_info.imageFormat = format.format;

        if (format.format == VK_FORMAT_B8G8R8A8_SRGB) {
            cout << "surface format: BGRA8888 SRGB" << endl;
        }
        if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            cout << "surface color space: SRGB" << endl;
        }

        auto capabilities = getSurfaceCapabilities();
        uint32_t image_count = 2;   // I want to use double-buffering, so I set image_count = 2
        if (image_count < capabilities.minImageCount || image_count > capabilities.maxImageCount) {
            image_count = capabilities.minImageCount;
        }
        cout << "image_count = " << image_count << endl;
        create_info.minImageCount = image_count;

        VkExtent2D extent = {WindowWidth, WindowHeight};
        if (extent.width <= capabilities.minImageExtent.width || extent.width >= capabilities.maxImageExtent.width) {
            extent.width = capabilities.maxImageExtent.width;
        }
        if (extent.height <= capabilities.minImageExtent.height || extent.height >= capabilities.maxImageExtent.height) {
            extent.height = capabilities.maxImageExtent.height;
        }
        create_info.imageExtent = extent;
        printf("extent = (%d, %d)\n", extent.width, extent.height);

        auto family_idx = getQueueFamilyIdx();
        uint32_t idices[] = {family_idx.graphic_queue_idx.value(), family_idx.present_queue_idx.value()};
        if (family_idx.graphic_queue_idx.value() != family_idx.present_queue_idx.value()) {
            create_info.pQueueFamilyIndices = idices;
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
        } else {
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices = nullptr;
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        create_info.imageArrayLayers = 1;   // currently we only draw a 2D triangle, so set it 1
        create_info.presentMode = getSurfacePresent();
        create_info.preTransform = capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.clipped = VK_TRUE;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        create_info.oldSwapchain = nullptr;
        create_info.pNext = nullptr;

        assertm("can't create swapchain", vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapchain_) == VK_SUCCESS);

        uint32_t count;
        vkGetSwapchainImagesKHR(device_, swapchain_, &count, nullptr);
        images_.resize(count);
        vkGetSwapchainImagesKHR(device_, swapchain_, &count, images_.data());

        printf("got %d images\n", count);
    }

    VkSurfaceFormatKHR getSurfaceFormat() {
        uint32_t count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &count, nullptr);
        vector<VkSurfaceFormatKHR> formats(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &count, formats.data());
        for (auto& format: formats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
        return formats.at(0);
    }

    VkPresentModeKHR getSurfacePresent() {
        uint32_t count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &count, nullptr);
        vector<VkPresentModeKHR> presents(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_, &count, presents.data());
        for (auto& present: presents) {
            if (present == VK_PRESENT_MODE_MAILBOX_KHR) {   // if avaliable, we choose mailbox mode
                return present;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;    // this present mode must be supported
    }

    VkSurfaceCapabilitiesKHR getSurfaceCapabilities() {
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &capabilities);
        return capabilities;
    }

    void createImageViews() {
        imageviews_.resize(images_.size());
        for (int i = 0; i < images_.size(); i++) {
            VkImageViewCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.image = images_.at(i);
            create_info.format = getSurfaceFormat().format;
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            create_info.subresourceRange.levelCount = 1;
            create_info.subresourceRange.layerCount = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.baseMipLevel = 0;
            assertm("can't create image view", vkCreateImageView(device_, &create_info, nullptr, &imageviews_.at(i)) == VK_SUCCESS);
        }
    }

    VkShaderModule createShaderModule(string filename) {
        VkShaderModuleCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        string content = ReadShader(filename);
        create_info.codeSize = content.size();
        create_info.pCode = (const uint32_t*)(content.data());

        VkShaderModule shader;
        assertm("can't create shader", vkCreateShaderModule(device_, &create_info, nullptr, &shader) == VK_SUCCESS);
        return shader;
    }

    void createGraphicPipeline() {
        VkGraphicsPipelineCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

        // vertex input state
        auto bind_description = Vertex::GetBindingDescriptions();
        auto attrib_description = Vertex::GetAttribDescriptions();

        VkPipelineVertexInputStateCreateInfo vertex_create_info = {};
        vertex_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrib_description.size());
        vertex_create_info.pVertexAttributeDescriptions = attrib_description.data();
        vertex_create_info.vertexBindingDescriptionCount = 1;
        vertex_create_info.pVertexBindingDescriptions = &bind_description;

        create_info.pVertexInputState = &vertex_create_info;

        // input assembly state
        VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
        assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assembly_create_info.primitiveRestartEnable = VK_FALSE;
        assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        create_info.pInputAssemblyState = &assembly_create_info;

        // viewport and scissors
        VkViewport viewport;
        viewport.x = 0;
        viewport.y = 0;
        int w, h;
        SDL_Vulkan_GetDrawableSize(window_, &w, &h);
        viewport.width = w;
        viewport.height = h;
        viewport.maxDepth = 1;
        viewport.minDepth = 0;

        VkRect2D rect;
        rect.offset = {0, 0};
        rect.extent.width = w;
        rect.extent.height = h;

        VkPipelineViewportStateCreateInfo viewport_create_info = {};
        viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_create_info.scissorCount = 1;
        viewport_create_info.pScissors = &rect;
        viewport_create_info.pViewports = &viewport;
        viewport_create_info.viewportCount = 1;

        create_info.pViewportState = &viewport_create_info;

        // shaders
        VkShaderModule vert_module = createShaderModule("shader/vert.spv"),
                       frag_module = createShaderModule("shader/frag.spv");

        VkPipelineShaderStageCreateInfo vert_create_info = {};
        vert_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_create_info.module = vert_module;
        vert_create_info.pName = "main";
        vert_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;

        VkPipelineShaderStageCreateInfo frag_create_info = {};
        frag_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_create_info.module = frag_module;
        frag_create_info.pName = "main";
        frag_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkPipelineShaderStageCreateInfo stage_create_infos[] = {
            vert_create_info,
            frag_create_info
        };

        create_info.pStages = stage_create_infos;
        create_info.stageCount = 2;

        // rasterization
        VkPipelineRasterizationStateCreateInfo raster_create_info = {};
        raster_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster_create_info.lineWidth = 1.0f;
        raster_create_info.depthClampEnable = VK_FALSE;
        raster_create_info.rasterizerDiscardEnable = VK_FALSE;
        raster_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
        raster_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
        raster_create_info.polygonMode = VK_POLYGON_MODE_FILL;

        create_info.pRasterizationState = &raster_create_info;

        // multisample
        VkPipelineMultisampleStateCreateInfo multisample_create_info = {};
        multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisample_create_info.sampleShadingEnable = VK_FALSE;
        
        create_info.pMultisampleState = &multisample_create_info;

        // depth and stencil
        create_info.pDepthStencilState = nullptr;

        // color blending
        VkPipelineColorBlendAttachmentState color_attachment = {};
        color_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
        color_attachment.blendEnable = VK_TRUE;
        color_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        color_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        color_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo color_create_info = {};
        color_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_create_info.attachmentCount = 1;
        color_create_info.pAttachments = &color_attachment;
        color_create_info.logicOpEnable = VK_FALSE;

        create_info.pColorBlendState = &color_create_info;

        // pipeline layout
        VkPipelineLayoutCreateInfo layout_create_info = {};
        layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        assertm("pipeline layout can't create", vkCreatePipelineLayout(device_, &layout_create_info, nullptr, &pipeline_layout_) == VK_SUCCESS);

        create_info.layout = pipeline_layout_;

        // render pass
        create_info.renderPass = renderpass_;

        // dynamic state
        create_info.pDynamicState = nullptr;

        // create pipeline
        assertm("pipeline can't create", vkCreateGraphicsPipelines(device_, nullptr, 1, &create_info, nullptr, &pipeline_) == VK_SUCCESS);

        // destroy shaders
        vkDestroyShaderModule(device_, vert_module, nullptr);
        vkDestroyShaderModule(device_, frag_module, nullptr);
    }

    void createRenderPass() {
        VkRenderPassCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        
        // attachment description
        VkAttachmentDescription description = {};
        description.format = getSurfaceFormat().format;
        description.samples = VK_SAMPLE_COUNT_1_BIT;
        description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // subpass
        VkAttachmentReference reference = {};
        reference.attachment = 0;
        reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_description = {};
        subpass_description.colorAttachmentCount = 1;
        subpass_description.pColorAttachments = &reference;
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.pInputAttachments = nullptr;

        // render pass
        create_info.subpassCount = 1;
        create_info.pSubpasses = &subpass_description;
        create_info.attachmentCount = 1;
        create_info.pAttachments = &description;

        // create a subpass
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;

        dependency.srcAccessMask = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        dependency.dstAccessMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

        create_info.dependencyCount = 1;
        create_info.pDependencies = &dependency;

        assertm("render pass can't create", vkCreateRenderPass(device_, &create_info, nullptr, &renderpass_) == VK_SUCCESS);
    }

    void createFramebuffer() {
        int w, h;
        SDL_Vulkan_GetDrawableSize(window_, &w, &h);
        framebuffers_.resize(images_.size());
        for (int i = 0; i < images_.size(); i++) {
            VkFramebufferCreateInfo create_info = {};
            create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            create_info.width = w;
            create_info.height = h;
            create_info.attachmentCount = 1;
            create_info.pAttachments = &imageviews_.at(i);
            create_info.renderPass = renderpass_;
            create_info.layers = 1;
            assertm("frame buffer can' create", vkCreateFramebuffer(device_, &create_info, nullptr, &framebuffers_.at(i)) == VK_SUCCESS);
        }
    }

    void createCommandBuffer() {
        command_buffers_.resize(framebuffers_.size());

        VkCommandBufferAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool = commandpool_;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = static_cast<uint32_t>(command_buffers_.size());

        assertm("command buffers create failed", vkAllocateCommandBuffers(device_, &allocate_info, command_buffers_.data()) == VK_SUCCESS);
    }

    void prepDraw() {
        for (int i = 0; i < command_buffers_.size(); i++) {
            VkCommandBuffer& buffer = command_buffers_.at(i);
            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            assertm("can't begin record command buffer", vkBeginCommandBuffer(buffer, &begin_info) == VK_SUCCESS);

            VkRenderPassBeginInfo renderpass_begin_info = {};
            renderpass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

            VkClearValue clear_value = {0, 0.5, 0, 1};
            renderpass_begin_info.renderPass = renderpass_;
            renderpass_begin_info.clearValueCount = 1;
            renderpass_begin_info.pClearValues = &clear_value;
            renderpass_begin_info.framebuffer = framebuffers_.at(i);
            renderpass_begin_info.renderArea.offset = {0, 0};
            int w, h;
            SDL_Vulkan_GetDrawableSize(window_, &w, &h);
            renderpass_begin_info.renderArea.extent.width = w;
            renderpass_begin_info.renderArea.extent.height = h;

            vkCmdBeginRenderPass(buffer, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

            // bind vertex buffer
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(buffer, 0, 1, &vertex_buffer_, offsets);

            vkCmdDraw(buffer, static_cast<uint32_t>(TriangleVertices.size()), 1, 0, 0);

            vkCmdEndRenderPass(buffer);

            assertm("can't end record command buffer", vkEndCommandBuffer(buffer) == VK_SUCCESS);
        }
    }

    void createSemaphores() {
        VkSemaphoreCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        assertm("create image avaliable semaphore failed", vkCreateSemaphore(device_, &create_info, nullptr, &image_avaliable_semaphore_) == VK_SUCCESS);
        assertm("create present finish semaphore failed", vkCreateSemaphore(device_, &create_info, nullptr, &present_finish_semaphore_) == VK_SUCCESS);
    }

    void createVertexBuffer() {
        VkDeviceSize size = sizeof(Vertex)*TriangleVertices.size();

        VkBuffer staging_buffer;
        VkDeviceMemory staging_buf_memory;
        createBuffer(size,
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                     staging_buffer, staging_buf_memory);

        void* data;
        vkMapMemory(device_, staging_buf_memory, 0, size, 0, &data);
        memcpy(data, TriangleVertices.data(), size);
        vkUnmapMemory(device_, staging_buf_memory);

        createBuffer(size,
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     vertex_buffer_, vertex_buf_memory_);

        copyBuffer(staging_buffer, vertex_buffer_, size);

        vkDestroyBuffer(device_, staging_buffer, nullptr);
        vkFreeMemory(device_, staging_buf_memory, nullptr);
    }

    void copyBuffer(VkBuffer& src, VkBuffer& dst, VkDeviceSize size) {
        VkCommandBufferAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool = commandpool_;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = 1;

        VkCommandBuffer buffer;
        vkAllocateCommandBuffers(device_, &allocate_info, &buffer);

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(buffer, &begin_info);

        VkBufferCopy region = {};
        region.size = size;
        region.srcOffset = 0;
        region.dstOffset = 0;
        vkCmdCopyBuffer(buffer, src, dst, 1, &region);

        vkEndCommandBuffer(buffer);

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &buffer;

        vkQueueSubmit(graphic_queue_, 1, &submit_info, nullptr);
        vkQueueWaitIdle(graphic_queue_);

        vkFreeCommandBuffers(device_, commandpool_, 1, &buffer);
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory) {
        VkBufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.usage = usage;
        create_info.size = size;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        assertm("create buffer failed", vkCreateBuffer(device_, &create_info, nullptr, &buffer) == VK_SUCCESS);

        VkMemoryRequirements requirements = {};
        vkGetBufferMemoryRequirements(device_, buffer, &requirements);

        VkMemoryAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize = requirements.size;
        allocate_info.memoryTypeIndex = findMemoryType(requirements.memoryTypeBits, properties);

        assertm("can't allocate memory", vkAllocateMemory(device_, &allocate_info, nullptr, &memory) == VK_SUCCESS);

        vkBindBufferMemory(device_, buffer, memory, 0);
    }

    uint32_t findMemoryType(uint32_t typefilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(physical_device_, &mem_properties);

        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
            if ((typefilter & (1<<i)) &&
                (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("no suitable memory type");
    }

    void drawFrame() {
        uint32_t image_idx;
        vkAcquireNextImageKHR(device_, swapchain_, std::numeric_limits<uint64_t>::max(), image_avaliable_semaphore_, nullptr, &image_idx);

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore wait_semaphores[] = {image_avaliable_semaphore_};
        VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        // the submit will block untill wait_semaphores signalled;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = wait_semaphores;

        // the stage(situation) you want to wait the semaphore
        submit_info.pWaitDstStageMask = wait_stages;

        // the command you want to send
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffers_.at(image_idx);

        VkSemaphore signal_semaphores[] = {present_finish_semaphore_};
        // the sumbit will signal the present_finish_semaphore_ when finish
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = signal_semaphores;

        assertm("can't submit command", vkQueueSubmit(graphic_queue_, 1, &submit_info, nullptr) == VK_SUCCESS);

        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.pImageIndices = &image_idx;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &swapchain_;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = signal_semaphores;

        assertm("queue present failed", vkQueuePresentKHR(present_queue_, &present_info) == VK_SUCCESS);
    }

    void quitVulkan() {
        vkDestroyBuffer(device_, vertex_buffer_, nullptr);
        vkFreeMemory(device_, vertex_buf_memory_, nullptr);
        vkDestroySemaphore(device_, image_avaliable_semaphore_, nullptr);
        vkDestroySemaphore(device_, present_finish_semaphore_, nullptr);
        vkFreeCommandBuffers(device_, commandpool_, command_buffers_.size(), command_buffers_.data());
        for (auto& framebuffer: framebuffers_) {
            vkDestroyFramebuffer(device_, framebuffer, nullptr);
        }
        vkDestroyPipeline(device_, pipeline_, nullptr);
        vkDestroyRenderPass(device_, renderpass_, nullptr);
        vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
        for (auto& view: imageviews_) {
            vkDestroyImageView(device_, view, nullptr);
        }
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
        vkDestroyCommandPool(device_, commandpool_, nullptr);
        vkDestroyDevice(device_, nullptr);
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        vkDestroyInstance(instance_, nullptr);
    }
};

int main(int argc, char** argv) {
    App app;
    app.SetTitle("vertex buffers");
    app.Run();
    return 0;
}
