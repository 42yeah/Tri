#include "TriApp.hpp"

#include "TriFileUtils.hpp"
#include "TriGraphicsUtils.hpp"
#include "TriLog.hpp"

#include <glm/glm.hpp>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <set>

void TriApp::Init()
{
    // Initialize GLFW window
    if (!mpWindow)
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        mpWindow =
            glfwCreateWindow(width, height, mAppName.c_str(), nullptr, nullptr);
    }

    std::vector<const char *> reqInstanceExtensions;

    // Locate available instance extensions
    if (mInstanceExtensions.empty())
    {
        uint32_t numInstanceExtensions = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &numInstanceExtensions,
                                               nullptr);
        mInstanceExtensions.resize(numInstanceExtensions);
        vkEnumerateInstanceExtensionProperties(nullptr, &numInstanceExtensions,
                                               mInstanceExtensions.data());

        TriLogInfo() << "Number of available instance extensions: "
                     << numInstanceExtensions;

        for (size_t i = 0; i < numInstanceExtensions; i++)
        {
            TriLogVerbose() << "  " << mInstanceExtensions[i].extensionName;
        }

        // Get extensions required from GLFW
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions =
            glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        reqInstanceExtensions =
            std::vector(glfwExtensions, glfwExtensions + glfwExtensionCount);
#if TRI_WITH_VULKAN_VALIDATION
        reqInstanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        for (const char *reqExtension : reqInstanceExtensions)
        {
            auto position = std::find_if(
                mInstanceExtensions.begin(), mInstanceExtensions.end(),
                [&](const VkExtensionProperties &prop)
                { return std::string(reqExtension) == prop.extensionName; });

            if (position == mInstanceExtensions.end())
            {
                TriLogError() << "Missing instance extension: " << reqExtension;
                Finalize();
                return;
            }
        }

        TriLogInfo() << "All required instance extensions found";
    }

    std::vector<const char *> reqLayers;

    // Locate available instance layers
    if (mInstanceLayers.empty())
    {
#if TRI_WITH_VULKAN_VALIDATION
        // Req. layers
        reqLayers.emplace_back("VK_LAYER_KHRONOS_validation");

#endif

        uint32_t numLayers = 0;
        vkEnumerateInstanceLayerProperties(&numLayers, nullptr);
        mInstanceLayers.resize(numLayers);
        vkEnumerateInstanceLayerProperties(&numLayers, mInstanceLayers.data());

        TriLogInfo() << "Number of available layers: " << numLayers;

        for (size_t i = 0; i < numLayers; i++)
        {
            TriLogVerbose() << "  " << mInstanceLayers[i].layerName;
        }

        for (const char *reqLayer : reqLayers)
        {
            auto position = std::find_if(
                mInstanceLayers.begin(), mInstanceLayers.end(),
                [&](const VkLayerProperties &prop)
                { return std::string(reqLayer) == prop.layerName; });

            if (position == mInstanceLayers.end())
            {
                TriLogError() << "Missing required layer: " << reqLayer;
                Finalize();
                return;
            }
        }

        TriLogInfo() << "All required layers found";
    }

    // Create Vulkan instance
    if (!mInstance)
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.pApplicationName = mAppName.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.pApplicationInfo = &appInfo;

        VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{};
#if TRI_WITH_VULKAN_VALIDATION
        PopulateDebugUtilsMessengerCreateInfoEXT(debugUtilsMessengerCreateInfo);
        createInfo.pNext = &debugUtilsMessengerCreateInfo;
