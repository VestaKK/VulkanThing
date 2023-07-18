#define GLFW_INCLUDE_VULKAN
#define CGLM_USE_ANONYMOUS_STRUCT 1
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <cglm/types-struct.h>
#include <defines.h>

#define EXT_COUNT 3
#define LAYER_COUNT 1 
#define DEVICE_EXT_COUNT 1
#define WIDTH 640
#define HEIGHT 480
#define MAX_FRAMES_IN_FLIGHT 2

#define VK_CHECK(x) if(x != VK_SUCCESS) { printf("ERROR IN FUNCTION, LINE: %d\n", __LINE__); Assert(0); }

const char* const vulkanExtensions[EXT_COUNT] = { VK_KHR_SURFACE_EXTENSION_NAME, "VK_KHR_win32_surface", VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
const char* const vulkanLayerNames[LAYER_COUNT] = { "VK_LAYER_KHRONOS_validation" };
const char* const vulkanDeviceExtensions[DEVICE_EXT_COUNT] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

global_variable VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
    void*                                            pUserData) {
    printf("%s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

typedef struct debug_read_file_result {
    uint32 contentSize;
    void* contents;
} debug_read_file_result;

typedef struct Shaders {
    VkPipelineShaderStageCreateInfo vertCI, fragCI;
} Shaders;

typedef struct Vertex {
    vec2 pos;
    vec3 color;
} Vertex;

typedef struct vulkan_state {

    uint32 width;
    uint32 height;

    VkInstance instance;
    GLFWwindow* window;
    VkBool32 framebufferResized;

    VkDebugUtilsMessengerEXT debugMessenger;

    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkExtent2D surfaceExtent;

    VkPhysicalDevice gpu;
    VkPhysicalDeviceMemoryProperties memoryProps;
    uint32 QFIndex;
    uint32 QFQueueCount;
    real32 QueuePriority;

    VkDevice device;
    VkQueue deviceQueue;

    VkSwapchainKHR swapchain;
    VkExtent2D swapchainExtent;
    VkViewport viewport;
    VkRect2D scissor;
    uint32 imageCount;
    VkImage* images;
    VkImageView* imageViews;
    VkFramebuffer* framebuffers;
    VkFormat format;
    uint32 currentFrame;

    VkShaderModule vertModule;
    VkShaderModule fragModule;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];

    VkSemaphore sm1[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore sm2[MAX_FRAMES_IN_FLIGHT];
    VkFence fences[MAX_FRAMES_IN_FLIGHT];

} VKState;

internal void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    VKState* state = (VKState*) (glfwGetWindowUserPointer(window));
    state->framebufferResized = VK_TRUE;
}

VkResult createInstance(VKState* state) {

    uint32 layerPropertyCount = 0;
    vkEnumerateInstanceLayerProperties(&layerPropertyCount, NULL);
    VkLayerProperties* layerProperties = calloc(layerPropertyCount, sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layerPropertyCount, layerProperties);
    for (int i=0 ; i<layerPropertyCount ; i++) {
        printf("%s\n", layerProperties[i].layerName);
    }

    FREE(layerProperties);

    uint32 apiVersion = 0;
    vkEnumerateInstanceVersion(&apiVersion);

    VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.pApplicationName = "VkProject";
    appInfo.applicationVersion = 1;
    appInfo.engineVersion = 1;
    appInfo.apiVersion = apiVersion;
    appInfo.pEngineName = "An Engine";

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
    debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugMessengerInfo.messageType =  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugMessengerInfo.pfnUserCallback = debugCallback;
    debugMessengerInfo.pUserData = NULL;

    VkInstanceCreateInfo instanceInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = EXT_COUNT;
    instanceInfo.ppEnabledExtensionNames = vulkanExtensions;
    instanceInfo.enabledLayerCount = LAYER_COUNT;
    instanceInfo.ppEnabledLayerNames = vulkanLayerNames;
    instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugMessengerInfo;

    VK_CHECK(vkCreateInstance(&instanceInfo, NULL, &state->instance));

    PFN_vkCreateDebugUtilsMessengerEXT pVkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(state->instance, "vkCreateDebugUtilsMessengerEXT");
    VK_CHECK(pVkCreateDebugUtilsMessengerEXT(state->instance, &debugMessengerInfo, NULL, &state->debugMessenger));

    state->surfaceExtent.width = state->width;
    state->surfaceExtent.height = state->height;

    VK_CHECK(glfwCreateWindowSurface(state->instance, state->window, NULL, &state->surface));

    return VK_SUCCESS;
}

VkResult createDevice(VKState* state) {
    state->QueuePriority = 1.0f;
    
    VkDeviceQueueCreateInfo deviceQueueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    deviceQueueCreateInfo.queueFamilyIndex = state->QFIndex;
    deviceQueueCreateInfo.queueCount = state->QFQueueCount;
    deviceQueueCreateInfo.pQueuePriorities = &state->QueuePriority;

    VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    deviceCreateInfo.enabledExtensionCount = DEVICE_EXT_COUNT;
    deviceCreateInfo.ppEnabledExtensionNames = vulkanDeviceExtensions;
    deviceCreateInfo.enabledLayerCount = LAYER_COUNT;
    deviceCreateInfo.ppEnabledLayerNames = vulkanLayerNames;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;

    VK_CHECK(vkCreateDevice(state->gpu, &deviceCreateInfo, NULL, &state->device));
    vkGetDeviceQueue(state->device, state->QFIndex, 0, &state->deviceQueue);
    return VK_SUCCESS;
}

void selectGPU(VKState* state) {
    uint32 physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(state->instance, &physicalDeviceCount, NULL);
    VkPhysicalDevice* physicalDevices = calloc(physicalDeviceCount, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(state->instance, &physicalDeviceCount, physicalDevices);

    for (uint32 i=0 ; i<physicalDeviceCount ; i++) {
        printf("\nDevice Handle: %p\n", physicalDevices[i]);

        uint32 propertyCount = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevices[i], NULL, &propertyCount, NULL);
        VkExtensionProperties* properties = calloc(propertyCount, sizeof(VkExtensionProperties));
        vkEnumerateDeviceExtensionProperties(physicalDevices[i], NULL, &propertyCount, properties);

        bool32 swapchainSupport = FALSE;

        for (int i=0; i<propertyCount; i++) {

            // printf("%s\n",properties[i].extensionName);

            for (int j=0; j<DEVICE_EXT_COUNT; j++) {
                if (strcmp(vulkanDeviceExtensions[j], properties[i].extensionName)) {
                    swapchainSupport = TRUE;
                    break;
                }
            }
        }

        FREE(properties);

        if (!swapchainSupport) continue;

        uint32 queuePropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queuePropertyCount, NULL);
        VkQueueFamilyProperties* queueFamilyProperties = calloc(queuePropertyCount, sizeof(VkQueueFamilyProperties));
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queuePropertyCount, queueFamilyProperties);

        for (uint32 queueFamilyIndex=0 ; queueFamilyIndex<queuePropertyCount ; queueFamilyIndex++) {
            VkBool32 surfaceSupport;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[i], queueFamilyIndex, state->surface, &surfaceSupport);

            char TextBuffer[64];
            printf("QueueFamily: %d\n", queueFamilyIndex);
            wsprintf(TextBuffer, "Surface Support: %s\n", (surfaceSupport == VK_TRUE) ? "Yes" : "No");
            printf("%s", TextBuffer);

            if (surfaceSupport == VK_TRUE) {
                if (queueFamilyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    printf("Graphics Support: Yes\n");
                    state->gpu = physicalDevices[i];
                    state->QFIndex = queueFamilyIndex;
                    state->QFQueueCount = 1; //queueFamilyProperties[queueFamilyIndex].queueCount;
                    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->gpu, state->surface, &state->surfaceCapabilities);
                    vkGetPhysicalDeviceMemoryProperties(state->gpu, &state->memoryProps);
                    break;
                }
            }

            if (state->gpu != NULL) break;
        }
        FREE(queueFamilyProperties);
    }
    FREE(physicalDevices); 
}