#endif

        TriLogInfo() << "Number of requested instance extensions: "
                     << reqInstanceExtensions.size();
        for (size_t i = 0; i < reqInstanceExtensions.size(); i++)
        {
            TriLogVerbose() << "  " << reqInstanceExtensions[i];
        }

        TriLogInfo() << "Number of requested instance layers: "
                     << reqLayers.size();
        for (size_t i = 0; i < reqLayers.size(); i++)
        {
            TriLogVerbose() << "  " << reqLayers[i];
        }

        createInfo.enabledExtensionCount = reqInstanceExtensions.size();
        createInfo.ppEnabledExtensionNames = reqInstanceExtensions.data();
        createInfo.enabledLayerCount = reqLayers.size();
        createInfo.ppEnabledLayerNames = reqLayers.data();

        VkResult result = vkCreateInstance(&createInfo, nullptr, &mInstance);
        if (result != VK_SUCCESS)
        {
            TriLogError() << "Failed to create VkInstance: " << result;
            Finalize();
            return;
        }

        TriLogInfo() << "VkInstance created: " << mInstance;
    }

    mLibrary.Init();

#if TRI_WITH_VULKAN_VALIDATION
    if (!mDebugUtilsMessenger)
    {
        // Setup VkDebugUtilsMessenger
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        PopulateDebugUtilsMessengerCreateInfoEXT(createInfo);

        VkResult result = mLibrary.CreateDebugUtilsMessengerEXT(
            mInstance, &createInfo, nullptr, &mDebugUtilsMessenger);

        if (result != VK_SUCCESS)
        {
            TriLogError() << "Failed to create Vulkan DebugUtilsMessenger - "
                             "there will be no messages from validation layer";
        }
    }
#endif

    // Create GLFW window surface
    if (!mSurface)
    {
        VkResult result =
            glfwCreateWindowSurface(mInstance, mpWindow, nullptr, &mSurface);

        if (result != VK_SUCCESS)
        {
            TriLogError() << "Failed to create Vulkan window surface: "
                          << result;
            Finalize();
            return;
        }

        TriLogInfo() << "Vulkan window surface created: " << mSurface;
    }

    // Pick physical device extensions
    std::vector<const char *> reqDeviceExtensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    if (!mPhysicalDevice)
    {
        uint32_t deviceCount = 0;
        std::vector<VkPhysicalDevice> devices;
        vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
        devices.resize(deviceCount);
        vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

        TriLogInfo() << "Number of Vulkan-enabled devices: " << deviceCount;

        std::vector<std::pair<int, VkPhysicalDevice>> suitableDevices;

        for (size_t i = 0; i < deviceCount; i++)
        {
            VkPhysicalDevice device = devices[i];
            int score = RateDeviceSuitability(device, reqDeviceExtensions);

            if (score != 0)
            {
                suitableDevices.emplace_back(score, device);
            }
        }

        if (suitableDevices.empty())
        {
            TriLogError() << "No available Vulkan-enabled GPUs found";
            Finalize();
            return;
        }

        std::sort(suitableDevices.begin(), suitableDevices.end(),
                  [](const auto &left, const auto &right)
                  { return left.first < right.first; });

        TriLogInfo() << "Number of suitable Vulkan-enabled devices: "
                     << suitableDevices.size();

        mPhysicalDevice = suitableDevices[0].second;
    }

    // Create Vulkan logical device & queues
    if (!mDevice)
    {
        mQueueFamilyIndices = FindQueueFamilies();

        if (!mQueueFamilyIndices.IsComplete())
        {
            TriLogError() << "Incomplete queue family index list";
            Finalize();
            return;
        }

        std::set<uint32_t> uniqueQueueIndices{
            *mQueueFamilyIndices.graphicsFamily,
            *mQueueFamilyIndices.presentFamily};

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

        float queuePriority = 1.0f;

        for (uint32_t index : uniqueQueueIndices)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.pNext = nullptr;
            queueCreateInfo.queueFamilyIndex = index;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.emplace_back(std::move(queueCreateInfo));
        }

        TriLogInfo() << "Number of unique queues (graphics + present): "
                     << queueCreateInfos.size();

        /* TODO(42): Query used features at RateDeviceSuitability and use them
           over here - right now we don't use any feats
        */
        VkPhysicalDeviceFeatures deviceFeats{};

        // Create device
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.queueCreateInfoCount = queueCreateInfos.size();
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeats;
        createInfo.enabledExtensionCount = reqDeviceExtensions.size();
        createInfo.ppEnabledExtensionNames = reqDeviceExtensions.data();

        // We are NOT going to enable validation layers for this one

        VkResult result =
            vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice);

        if (result != VK_SUCCESS)
        {
            TriLogError() << "Failed to create logical Vulkan device";
            Finalize();
            return;
        }

        vkGetDeviceQueue(mDevice, *mQueueFamilyIndices.graphicsFamily, 0,
                         &mGraphicsQueue);
        vkGetDeviceQueue(mDevice, *mQueueFamilyIndices.presentFamily, 0,
                         &mPresentQueue);

        TriLogInfo() << "Device created: " << mDevice
                     << ", with graphics queue: " << mGraphicsQueue
                     << ", present queue: " << mPresentQueue;
    }

    if (!mSwapChain)
    {
        // TODO(42): Do something about swap chains

        SwapChainSupportDetails details =
            QuerySwapChainSupport(mPhysicalDevice);

        mSurfaceFormat = ChooseSwapSurfaceFormat(details.formats);
        mPresentMode = ChooseSwapPresentMode(details.presentModes);
        mSwapExtent = ChooseSwapExtent(details.capabilities);

        const VkSurfaceCapabilitiesKHR &capabilities = details.capabilities;

        // How many images before the producer queue becomes full
        uint32_t imageCount = capabilities.minImageCount;
        uint32_t maxImageCount = capabilities.maxImageCount;
        if (maxImageCount != 0)
        {
            imageCount = glm::clamp(imageCount, imageCount + 1, maxImageCount);
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.pNext = nullptr;
        createInfo.surface = mSurface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = mSurfaceFormat.format;
        createInfo.imageColorSpace = mSurfaceFormat.colorSpace;
        createInfo.imageExtent = mSwapExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        uint32_t queueIndicies[2] = {*mQueueFamilyIndices.graphicsFamily,
                                     *mQueueFamilyIndices.presentFamily};

        if (queueIndicies[0] == queueIndicies[1])
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 1;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
        }

        createInfo.pQueueFamilyIndices = queueIndicies;

        // No transforms, thank you very much
        createInfo.preTransform = capabilities.currentTransform;

        // Do not blend
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode = mPresentMode;
        createInfo.clipped = true;

        createInfo.oldSwapchain = nullptr;

        VkResult result =
            vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapChain);

        if (result != VK_SUCCESS)
        {
            TriLogError() << "Failed to create swap chain";
            Finalize();
            return;
        }

        TriLogInfo() << "Swap chain created: " << mSwapChain;

        // Retrieve swap chain images
        uint32_t numSwapChainImages = 0;
        vkGetSwapchainImagesKHR(mDevice, mSwapChain, &numSwapChainImages,
                                nullptr);
        mSwapChainImages.resize(numSwapChainImages);
        vkGetSwapchainImagesKHR(mDevice, mSwapChain, &numSwapChainImages,
                                mSwapChainImages.data());

        TriLogInfo() << "Number of swap chain images: " << numSwapChainImages;
    }

    if (mSwapChainImageViews.empty())
    {
        mSwapChainImageViews.resize(mSwapChainImages.size());

        for (size_t i = 0; i < mSwapChainImages.size(); i++)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.image = mSwapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = mSurfaceFormat.format;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VkResult result = vkCreateImageView(mDevice, &createInfo, nullptr,
                                                &mSwapChainImageViews[i]);
            if (result != VK_SUCCESS)
            {
                TriLogError() << "Failed to create swap chain image view";
                Finalize();
                return;
            }
        }
    }

    if (!mRenderPass)
    {
        // Data side
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = mSurfaceFormat.format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // One subpass (shader side)
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &colorAttachment;
        createInfo.subpassCount = 1;
        createInfo.pSubpasses = &subpass;
        createInfo.dependencyCount = 1;
        createInfo.pDependencies = &dependency;

        VkResult result =
            vkCreateRenderPass(mDevice, &createInfo, nullptr, &mRenderPass);
        if (result != VK_SUCCESS)
        {
            TriLogError() << "Failed to create render pass";
            Finalize();
            return;
        }
    }

    /* This is gonna be REALLY long so I am breaking it off into its own
       function
    */
    VkResult result = InitGraphicsPipeline();

    if (result != VK_SUCCESS)
    {
        TriLogError() << "Failed during graphics pipeline initialization";
        Finalize();
        return;
    }

    if (mFramebuffers.empty())
    {
        mFramebuffers.resize(mSwapChainImageViews.size());
        for (size_t i = 0; i < mSwapChainImageViews.size(); i++)
        {
            // Create a framebuffer for each swap chain image view
            VkImageView attachments[] = {mSwapChainImageViews[i]};

            VkFramebufferCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.renderPass = mRenderPass;
            createInfo.attachmentCount = 1;
            createInfo.pAttachments = attachments;
            createInfo.width = mSwapExtent.width;
            createInfo.height = mSwapExtent.height;
            createInfo.layers = 1;

            VkResult result = vkCreateFramebuffer(mDevice, &createInfo, nullptr,
                                                  &mFramebuffers[i]);

            if (result != VK_SUCCESS)
            {
                TriLogError()
                    << "Failed during swap chain creation (" << i << ")";
                Finalize();
                return;
            }
        }

        TriLogInfo() << "Number of framebuffers created: "
                     << mFramebuffers.size();
    }

    if (!mCommandPool)
    {
        VkCommandPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        createInfo.queueFamilyIndex = *mQueueFamilyIndices.graphicsFamily;

        VkResult result =
            vkCreateCommandPool(mDevice, &createInfo, nullptr, &mCommandPool);
        if (result != VK_SUCCESS)
        {
            TriLogError() << "Failed to create command pool";
            Finalize();
            return;
        }

        // Allocate ONE (1) single command buffer
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.commandPool = mCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        result = vkAllocateCommandBuffers(mDevice, &allocInfo, &mCommandBuffer);
        if (result != VK_SUCCESS)
        {
            TriLogError() << "Failed to allocate command buffer";
            Finalize();
            return;
        }
    }

    // Create synchronization objects/entities
    VkSemaphoreCreateInfo semaCreateInfo{};
    semaCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaCreateInfo.pNext = nullptr;

    if (!mImageAvailableSemaphore)
    {
        VkResult result = vkCreateSemaphore(mDevice, &semaCreateInfo, nullptr,
                                            &mImageAvailableSemaphore);
        if (result != VK_SUCCESS)
        {
            TriLogError() << "Failed to create image available semaphore";
            Finalize();
            return;
        }
    }
    if (!mRenderFinishedSemaphore)
    {
        VkResult result = vkCreateSemaphore(mDevice, &semaCreateInfo, nullptr,
                                            &mRenderFinishedSemaphore);
        if (result != VK_SUCCESS)
        {
            TriLogError() << "Failed to create render finished semaphore";
            Finalize();
            return;
        }
    }
    if (!mInFlightFence)
    {
        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.pNext = nullptr;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
        VkResult result =
            vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &mInFlightFence);
        if (result != VK_SUCCESS)
        {
            TriLogError() << "Failed to create in-flight fence";
            Finalize();
            return;
        }
    }
}