VkResult setupSwapchainKHR(VKState* state) {

    state->swapchainExtent = state->surfaceExtent;

    VkSwapchainCreateInfoKHR swapchainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swapchainCreateInfo.surface = state->surface;
    swapchainCreateInfo.imageExtent = state->swapchainExtent;
    state->format = VK_FORMAT_B8G8R8A8_SRGB;
    swapchainCreateInfo.imageFormat = state->format;

    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.minImageCount = 3;
    swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 0;
    swapchainCreateInfo.pQueueFamilyIndices = NULL;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(state->device, &swapchainCreateInfo, NULL, &state->swapchain));

    vkGetSwapchainImagesKHR(state->device, state->swapchain, &state->imageCount, NULL);

    state->images = calloc(state->imageCount, sizeof(VkImage));
    state->imageViews = calloc(state->imageCount, sizeof(VkImageView));

    VK_CHECK(vkGetSwapchainImagesKHR(state->device, state->swapchain, &state->imageCount, state->images));

    for(int i=0 ; i<state->imageCount ; i++) {

        VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        imageViewCreateInfo.image = state->images[i];
        imageViewCreateInfo.format = state->format;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;

        // NOTE: Is going to change
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(state->device, &imageViewCreateInfo, NULL, &(state->imageViews[i])));
    }

    return VK_SUCCESS;
}