VkResult TriApp::InitGraphicsPipeline()
{
    std::optional<std::vector<char>> vertexShaderCode =
        ReadBinaryFile("Shaders/triangle.vert.svc");
    std::optional<std::vector<char>> fragmentShaderCode =
        ReadBinaryFile("Shaders/triangle.frag.svc");

    if (!vertexShaderCode.has_value() || !fragmentShaderCode.has_value())
    {
        TriLogError() << "Failed to create vertex/fragment shader(s)";
        return VK_RESULT_MAX_ENUM;
    }

    VkShaderModule vertexShader = CreateShaderModule(*vertexShaderCode);
    VkShaderModule fragmentShader = CreateShaderModule(*fragmentShaderCode);

    // Vertex shader create info
    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo{};
    vertexShaderCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderCreateInfo.pNext = nullptr;
    vertexShaderCreateInfo.module = vertexShader;
    vertexShaderCreateInfo.pName = "main";

    // Fragment shader create info
    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo{};
    fragmentShaderCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderCreateInfo.pNext = nullptr;
    fragmentShaderCreateInfo.module = fragmentShader;
    fragmentShaderCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = {vertexShaderCreateInfo,
                                                fragmentShaderCreateInfo};

    // Dynamic states
    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                 VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.pNext = nullptr;
    dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    // Pipeline vertex input state
    VkPipelineVertexInputStateCreateInfo vertexCreateInfo{};
    vertexCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexCreateInfo.pNext = nullptr;
    vertexCreateInfo.vertexBindingDescriptionCount = 0;
    vertexCreateInfo.pVertexBindingDescriptions = nullptr;
    vertexCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexCreateInfo.pVertexAttributeDescriptions = nullptr;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.pNext = nullptr;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Viewport & scissor (if fixed)
    // VkViewport viewport{}; ...

    VkPipelineViewportStateCreateInfo viewportCreateInfo{};
    viewportCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCreateInfo.pNext = nullptr;
    viewportCreateInfo.viewportCount = 1;
    // viewportCreateInfo.pViewports = &viewport;
    viewportCreateInfo.scissorCount = 1;
    // viewportCreateInfo.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.pNext = nullptr;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    // Multisampling: disabled for now
    VkPipelineMultisampleStateCreateInfo multiSample{};
    multiSample.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multiSample.pNext = nullptr;
    multiSample.sampleShadingEnable = VK_FALSE;
    multiSample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multiSample.minSampleShading = 1.0f;
    multiSample.pSampleMask = nullptr;
    multiSample.alphaToCoverageEnable = VK_FALSE;
    multiSample.alphaToOneEnable = VK_FALSE;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    // Basically same as above
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCreateInfo.pNext = nullptr;
    layoutCreateInfo.setLayoutCount = 0;
    layoutCreateInfo.pSetLayouts = nullptr;
    layoutCreateInfo.pushConstantRangeCount = 0;
    layoutCreateInfo.pPushConstantRanges = nullptr;

    if (!mPipelineLayout)
    {
        VkResult result = vkCreatePipelineLayout(mDevice, &layoutCreateInfo,
                                                 nullptr, &mPipelineLayout);

        if (result != VK_SUCCESS)
        {
            TriLogError() << "Failed to create VkPipelineLayout";
        }
    }

    if (!mGraphicsPipeline)
    {
        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.sType =
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.pNext = nullptr;
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = stages;
        pipelineCreateInfo.pVertexInputState = &vertexCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
        pipelineCreateInfo.pViewportState = &viewportCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizer;
        pipelineCreateInfo.pMultisampleState = &multiSample;
        pipelineCreateInfo.pDepthStencilState = nullptr;
        pipelineCreateInfo.pColorBlendState = &colorBlending;
        pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
        pipelineCreateInfo.layout = mPipelineLayout;
        pipelineCreateInfo.renderPass = mRenderPass;
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.basePipelineHandle = nullptr;
        pipelineCreateInfo.basePipelineIndex = -1;

        VkResult result =
            vkCreateGraphicsPipelines(mDevice, nullptr, 1, &pipelineCreateInfo,
                                      nullptr, &mGraphicsPipeline);
        if (result != VK_SUCCESS)
        {
            TriLogError() << "Failed to create VkGraphicsPipeline";
        }
    }

    vkDestroyShaderModule(mDevice, vertexShader, nullptr);
    vkDestroyShaderModule(mDevice, fragmentShader, nullptr);

    TriLogInfo() << "Graphics pipeline creation done: " << mGraphicsPipeline;

    return VK_SUCCESS;
}