uint32 SafeTruncateUInt64(uint64 Value)
{
    // TODO: Defines for maximum values
    Assert(Value <= 0xFFFFFFFF);
    uint32 result = (uint32)Value;
    return(result);
}

debug_read_file_result windows_read_file(char* fileName) {
    debug_read_file_result result = {};

    HANDLE fileHandle = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (fileHandle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(fileHandle, &fileSize)) {
            uint32 fileSize32 = SafeTruncateUInt64(fileSize.QuadPart);

            result.contents = VirtualAlloc(0, fileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

            if (result.contents) {
                DWORD bytesRead;
                if(ReadFile(fileHandle, result.contents, fileSize32, &bytesRead, 0) && 
                    (fileSize32 == bytesRead)) {
                    result.contentSize = fileSize32;
                }
                else {
                    VirtualFree(result.contents, 0, MEM_RELEASE);
                    result.contents = 0;
                }
            }
        }
        CloseHandle(fileHandle);
    }
    return result;
}

VkResult createShaderModule(VkDevice device, debug_read_file_result* fileResult, VkShaderModule* pModule) {
    
    VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    createInfo.flags = 0;
    createInfo.codeSize = fileResult->contentSize;
    createInfo.pCode = fileResult->contents;

    return vkCreateShaderModule(device, &createInfo, NULL, pModule);
}

VkResult createShaders(VKState* state, Shaders* shaders) {

    debug_read_file_result vertFile = windows_read_file("res/shaders/vert.spv");
    debug_read_file_result fragFile = windows_read_file("res/shaders/frag.spv");

    VK_CHECK(createShaderModule(state->device, &vertFile, &state->vertModule));
    VK_CHECK(createShaderModule(state->device, &fragFile, &state->fragModule)); 

    shaders->vertCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaders->vertCI.flags = 0;
    shaders->vertCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaders->vertCI.pName = "main";
    shaders->vertCI.module = state->vertModule;

    shaders->fragCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaders->fragCI.flags = 0;
    shaders->fragCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaders->fragCI.pName = "main";
    shaders->fragCI.module = state->fragModule;

    return VK_SUCCESS;
}

VkResult createRenderPass(VKState* state) {

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = state->format;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    VkAttachmentReference colorAttachReference = {};
    colorAttachReference.attachment = 0;
    colorAttachReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments = &colorAttachReference;
    subpass.colorAttachmentCount = 1;

    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    return vkCreateRenderPass(state->device, &renderPassCreateInfo, NULL, &state->renderPass);
}

VkResult createGraphicsPipeline(VKState* state) {

    Shaders shaders =  {};

    VK_CHECK(createShaders(state, &shaders));

    VkPipelineLayoutCreateInfo pLCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pLCreateInfo.flags = 0;
    pLCreateInfo.setLayoutCount = 0;
    pLCreateInfo.pSetLayouts = NULL;
    pLCreateInfo.pushConstantRangeCount = 0;
    pLCreateInfo.pPushConstantRanges = NULL;

    VK_CHECK(vkCreatePipelineLayout(state->device, &pLCreateInfo, NULL, &state->pipelineLayout));

    // NOTE: Vertex Buffer
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bindingDescription.stride = sizeof(Vertex);

    VkVertexInputAttributeDescription attributeDescriptions[2] = {};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = _offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = _offsetof(Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertexInputInfo.flags = 0;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssemblyInfo.flags = 0;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    state->viewport.x = 0.0f;
    state->viewport.y = 0.0f;
    state->viewport.maxDepth = 1.0f;
    state->viewport.minDepth = 0.0f;
    state->viewport.width = state->swapchainExtent.width;
    state->viewport.height = state->swapchainExtent.height;

    state->scissor.extent = state->swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &state->viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &state->scissor;

    VkPipelineRasterizationStateCreateInfo rasterizerInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizerInfo.lineWidth = 1.0f;
    rasterizerInfo.depthBiasEnable = VK_FALSE;
    rasterizerInfo.depthClampEnable = VK_FALSE;
    rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;

    VkPipelineMultisampleStateCreateInfo multisampleInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo.alphaToOneEnable = VK_FALSE;
    multisampleInfo.sampleShadingEnable = VK_FALSE;
    multisampleInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleInfo.pSampleMask = NULL;
    multisampleInfo.minSampleShading = 1.0f;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo colorBlendInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &colorBlendAttachment;
    colorBlendInfo.logicOpEnable = VK_FALSE;

    VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicStateInfo.dynamicStateCount = 2;
    dynamicStateInfo.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo GPCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    GPCreateInfo.flags = 0;
    GPCreateInfo.layout = state->pipelineLayout;
    GPCreateInfo.renderPass = state->renderPass;
    GPCreateInfo.subpass = 0;
    GPCreateInfo.stageCount = 2;
    GPCreateInfo.pStages = (void*) &shaders;
    GPCreateInfo.pVertexInputState = &vertexInputInfo;
    GPCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
    GPCreateInfo.pViewportState = &viewportInfo;
    GPCreateInfo.pRasterizationState = &rasterizerInfo;
    GPCreateInfo.pMultisampleState = &multisampleInfo;
    GPCreateInfo.pColorBlendState = &colorBlendInfo;
    GPCreateInfo.pDynamicState = &dynamicStateInfo;

    GPCreateInfo.pTessellationState = NULL;
    GPCreateInfo.pDepthStencilState = NULL;
    GPCreateInfo.basePipelineHandle = NULL;
    GPCreateInfo.basePipelineIndex = 0;

    return vkCreateGraphicsPipelines(state->device, NULL, 1, &GPCreateInfo, NULL, &state->graphicsPipeline);
}

VkResult createFramebuffers(VKState* state) {
    
    state->framebuffers = calloc(state->imageCount, sizeof(VkFramebuffer));

    for (int i=0; i<state->imageCount; i++) {

        VkImageView attachments[] = { state->imageViews[i] };

        VkFramebufferCreateInfo framebufferCreateInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.layers = 1;
        framebufferCreateInfo.renderPass = state->renderPass;
        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = state->swapchainExtent.width;
        framebufferCreateInfo.height = state->swapchainExtent.height;

        VK_CHECK(vkCreateFramebuffer(state->device, &framebufferCreateInfo, NULL, &state->framebuffers[i]));
    }

    return VK_SUCCESS;
}

VkResult createCommandPool(VKState* state) {
    VkCommandPoolCreateInfo commandPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    commandPoolCreateInfo.queueFamilyIndex = state->QFIndex;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    return vkCreateCommandPool(state->device, &commandPoolCreateInfo, NULL, &state->commandPool);
}

VkResult allocateMemory(
    VkDevice device,
    VkPhysicalDeviceMemoryProperties memoryProps,
    VkMemoryPropertyFlags properties, 
    VkBuffer buffer, 
    VkDeviceMemory* memory) {

    VkMemoryRequirements memoryReqs;
    vkGetBufferMemoryRequirements(device, buffer, &memoryReqs);

    uint32 memTypeIndex = 0;

    for (int i=0; i<memoryProps.memoryTypeCount; i++) {
        if ((memoryReqs.memoryTypeBits & (1 << i)) && 
        (memoryProps.memoryTypes[i].propertyFlags & properties) == properties) {
            memTypeIndex = i;
        }
    }

    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.memoryTypeIndex = memTypeIndex;
    allocInfo.allocationSize = memoryReqs.size;

    return vkAllocateMemory(device, &allocInfo, NULL, memory);
}

VkResult createVertexBuffer(VKState* state) {
    
    VkBufferCreateInfo bufferInfo = {  VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.flags = 0;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.pQueueFamilyIndices = NULL;
    bufferInfo.queueFamilyIndexCount = 0;
    bufferInfo.size = 3 * sizeof(Vertex);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    Vertex vertices[3] = {
        {{0.0f,  -0.5f},{1.0f, 0.0f, 0.0f}},
        {{0.5f,  0.5f},{0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f},{0.0f, 0.0f, 1.0f}}
    };

    vkCreateBuffer(state->device, &bufferInfo, NULL, &state->vertexBuffer);

    VK_CHECK(allocateMemory(
        state->device,
        state->memoryProps, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        state->vertexBuffer, 
        &state->vertexBufferMemory
        ));
        
    vkBindBufferMemory(state->device, state->vertexBuffer, state->vertexBufferMemory, 0);

    void* data;

    vkMapMemory(state->device, state->vertexBufferMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, vertices, bufferInfo.size);
    vkUnmapMemory(state->device, state->vertexBufferMemory);

    data = NULL;

    return VK_SUCCESS;
}

VkResult allocateCommandBuffers(VKState* state) {

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    commandBufferAllocateInfo.commandPool = state->commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.pNext = NULL;
    commandBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

    return vkAllocateCommandBuffers(state->device, &commandBufferAllocateInfo, state->commandBuffers);
}

VkResult recordCommandBuffer(VKState* state, uint32 imageIndex) {
    
    VkCommandBufferBeginInfo cmdBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    cmdBufferBeginInfo.flags = 0;
    cmdBufferBeginInfo.pInheritanceInfo = NULL;
    cmdBufferBeginInfo.pNext = NULL;

    vkBeginCommandBuffer(state->commandBuffers[state->currentFrame], &cmdBufferBeginInfo);

    VkClearValue clearValue = {{{0.0f,0.0f,0.0f,1.0f}}};

    VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassBeginInfo.framebuffer = state->framebuffers[imageIndex];
    renderPassBeginInfo.renderPass = state->renderPass;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;
    renderPassBeginInfo.renderArea.extent = state->swapchainExtent;
    renderPassBeginInfo.renderArea.offset = (VkOffset2D){0, 0};
    
    vkCmdBeginRenderPass(state->commandBuffers[state->currentFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(state->commandBuffers[state->currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, state->graphicsPipeline);

    VkBuffer vertexBuffers[] = { state->vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(state->commandBuffers[state->currentFrame], 0, 1, vertexBuffers, offsets);

    vkCmdSetViewport(state->commandBuffers[state->currentFrame], 0, 1, &state->viewport);
    vkCmdSetScissor(state->commandBuffers[state->currentFrame], 0, 1, &state->scissor);
    vkCmdDraw(state->commandBuffers[state->currentFrame], 3, 1, 0, 0);
    vkCmdEndRenderPass(state->commandBuffers[state->currentFrame]);

    return vkEndCommandBuffer(state->commandBuffers[state->currentFrame]);
}

VkResult createSyncObjects(VKState* state) {
    VkSemaphoreCreateInfo smCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) {
        VK_CHECK(vkCreateFence(state->device, &fenceCreateInfo, NULL, &state->fences[i]));
        VK_CHECK(vkCreateSemaphore(state->device, &smCreateInfo, NULL, &state->sm1[i]));
        VK_CHECK(vkCreateSemaphore(state->device, &smCreateInfo, NULL, &state->sm2[i]));
    }

    return VK_SUCCESS; 
}

void cleanUpSwapchain(VKState* state) {

    for (int i=0 ; i<state->imageCount ; i++) {
        vkDestroyFramebuffer(state->device, state->framebuffers[i], NULL);
    }

    FREE(state->framebuffers);

    for (int i=0 ; i<state->imageCount ; i++) {
        vkDestroyImageView(state->device, state->imageViews[i], NULL);
    }

    FREE(state->imageViews);
    FREE(state->images);

    vkDestroySwapchainKHR(state->device, state->swapchain, NULL);
}

VkResult recreateSwapchain(VKState* state) {
    vkDeviceWaitIdle(state->device);
    cleanUpSwapchain(state);

    state->surfaceExtent.width = 0;
    state->surfaceExtent.height = 0;
    glfwGetFramebufferSize(state->window, (int*)(&state->surfaceExtent.width), (int*)(&state->surfaceExtent.height));

    while (state->surfaceExtent.width == 0 || state->surfaceExtent.height == 0) {
        glfwGetFramebufferSize(state->window, (int*)(&state->surfaceExtent.width), (int*)(&state->surfaceExtent.height));
        glfwWaitEvents();
    }
    
    vkDeviceWaitIdle(state->device);

    VK_CHECK(setupSwapchainKHR(state));
    VK_CHECK(createFramebuffers(state));
    return VK_SUCCESS;
}

void Draw(VKState* state) {
    vkWaitForFences(state->device, 1, &state->fences[state->currentFrame], VK_TRUE, UINT64_MAX);

    uint32 imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(state->device, state->swapchain, UINT64_MAX, state->sm1[state->currentFrame], VK_NULL_HANDLE, &imageIndex);

    if ( result == VK_ERROR_OUT_OF_DATE_KHR ) {
        recreateSwapchain(state);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR ) {
        VK_CHECK(0);
    }

    vkResetFences(state->device, 1, &state->fences[state->currentFrame]);

    vkResetCommandBuffer(state->commandBuffers[state->currentFrame], 0);
    recordCommandBuffer(state, imageIndex);

    VkSemaphore waitSemaphores[] = { state->sm1[state->currentFrame] }; 
    VkSemaphore signalSemaphores[] = { state->sm2[state->currentFrame] }; 
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &state->commandBuffers[state->currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    vkQueueSubmit(state->deviceQueue, 1, &submitInfo, state->fences[state->currentFrame]);

    VkSwapchainKHR swapchains[] = { state->swapchain };

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pSwapchains = swapchains;
    presentInfo.swapchainCount = 1; 
    presentInfo.pResults = NULL;

    result = vkQueuePresentKHR(state->deviceQueue, &presentInfo);

    if ( result == VK_ERROR_OUT_OF_DATE_KHR || 
        result == VK_SUBOPTIMAL_KHR || 
        state->framebufferResized == VK_TRUE 
        ) {

        state->framebufferResized = VK_FALSE;
        recreateSwapchain(state);

    } else if (result != VK_SUCCESS) {
        VK_CHECK(0);
    }

    state->currentFrame = (state->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

int main(void)
{

    VKState VKS = {};
    VKS.width = WIDTH;
    VKS.height = HEIGHT;

    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    VKS.window = glfwCreateWindow(VKS.width, VKS.height, "VkProject", NULL, NULL);
    glfwSetWindowUserPointer(VKS.window, &VKS);
    glfwSetFramebufferSizeCallback(VKS.window, framebufferResizeCallback);

    if (!VKS.window)
    {
        glfwTerminate();
        return -1;
    }
    
    VK_CHECK(createInstance(&VKS));
    selectGPU(&VKS);
    VK_CHECK(createDevice(&VKS));
    VK_CHECK(setupSwapchainKHR(&VKS));
    VK_CHECK(createRenderPass(&VKS));
    VK_CHECK(createGraphicsPipeline(&VKS));
    VK_CHECK(createFramebuffers(&VKS));
    VK_CHECK(createVertexBuffer(&VKS));
    VK_CHECK(createCommandPool(&VKS));
    VK_CHECK(allocateCommandBuffers(&VKS));
    VK_CHECK(createSyncObjects(&VKS));

    /* Make the window's context current */
    glfwMakeContextCurrent(VKS.window);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(VKS.window))
    {
        glfwPollEvents();
        Draw(&VKS);
    }
    vkDeviceWaitIdle(VKS.device);

    cleanUpSwapchain(&VKS);

    for (int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(VKS.device, VKS.sm2[i], NULL);
        vkDestroySemaphore(VKS.device, VKS.sm1[i], NULL);
        vkDestroyFence(VKS.device, VKS.fences[i], NULL);
    }
    
    vkDestroyCommandPool(VKS.device, VKS.commandPool, NULL);

    vkDestroyBuffer(VKS.device, VKS.vertexBuffer, NULL);
    vkFreeMemory(VKS.device, VKS.vertexBufferMemory, NULL);

    vkDestroyPipeline(VKS.device, VKS.graphicsPipeline, NULL);

    vkDestroyPipelineLayout(VKS.device, VKS.pipelineLayout, NULL);

    vkDestroyRenderPass(VKS.device, VKS.renderPass, NULL);

    vkDestroyShaderModule(VKS.device, VKS.fragModule, NULL);

    vkDestroyShaderModule(VKS.device, VKS.vertModule, NULL);

    vkDestroyDevice(VKS.device, NULL);

    vkDestroySurfaceKHR(VKS.instance, VKS.surface, NULL);

    PFN_vkDestroyDebugUtilsMessengerEXT pVkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(VKS.instance, "vkDestroyDebugUtilsMessengerEXT");
    pVkDestroyDebugUtilsMessengerEXT(VKS.instance, VKS.debugMessenger, NULL);

    vkDestroyInstance(VKS.instance, NULL);

    glfwTerminate();
    return 0;
}