void TriApp::Loop()
{
    while (mpWindow && !glfwWindowShouldClose(mpWindow))
    {
        glfwPollEvents();
        RenderFrame();
    }
}

void TriApp::Finalize()
{
    if (mDevice)
        vkDeviceWaitIdle(mDevice);
    
    if (mImageAvailableSemaphore)
    {
        vkDestroySemaphore(mDevice, mImageAvailableSemaphore, nullptr);
        mImageAvailableSemaphore = nullptr;
    }
    if (mRenderFinishedSemaphore)
    {
        vkDestroySemaphore(mDevice, mRenderFinishedSemaphore, nullptr);
        mRenderFinishedSemaphore = nullptr;
    }
    if (mInFlightFence)
    {
        vkDestroyFence(mDevice, mInFlightFence, nullptr);
        mInFlightFence = nullptr;
    }

    if (mCommandPool)
    {
        vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
        mCommandBuffer = nullptr;
        mCommandPool = nullptr;
    }

    if (!mFramebuffers.empty())
    {
        for (VkFramebuffer framebuffer : mFramebuffers)
        {
            vkDestroyFramebuffer(mDevice, framebuffer, nullptr);
        }
        mFramebuffers.clear();
    }

    if (mRenderPass)
    {
        vkDestroyRenderPass(mDevice, mRenderPass, nullptr);
        mRenderPass = nullptr;
    }

    if (mGraphicsPipeline)
    {
        vkDestroyPipeline(mDevice, mGraphicsPipeline, nullptr);
        mGraphicsPipeline = nullptr;
    }

    if (mPipelineLayout)
    {
        vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
        mPipelineLayout = nullptr;
    }

    if (!mSwapChainImageViews.empty())
    {
        for (const VkImageView &imageView : mSwapChainImageViews)
        {
            vkDestroyImageView(mDevice, imageView, nullptr);
        }
        mSwapChainImageViews.clear();
    }

    if (mSwapChain)
    {
        vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);
        mSwapChain = nullptr;
    }

    if (mGraphicsQueue)
        mGraphicsQueue = nullptr;

    if (mPresentQueue)
        mPresentQueue = nullptr;

    if (mSurface)
    {
        vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
        mSurface = nullptr;
    }

    if (mDevice)
    {
        vkDestroyDevice(mDevice, nullptr);
        mDevice = nullptr;
    }

    if (mPhysicalDevice)
    {
        // Vulkan will implicitly destroy the device during the destruction of
        // VkInstance
        mPhysicalDevice = nullptr;
    }

#if TRI_WITH_VULKAN_VALIDATION
    if (mDebugUtilsMessenger)
    {
        mLibrary.DestroyDebugUtilsMessengerEXT(mInstance, mDebugUtilsMessenger,
                                               nullptr);
        mDebugUtilsMessenger = nullptr;
    }
#endif

    mLibrary.Finalize();

    if (mInstance)
    {
        TriLogInfo() << "Finalizing VkInstance: " << mInstance;
        vkDestroyInstance(mInstance, nullptr);
        mInstance = nullptr;
    }

    if (mpWindow)
    {
        glfwDestroyWindow(mpWindow);
        glfwTerminate();
        mpWindow = nullptr;
    }

    mDeviceExtensions.clear();

    mInstanceExtensions.clear();
    mInstanceLayers.clear();
}

VKAPI_ATTR VkBool32 VKAPI_CALL TriApp::VKDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    // TriApp *that = static_cast<TriApp *>(pUserData);

    switch (severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        TriLogVerbose() << "[VK] " << pCallbackData->pMessage;
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        TriLogInfo() << "[VK] " << pCallbackData->pMessage;
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        TriLogWarning() << "[VK] " << pCallbackData->pMessage;
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        TriLogError() << "[VK] " << pCallbackData->pMessage;
#ifndef NDEBUG
        std::abort();
#endif
        break;

    default:
        TriLogWarning() << "[VK] Unknown severity: " << pCallbackData->pMessage;
        break;
    }

    return VK_FALSE;
}

void TriApp::PopulateDebugUtilsMessengerCreateInfoEXT(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo)
{
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.pNext = nullptr;

    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    createInfo.pfnUserCallback = TriApp::VKDebugCallback;
    createInfo.pUserData = this;
}

int TriApp::RateDeviceSuitability(
    VkPhysicalDevice device, const std::vector<const char *> &reqExtensions)
{
    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceFeatures feats;
    vkGetPhysicalDeviceProperties(device, &props);
    vkGetPhysicalDeviceFeatures(device, &feats);

    uint32_t numDeviceExtensions = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &numDeviceExtensions,
                                         nullptr);
    mDeviceExtensions.resize(numDeviceExtensions);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &numDeviceExtensions,
                                         mDeviceExtensions.data());

    // TODO(42): Manually add swap chain support check here (even if it is
    // missing)

    for (const char *extension : reqExtensions)
    {
        auto position = std::find_if(
            mDeviceExtensions.begin(), mDeviceExtensions.end(),
            [&](const VkExtensionProperties &props)
            { return std::string(extension) == props.extensionName; });

        if (position == mDeviceExtensions.end())
        {
            TriLogError() << "Device '" << props.deviceName
                          << "' is missing required device extension: "
                          << extension;
            return 0;
        }
    }

    TriLogVerbose() << "All required device extensions found for device '"
                    << props.deviceName << "'";

    SwapChainSupportDetails details = QuerySwapChainSupport(device);

    if (details.formats.empty() || details.presentModes.empty())
    {
        // The swap chain cannot be presented
        TriLogWarning() << "Device does not support swap chain with any "
                           "formats/present modes";
        return 0;
    }

    int score = 0;

    switch (props.deviceType)
    {
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        score = 1;
        break;

    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        score = 2;
        break;

    default:
        score = 0;
        break;
    }

    if (feats.geometryShader)
        score *= 2;

    if (feats.tessellationShader)
        score *= 2;

    if (score == 0)
    {
        TriLogWarning() << "Device '" << props.deviceName << "' unsuitable";
    }
    else
    {
        TriLogVerbose() << "Device '" << props.deviceName << "' has a score of "
                        << score;
    }

    return score;
}

QueueFamilyIndices TriApp::FindQueueFamilies()
{
    QueueFamilyIndices indices{};

    uint32_t numQueueFamilies = 0;
    std::vector<VkQueueFamilyProperties> queueFamilyProps;

    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &numQueueFamilies,
                                             nullptr);
    queueFamilyProps.resize(numQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &numQueueFamilies,
                                             queueFamilyProps.data());

    TriLogInfo() << "Queue family count: " << numQueueFamilies;

    size_t i = 0;
    for (const VkQueueFamilyProperties &prop : queueFamilyProps)
    {
        TriLogVerbose() << "Queue family #" << i << " flags: 0x" << std::hex
                        << prop.queueFlags << std::dec;

        if (prop.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(
            mPhysicalDevice, i, mSurface, &presentSupport);
        if (result == VK_SUCCESS)
        {
            indices.presentFamily = i;
        }
        else
        {
            TriLogWarning() << "Failed to get physical device surface support "
                               "info for queue family index "
                            << i;
        }

        i++;

        if (indices.IsComplete())
        {
            break;
        }
    }

    return indices;
}

SwapChainSupportDetails
TriApp::QuerySwapChainSupport(VkPhysicalDevice physicalDevice)
{
    SwapChainSupportDetails details{};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, mSurface,
                                              &details.capabilities);

    uint32_t numFormats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, mSurface, &numFormats,
                                         nullptr);
    details.formats.resize(numFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, mSurface, &numFormats,
                                         details.formats.data());

    if (details.formats.empty())
    {
        TriLogWarning() << "No surface formats available for device";
    }

    uint32_t numPresentModes = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, mSurface,
                                              &numPresentModes, nullptr);
    details.presentModes.resize(numPresentModes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, mSurface,
                                              &numPresentModes,
                                              details.presentModes.data());

    return details;
}

VkSurfaceFormatKHR TriApp::ChooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
    for (const VkSurfaceFormatKHR &format : availableFormats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    // Just use the first one
    return availableFormats[0];
}

VkPresentModeKHR
TriApp::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &presentModes)
{
    // VK_PRESENT_MODE_FIFO_KHR is guaranteed to be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
TriApp::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
{
    uint32_t numericMax = std::numeric_limits<uint32_t>::max();
    const VkExtent2D &currentExtent = capabilities.currentExtent;

    if (currentExtent.width != numericMax && currentExtent.height != numericMax)
    {
        TriLogVerbose() << "Swap chain extent as specified by capabilities: "
                        << currentExtent.width << ", " << currentExtent.height;
        return currentExtent;
    }

    const VkExtent2D &minExtent = capabilities.minImageExtent;
    const VkExtent2D &maxExtent = capabilities.maxImageExtent;

    int fbWidth = 0;
    int fbHeight = 0;
    glfwGetFramebufferSize(mpWindow, &fbWidth, &fbHeight);

    VkExtent2D ret{};
    ret.width = glm::clamp(static_cast<uint32_t>(fbWidth), minExtent.width,
                           maxExtent.width);
    ret.height = glm::clamp(static_cast<uint32_t>(fbHeight), minExtent.width,
                            maxExtent.height);

    TriLogVerbose() << "Retrieved clamped GLFW frame buffer size: " << ret.width
                    << ", " << ret.height;

    return ret;
}

VkShaderModule TriApp::CreateShaderModule(const std::vector<char> &svcBuffer)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.codeSize = svcBuffer.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(svcBuffer.data());

    VkShaderModule shaderModule = nullptr;
    VkResult result =
        vkCreateShaderModule(mDevice, &createInfo, nullptr, &shaderModule);

    if (result != VK_SUCCESS)
    {
        TriLogError() << "Failed during Vulkan shader module creation";
        return nullptr;
    }

    return shaderModule;
}

bool TriApp::RecordCommandBuffer(VkCommandBuffer commandBuffer,
                                 uint32_t imageIndex)
{
    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;

    VkResult result =
        vkBeginCommandBuffer(mCommandBuffer, &commandBufferBeginInfo);

    if (result != VK_SUCCESS)
    {
        TriLogError() << "Failed to begin command buffer";
        return false;
    }

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = mRenderPass;
    renderPassBeginInfo.framebuffer = mFramebuffers[imageIndex];
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = mSwapExtent;
    VkClearValue clearColor{};
    clearColor.color = {{1.0f, 0.0f, 1.0f, 1.0f}};
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(mCommandBuffer, &renderPassBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      mGraphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(mSwapExtent.width);
    viewport.height = static_cast<float>(mSwapExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = mSwapExtent;

    vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(mCommandBuffer, 0, 1, &scissor);

    vkCmdDraw(mCommandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(mCommandBuffer);

    result = vkEndCommandBuffer(mCommandBuffer);

    if (result != VK_SUCCESS)
    {
        TriLogError() << "Failed to end command buffer";
        return false;
    }

    return true;
}

void TriApp::RenderFrame()
{
    uint64_t infinite = std::numeric_limits<uint64_t>::max();
    vkWaitForFences(mDevice, 1, &mInFlightFence, true,
                    infinite);
    vkResetFences(mDevice, 1, &mInFlightFence);
    
    uint32_t imageIndex = 0;
    vkAcquireNextImageKHR(mDevice, mSwapChain, infinite,
                          mImageAvailableSemaphore, nullptr, &imageIndex);

    TriLogVerbose() << "Draw one frame on swap chain image: #" << imageIndex;

    // Though this may be unnecessary, it's better that we reset it
    vkResetCommandBuffer(mCommandBuffer, 0);
    RecordCommandBuffer(mCommandBuffer, imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;

    // Wait until swap chain image is available (signaled after
    // vkAcquireNextImageKHR)
    VkSemaphore waitSemaphores[] = {mImageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffer;

    VkSemaphore signalSemaphore[] = {mRenderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphore;

    VkResult result =
        vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mInFlightFence);

    if (result != VK_SUCCESS)
    {
        TriLogError() << "Failed to submit command buffer to queue";
        return;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &mRenderFinishedSemaphore;

    VkSwapchainKHR swapChains[] = {mSwapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(mPresentQueue, &presentInfo);

    if (result != VK_SUCCESS)
    {
        TriLogError() << "Failed to present queue";
        return;
    }
}
