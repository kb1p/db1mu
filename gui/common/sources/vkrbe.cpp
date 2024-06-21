#include "vkrbe.h"
#include <log.h>
#include <stdexcept>
#include <limits>

using namespace std;

inline void CHECK(VkResult vr, const char *msg)
{
    if (vr != VK_SUCCESS)
        throw runtime_error { msg };
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderingBackend::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                                     VkDebugUtilsMessageTypeFlagsEXT type,
                                                                     const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                                     void *pThis)
{
    // auto zis = static_cast<VulkanRenderingBackend*>(pThis);
    auto level = Log::LVL_DEBUG;
    switch (severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            level = Log::LVL_ERROR;
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            level = Log::LVL_WARNING;
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            level = Log::LVL_INFO;
            break;
    }

    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        Log::instance().print(level, "[Vulkan renderer] %s", pCallbackData->pMessage);

    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance inst,
                                      const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                      const VkAllocationCallbacks *pAllocator,
                                      VkDebugUtilsMessengerEXT *pDebugMessenger) noexcept
{
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(inst, "vkCreateDebugUtilsMessengerEXT"));
    return func ? func(inst, pCreateInfo, pAllocator, pDebugMessenger) : VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance inst,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) noexcept
{
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(inst, "vkDestroyDebugUtilsMessengerEXT"));
    if (func)
        func(inst, debugMessenger, pAllocator);
}

template <typename T>
T clamp(T val, const T minBound, const T maxBound) noexcept
{
    assert(minBound < maxBound);

    const T s = maxBound - minBound;
    if (val < minBound)
    {
        const T d = minBound - val;
        val += (d / s + (d % s != 0 ? 1 : 0)) * s;
    }
    if (val > maxBound)
    {
        const T d = val - maxBound;
        val -= (d / s + (d % s != 0 ? 1 : 0)) * s;
    }

    return val;
}

static const char *VALIDATION_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation"
};

static constexpr uint32_t VALIDATION_LAYERS_COUNT = sizeof(VALIDATION_LAYERS) / sizeof(VALIDATION_LAYERS[0]);

static const char *DEVICE_EXTENSIONS[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static constexpr uint32_t DEVICE_EXTENSIONS_COUNT = sizeof(DEVICE_EXTENSIONS) / sizeof(DEVICE_EXTENSIONS[0]);

// Shader modules
static const uint32_t VERTEX_SHADER[] = {
#include "vkrbe_vert.spv.inc"
};

static const uint32_t FRAGMENT_SHADER[] = {
#include "vkrbe_frag.spv.inc"
};

#ifdef NDEBUG
    constexpr bool g_debug = false;
#else
    constexpr bool g_debug = true;
#endif

void VulkanRenderingBackend::Buffer::dispose(VkDevice dev)
{
    vkDestroyBuffer(dev, buffer, nullptr);
    vkFreeMemory(dev, memory, nullptr);
}

void VulkanRenderingBackend::Texture::dispose(VkDevice dev)
{
    vkDestroyImageView(dev, view, nullptr);
    vkDestroyImage(dev, image, nullptr);
    vkFreeMemory(dev, memory, nullptr);
}

void VulkanRenderingBackend::SampleableTexture::dispose(VkDevice dev)
{
    vkDestroySampler(dev, sampler, nullptr);
    Texture::dispose(dev);
}

void VulkanRenderingBackend::init(uint32_t nExts, const char **pExts)
{
    // Create instance
    const VkApplicationInfo appInfo = {
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "db1mu (NES emulator)",
        VK_MAKE_VERSION(1, 0, 0),
        "none",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_0
    };

    const VkDebugUtilsMessengerCreateInfoEXT dbgCreateInfo = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        nullptr,
        0,
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        &debugCallback,
        this
    };

    const VkInstanceCreateInfo instanceCreateInfo = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        g_debug ? &dbgCreateInfo : nullptr,
        0,
        &appInfo,
        g_debug ? VALIDATION_LAYERS_COUNT : 0u,
        g_debug ? VALIDATION_LAYERS : nullptr,
        nExts,
        pExts
    };

    CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &m_inst),
          "failed to create vulkan instance");

    if (g_debug)
    {
        CHECK(CreateDebugUtilsMessengerEXT(m_inst, &dbgCreateInfo, nullptr, &m_dbgMsgr),
              "failed to create debug messenger");
    }

    m_externalInstance = false;
}

void VulkanRenderingBackend::init(VkInstance inst)
{
    m_inst = inst;
    m_externalInstance = true;
}

void VulkanRenderingBackend::destroySwapchain()
{
    for (const auto &swapChainItm: m_swapChainData)
    {
        vkDestroyFramebuffer(m_dev, swapChainItm.fb, nullptr);
#ifdef USE_IMGUI
        vkDestroyFramebuffer(m_dev, swapChainItm.fbIG, nullptr);
#endif
        vkDestroyImageView(m_dev, swapChainItm.view, nullptr);
    }
    vkDestroySwapchainKHR(m_dev, m_swapChain, nullptr);
}

void VulkanRenderingBackend::release()
{
    CHECK(vkDeviceWaitIdle(m_dev), "failed to wait device idle at shutdown");

    for (int i = 0; i < MAX_FIF; i++)
    {
        vkDestroySemaphore(m_dev, m_semsImageAvailable[i], nullptr);
        vkDestroySemaphore(m_dev, m_semsRenderFinished[i], nullptr);
        vkDestroyFence(m_dev, m_fncsInFlight[i], nullptr);
    }
    destroySwapchain();
    m_texture.dispose(m_dev);
    m_texStgBuf.dispose(m_dev);
    vkDestroyCommandPool(m_dev, m_cmdTmpPool, nullptr);
    vkDestroyCommandPool(m_dev, m_cmdPool, nullptr);
    vkDestroyPipeline(m_dev, m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_dev, m_pipelineLayout, nullptr);
    vkDestroyDescriptorPool(m_dev, m_descPool, nullptr);
    vkDestroyDescriptorSetLayout(m_dev, m_descrSetLayout, nullptr);
    vkDestroyRenderPass(m_dev, m_renderPass, nullptr);

#ifdef USE_IMGUI
    vkDestroyCommandPool(m_dev, m_cmdPoolIG, nullptr);
    vkDestroyDescriptorPool(m_dev, m_descPoolIG, nullptr);
    vkDestroyRenderPass(m_dev, m_renderPassIG, nullptr);
#endif

    vkDestroyDevice(m_dev, nullptr);
    if (m_dbgMsgr != VK_NULL_HANDLE)
        DestroyDebugUtilsMessengerEXT(m_inst, m_dbgMsgr, nullptr);

    if (!m_externalInstance)
    {
        vkDestroySurfaceKHR(m_inst, m_surface, nullptr);
        vkDestroyInstance(m_inst, nullptr);
    }
}

void VulkanRenderingBackend::setupOutput(VkSurfaceKHR surf)
{
    if (m_width <= 0 || m_height <= 0)
        throw runtime_error { "window size must be set prior to output surface" };

    m_surface = surf;
    assert(m_surface != VK_NULL_HANDLE);

    // Pick proper physical device
    vector<VkPhysicalDevice> physDevs;
    uint32_t physDevCount = 0;
    CHECK(vkEnumeratePhysicalDevices(m_inst, &physDevCount, nullptr),
          "failed to get physical devices count");
    physDevs.resize(physDevCount);
    CHECK(vkEnumeratePhysicalDevices(m_inst, &physDevCount, physDevs.data()),
          "failed to enumerate physical devices");

    vector<VkExtensionProperties> availableExts;
    vector<VkSurfaceFormatKHR> surfFmts;
    vector<VkPresentModeKHR> presentModes;
    for (const auto devCand: physDevs)
    {
        vkGetPhysicalDeviceProperties(devCand, &m_physDevProps);
        vkGetPhysicalDeviceFeatures(devCand, &m_physDevFeats);

        const bool typeMatch = m_physDevProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
                               m_physDevProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
        if (typeMatch)
        {
            // Check the swapchain extension is supported by this device
            bool reqExtsSupported = false;
            uint32_t extCount = 0;
            vkEnumerateDeviceExtensionProperties(devCand, nullptr, &extCount, nullptr);
            availableExts.resize(extCount);
            vkEnumerateDeviceExtensionProperties(devCand, nullptr, &extCount, availableExts.data());
            for (auto it = availableExts.begin(); !reqExtsSupported && it != availableExts.end(); ++it)
                for (auto itr = begin(DEVICE_EXTENSIONS); itr != end(DEVICE_EXTENSIONS); ++itr)
                    if (strcmp(it->extensionName, *itr) == 0)
                    {
                        reqExtsSupported = true;
                        break;
                    }
            if (reqExtsSupported)
            {
                // Check swap chain supported formats and presentation modes
                CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devCand, m_surface, &m_surfaceCaps),
                      "failed to check surface capabilities");
                uint32_t n = 0;
                CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(devCand, m_surface, &n, nullptr),
                      "failed to get number of formats supported by surface");
                if (n > 0)
                {
                    surfFmts.resize(n);
                    CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(devCand, m_surface, &n, surfFmts.data()),
                          "failed to enumerate formats supported by surface");
                }
                CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(devCand, m_surface, &n, nullptr),
                      "failed to get number of presentation modes supported by surface");
                if (n > 0)
                {
                    presentModes.resize(n);
                    CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(devCand, m_surface, &n, presentModes.data()),
                          "failed to enumerate presentation modes supported by surface");
                }

                // It's enough if there are at least one supported presentation mode and one format
                const bool match = !surfFmts.empty() && !presentModes.empty();
                if (match)
                {
                    m_physDev = devCand;
                    Log::i("[Vulkan] Using device %s", m_physDevProps.deviceName);
                    break;
                }
            }
        }
    }
    if (m_physDev == VK_NULL_HANDLE)
        throw runtime_error { "no suitable GPU device found" };

    // Find queue family that supports graphic commands.
    // Store queue family props to class field because we'll need it to create presentation queue.
    std::vector<VkQueueFamilyProperties> queueFamPrps;
    uint32_t qfCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physDev, &qfCount, nullptr);
    queueFamPrps.resize(qfCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physDev, &qfCount, queueFamPrps.data());
    Maybe<uint32_t> qfIndexGraphical,
                    qfIndexPres;
    for (uint32_t i = 0; i < queueFamPrps.size(); i++)
    {
        if (qfIndexGraphical.isNothing() && (queueFamPrps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            qfIndexGraphical = i;
        if (qfIndexPres.isNothing())
        {
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_physDev, i, m_surface, &presentSupport);
            if (presentSupport == VK_TRUE)
                qfIndexPres = i;
        }
        if (!qfIndexGraphical.isNothing() && !qfIndexPres.isNothing())
            break;
    }
    if (qfIndexGraphical.isNothing())
        throw runtime_error { "no queue family that support graphical commands found" };
    if (qfIndexPres.isNothing())
        throw runtime_error { "no queue family that support presentation to window surface found" };

    m_qfIndexGraphical = qfIndexGraphical.value();
    m_qfIndexPresentation = qfIndexPres.value();

    // Create logical device and queues for rendering and presentation
    const float queuePriority = 1.0f;
    const VkDeviceQueueCreateInfo queueCreateInfos[] = {
        {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            nullptr,
            0,
            m_qfIndexGraphical,
            1,
            &queuePriority
        },
        {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            nullptr,
            0,
            m_qfIndexPresentation,
            1,
            &queuePriority
        }
    };
    VkPhysicalDeviceFeatures reqDevFeatures { };
    reqDevFeatures.samplerAnisotropy = m_physDevFeats.samplerAnisotropy;
    reqDevFeatures.sampleRateShading = VK_TRUE;
    const VkDeviceCreateInfo devCreateInfo = {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        nullptr,
        0,
        qfIndexGraphical == qfIndexPres ? 1u : 2u, // if rendering and presentation is supported by same family, we use only it
        queueCreateInfos,
        g_debug ? VALIDATION_LAYERS_COUNT : 0u,
        g_debug ? VALIDATION_LAYERS : nullptr,
        DEVICE_EXTENSIONS_COUNT,
        DEVICE_EXTENSIONS,
        &reqDevFeatures
    };
    CHECK(vkCreateDevice(m_physDev, &devCreateInfo, nullptr, &m_dev),
          "failed to create logical device");

    vkGetDeviceQueue(m_dev, m_qfIndexGraphical, 0, &m_renderQueue);
    if (m_renderQueue == VK_NULL_HANDLE)
        throw runtime_error { "failed to obtain queue that supports rendering commands from device" };
    vkGetDeviceQueue(m_dev, m_qfIndexPresentation, 0, &m_presentationQueue);
    if (m_presentationQueue == VK_NULL_HANDLE)
        throw runtime_error { "failed to obtain queue that supports presentation from device" };

    // Pick preferrable format and presentaton mode
    m_surfaceFormat = surfFmts[0];
    for (const auto &fmt: surfFmts)
        if (fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
            fmt.format == VK_FORMAT_B8G8R8A8_UNORM)
        {
            m_surfaceFormat = fmt;
            break;
        }
    m_presentationMode = VK_PRESENT_MODE_FIFO_KHR; // always supported
    for (const auto &mode: presentModes)
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            m_presentationMode = mode;
            break;
        }

    // Create renderpass
    {
        const VkAttachmentDescription attachDescrs[] = {
            // Color attachment
            {
                0,                                          // flags
                m_surfaceFormat.format,                     // format
                VK_SAMPLE_COUNT_1_BIT,                      // samples
                VK_ATTACHMENT_LOAD_OP_CLEAR,                // load op
                VK_ATTACHMENT_STORE_OP_STORE,               // store op
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,            // stencil load op
                VK_ATTACHMENT_STORE_OP_DONT_CARE,           // stencil store op
                VK_IMAGE_LAYOUT_UNDEFINED,                  // initial layout
#ifdef USE_IMGUI
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL    // final layout: will be painted over by ImGUI render pass
#else
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR             // final layout: presenting to the swap chain
#endif
            }
        };
        const VkAttachmentReference clrAttachRef = {
            0,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };
        const VkSubpassDescription subpass = {
            0,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            0,
            nullptr,
            1,
            &clrAttachRef,
            nullptr,
            nullptr,
            0,
            nullptr
        };
        const VkSubpassDependency rendPassExtDep = {
            VK_SUBPASS_EXTERNAL,
            0,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
        };
        const VkRenderPassCreateInfo passInfo = {
            VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            nullptr,
            0,
            sizeof(attachDescrs) / sizeof(VkAttachmentDescription),
            attachDescrs,
            1,
            &subpass,
            1,
            &rendPassExtDep
        };
        CHECK(vkCreateRenderPass(m_dev, &passInfo, nullptr, &m_renderPass),
            "failed to create render pass");
    }

#ifdef USE_IMGUI
    // Create additional renderpass for ImGUI rendering
    {
        VkAttachmentDescription attDescr = { };
        attDescr.format = m_surfaceFormat.format;
        attDescr.samples = VK_SAMPLE_COUNT_1_BIT;
        attDescr.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attDescr.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attDescr.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attDescr.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attDescr.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attDescr.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference attDescrRef = { };
        attDescrRef.attachment = 0;
        attDescrRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &attDescrRef;

        VkSubpassDependency spDep = { };
        spDep.srcSubpass = VK_SUBPASS_EXTERNAL;
        spDep.dstSubpass = 0;
        spDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        spDep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        spDep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        spDep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo info = { };
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &attDescr;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &spDep;
        CHECK(vkCreateRenderPass(m_dev, &info, nullptr, &m_renderPassIG),
              "[ImGUI] failed to create render pass");
    }
#endif

    // Create shader modules
    VkShaderModule smVert, smFrag;
    VkShaderModuleCreateInfo shaderCreateInfo = {
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        nullptr,
        0,
        sizeof(VERTEX_SHADER),
        VERTEX_SHADER
    };
    CHECK(vkCreateShaderModule(m_dev, &shaderCreateInfo, nullptr, &smVert),
          "failed to create vertex shader module");
    shaderCreateInfo.codeSize = sizeof(FRAGMENT_SHADER);
    shaderCreateInfo.pCode = FRAGMENT_SHADER;
    CHECK(vkCreateShaderModule(m_dev, &shaderCreateInfo, nullptr, &smFrag),
          "failed to create fragment shader module");

    // Create descriptor set layouts for uniforms
    const VkDescriptorSetLayoutBinding bindings[] = {
        {
            0,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            1,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            nullptr
        }
    };
    const VkDescriptorSetLayoutCreateInfo dslInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding),          // bindings count
        bindings
    };
    CHECK(vkCreateDescriptorSetLayout(m_dev, &dslInfo, nullptr, &m_descrSetLayout),
          "failed to create descriptor set layout");

    // Create pipeline layout
    const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        1,
        &m_descrSetLayout,
        0,                  // push constants range count
        nullptr             // push constants range array
    };
    CHECK(vkCreatePipelineLayout(m_dev, &pipelineLayoutInfo, nullptr, &m_pipelineLayout),
          "failed to create pipeline layout");

    // Create pipeline
    // |- Shader stages
    const VkPipelineShaderStageCreateInfo shaderStages[] = {
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_VERTEX_BIT,
            smVert,
            "main"
        },
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            smFrag,
            "main"
        }
    };

    // |- Dynamic states
    const VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    const VkPipelineDynamicStateCreateInfo dynamicStatesInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        nullptr,
        0,
        2,
        dynamicStates
    };

    // |- Vertex input -- not used (vertex attributes are embedded into shader)
    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        nullptr,
        0,
        0,
        nullptr,
        0,
        nullptr
    };

    // |- Input assembly
    const VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        nullptr,
        0,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VK_FALSE
    };

    // |- Viewport state
    const VkPipelineViewportStateCreateInfo viewportStateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        nullptr,
        0,
        1,
        nullptr,
        1,
        nullptr
    };

    // |- Rasterizer
    const VkPipelineRasterizationStateCreateInfo rasterizationStateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        nullptr,
        0,
        VK_FALSE,
        VK_FALSE,
        VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_BACK_BIT,
        VK_FRONT_FACE_COUNTER_CLOCKWISE,
        VK_FALSE,
        0.0f,
        0.0f,
        0.0f,
        1.0f
    };

    // |- Multisampling
    const VkPipelineMultisampleStateCreateInfo multisamplingStateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        nullptr,
        0,
        VK_SAMPLE_COUNT_1_BIT,
        VK_FALSE,        // disable sample shading
        1.0f,            // min. sample shading fraction
        nullptr,
        VK_FALSE,
        VK_FALSE
    };

    // |- Blending
    const VkPipelineColorBlendAttachmentState blendAttachmentState = {
        VK_FALSE,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    const VkPipelineColorBlendStateCreateInfo blendStateInfo = {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        nullptr,
        0,
        VK_FALSE,
        VK_LOGIC_OP_COPY,
        1,
        &blendAttachmentState,
        { 0.0f, 0.0f, 0.0f, 0.0f }
    };

    // Depth/stencil state
    const VkPipelineDepthStencilStateCreateInfo dsState = {
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        nullptr,
        0,
        VK_FALSE,            // depth test enable
        VK_FALSE,            // depth write enable
        VK_COMPARE_OP_LESS, // depth compare op
        VK_FALSE,           // depth bounds test
        VK_FALSE,           // stencil test
        { },                // front op state
        { },                // back op state
        0.0f,               // min depth
        1.0f                // max depth
    };

    // Pipeline itself
    const VkGraphicsPipelineCreateInfo pipelineInfo = {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        nullptr,
        0,
        2,
        shaderStages,
        &vertexInputInfo,
        &inputAssemblyInfo,
        nullptr,                // no tesselation
        &viewportStateInfo,
        &rasterizationStateInfo,
        &multisamplingStateInfo,
        &dsState,
        &blendStateInfo,
        &dynamicStatesInfo,
        m_pipelineLayout,
        m_renderPass,
        0,
        VK_NULL_HANDLE,
        -1
    };
    CHECK(vkCreateGraphicsPipelines(m_dev, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline),
          "failed to create graphical pipeline");

    // Destroy shader modules (they are already baked into pipeline).
    vkDestroyShaderModule(m_dev, smVert, nullptr);
    vkDestroyShaderModule(m_dev, smFrag, nullptr);

    // Create command pool (long-living, resettable buffers)
    const VkCommandPoolCreateInfo cmdPoolInfo = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        m_qfIndexGraphical
    };
    CHECK(vkCreateCommandPool(m_dev, &cmdPoolInfo, nullptr, &m_cmdPool),
          "failed to create command pool");

    // Another command pool for short-living temporary buffers, for copy operations.
    const VkCommandPoolCreateInfo cmdPoolTmpInfo = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        m_qfIndexGraphical
    };
    CHECK(vkCreateCommandPool(m_dev, &cmdPoolTmpInfo, nullptr, &m_cmdTmpPool),
          "failed to create command pool for transient buffers");

    // Allocate command buffers
    const VkCommandBufferAllocateInfo cmdBufInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        m_cmdPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        MAX_FIF
    };
    CHECK(vkAllocateCommandBuffers(m_dev, &cmdBufInfo, m_cmdBufs),
          "failed to allocate command buffers");

#ifdef USE_IMGUI
    // Command pool for ImGUI and allocate render command buffers from it
    const VkCommandPoolCreateInfo cmdPoolIGInfo = {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        m_qfIndexGraphical
    };
    CHECK(vkCreateCommandPool(m_dev, &cmdPoolIGInfo, nullptr, &m_cmdPoolIG),
          "[ImGUI] failed to create command pool");

    const VkCommandBufferAllocateInfo cmdBufIGInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        m_cmdPoolIG,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        MAX_FIF
    };
    CHECK(vkAllocateCommandBuffers(m_dev, &cmdBufIGInfo, m_cmdBufsIG),
          "[ImGUI] failed to allocate command buffers");
#endif

    // Create descriptor set pool
    const VkDescriptorPoolSize dsPoolSizes[] = {
        {
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            MAX_FIF
        }
    };
    constexpr auto numPoolSizes = sizeof(dsPoolSizes) / sizeof(VkDescriptorPoolSize);
    const VkDescriptorPoolCreateInfo dsPoolInf = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        nullptr,
        0,
        MAX_FIF, // max sets
        numPoolSizes,
        dsPoolSizes
    };
    CHECK(vkCreateDescriptorPool(m_dev, &dsPoolInf, nullptr, &m_descPool),
          "failed to create descriptor pool for emulator");

#ifdef USE_IMGUI
    const VkDescriptorPoolSize dsPoolSizesIG[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    constexpr auto numPoolSizesIG = sizeof(dsPoolSizesIG) / sizeof(VkDescriptorPoolSize);
    const VkDescriptorPoolCreateInfo dsPoolInfIG = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        nullptr,
        0,
        1000 * numPoolSizesIG,
        numPoolSizesIG,
        dsPoolSizesIG
    };
    CHECK(vkCreateDescriptorPool(m_dev, &dsPoolInfIG, nullptr, &m_descPoolIG),
          "failed to create secodary descriptor pool for ImGUI");
#endif

    // Allocate descriptor sets
    VkDescriptorSetLayout dsLayouts[MAX_FIF];
    for (int i = 0; i < MAX_FIF; i++)
        dsLayouts[i] = m_descrSetLayout;

    const VkDescriptorSetAllocateInfo dsAllocInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        nullptr,
        m_descPool,
        MAX_FIF,    // descriptor set count
        dsLayouts
    };
    CHECK(vkAllocateDescriptorSets(m_dev, &dsAllocInfo, m_ufmDescSets),
          "failed to allocate descriptor sets");

    // Create synchronization objects
    const VkSemaphoreCreateInfo semInfo = {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    const VkFenceCreateInfo fncInfo = {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        nullptr,
        VK_FENCE_CREATE_SIGNALED_BIT
    };

    // Prepare texture object that will be filled from PPU, mapped to quad and displayed into output surface
    prepareTexture();

    // Per-frame data
    for (int i = 0; i < MAX_FIF; i++)
    {
        // Create synchronization primitives
        const auto r1 = vkCreateSemaphore(m_dev, &semInfo, nullptr, &m_semsImageAvailable[i]);
        const auto r2 = vkCreateSemaphore(m_dev, &semInfo, nullptr, &m_semsRenderFinished[i]);
        const auto r3 =  vkCreateFence(m_dev, &fncInfo, nullptr, &m_fncsInFlight[i]);
        if (r1 != VK_SUCCESS || r2 != VK_SUCCESS || r3 != VK_SUCCESS)
            throw runtime_error { "failed to create per-frame synchronization primitives" };

        // Bind uniforms to pipeline
        const VkDescriptorImageInfo texInfo = {
            m_texture.sampler,
            m_texture.view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };
        const VkWriteDescriptorSet writeInfos[] = {
            {
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                nullptr,
                m_ufmDescSets[i],
                0,
                0,
                1,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                &texInfo,                           // ptr to image info
                nullptr,
                nullptr
            }
        };
        vkUpdateDescriptorSets(m_dev, sizeof(writeInfos) / sizeof(VkWriteDescriptorSet), writeInfos, 0, nullptr);
    }

    createSwapchain();
}

VulkanRenderingBackend::Buffer VulkanRenderingBackend::createBuffer(const VkDeviceSize size,
                                                                    const VkBufferUsageFlags usage,
                                                                    const VkMemoryPropertyFlags properties)
{
    Buffer buff;

    // Create buffer object
    const VkBufferCreateInfo bufInfo = {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        nullptr,
        0,
        size,
        usage,
        VK_SHARING_MODE_EXCLUSIVE
    };
    CHECK(vkCreateBuffer(m_dev, &bufInfo, nullptr, &buff.buffer),
          "failed to create buffer object");

    // Allocate and bind device memory to buffer object
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(m_dev, buff.buffer, &memReqs);
    const VkMemoryAllocateInfo memInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memReqs.size,
        findMemoryTypeIndex(memReqs.memoryTypeBits, properties)
    };
    CHECK(vkAllocateMemory(m_dev, &memInfo, nullptr, &buff.memory),
          "failed to allocate memory for vertex buffer object");
    CHECK(vkBindBufferMemory(m_dev, buff.buffer, buff.memory, 0),
          "failed to bind memory to VBO");

    return buff;
}

VulkanRenderingBackend::Texture VulkanRenderingBackend::createTexture(uint32_t width,
                                                                      uint32_t height,
                                                                      uint32_t nMipLevels,
                                                                      VkSampleCountFlagBits numSamples,
                                                                      VkFormat format,
                                                                      VkImageTiling tiling,
                                                                      VkImageUsageFlags usage,
                                                                      VkMemoryPropertyFlags props,
                                                                      VkImageAspectFlags aspect)
{
    Texture tex;
    const VkImageCreateInfo imgInf = {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr,
        0,
        VK_IMAGE_TYPE_2D,
        format,
        {
            width,
            height,
            1
        },
        nMipLevels,
        1,
        numSamples,
        tiling,
        usage,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        VK_IMAGE_LAYOUT_UNDEFINED
    };
    CHECK(vkCreateImage(m_dev, &imgInf, nullptr, &tex.image),
          "failed to create image");

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(m_dev, tex.image, &memReq);

    const VkMemoryAllocateInfo memInf = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memReq.size,
        findMemoryTypeIndex(memReq.memoryTypeBits, props)
    };
    CHECK(vkAllocateMemory(m_dev, &memInf, nullptr, &tex.memory),
          "failed to create memory");
    CHECK(vkBindImageMemory(m_dev, tex.image, tex.memory, 0),
          "failed to bind image memory");

    // Create image view
    tex.view = createSimple2DImgView(tex.image, format, aspect, nMipLevels);

    return tex;
}

uint32_t VulkanRenderingBackend::findMemoryTypeIndex(const uint32_t typeFilter, const VkMemoryPropertyFlags reqProps)
{
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_physDev, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
        if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & reqProps) == reqProps)
            return i;

    throw runtime_error { "required memory type is not supported by device" };
}

VkFormat VulkanRenderingBackend::findSupportedFormat(VkImageTiling tiling, VkFormatFeatureFlags features, initializer_list<VkFormat> fmts)
{
    for (const auto f: fmts)
    {
        VkFormatProperties fp;
        vkGetPhysicalDeviceFormatProperties(m_physDev, f, &fp);
        switch (tiling)
        {
            case VK_IMAGE_TILING_LINEAR:
                if ((fp.linearTilingFeatures & features) == features)
                    return f;
                break;
            case VK_IMAGE_TILING_OPTIMAL:
                if ((fp.optimalTilingFeatures & features) == features)
                    return f;
                break;
        }
    }

    throw runtime_error { "requested format not found" };
}

void VulkanRenderingBackend::copyBufferToImage(VkCommandBuffer cbuf, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    const VkBufferImageCopy cpyInf = {
        0,
        0,
        0,
        {
            VK_IMAGE_ASPECT_COLOR_BIT,
            0,
            0,
            1
        },
        { 0, 0, 0 },
        {
            width,
            height,
            1
        }
    };
    vkCmdCopyBufferToImage(cbuf, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cpyInf);
}

VkCommandBuffer VulkanRenderingBackend::beginTransientCmdBuf()
{
    // Allocate command buffers
    const VkCommandBufferAllocateInfo cmdBufInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        m_cmdTmpPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1
    };

    VkCommandBuffer tmpCmdBuf;
    CHECK(vkAllocateCommandBuffers(m_dev, &cmdBufInfo, &tmpCmdBuf),
          "failed to allocate transiend command buffer");

    const VkCommandBufferBeginInfo begInf = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    CHECK(vkBeginCommandBuffer(tmpCmdBuf, &begInf), "failed to begin transient buffer recording");

    return tmpCmdBuf;
}

void VulkanRenderingBackend::endTransientCmdBuf(VkCommandBuffer cbuf)
{
    CHECK(vkEndCommandBuffer(cbuf), "failed to end transient buffer recording");
    const VkSubmitInfo cmdSubmitInfo = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        0,
        nullptr,
        nullptr,
        1,
        &cbuf,
        0,
        nullptr
    };
    CHECK(vkQueueSubmit(m_renderQueue, 1, &cmdSubmitInfo, VK_NULL_HANDLE),
          "failed to submit transient buffer to queue");
    CHECK(vkQueueWaitIdle(m_renderQueue), "failed to wait transient buffer to finish");
    vkFreeCommandBuffers(m_dev, m_cmdTmpPool, 1, &cbuf);
}

void VulkanRenderingBackend::transitionImageLayout(VkCommandBuffer cbuf,
                                                   VkImage image,
                                                   VkFormat format,
                                                   VkImageLayout oldLayout,
                                                   VkImageLayout newLayout,
                                                   uint32_t nMipLevels)
{
    VkImageMemoryBarrier bar = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        0,  // srcAccessMask
        0,  // dstAccessMask
        oldLayout,
        newLayout,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image,
        {
            VK_IMAGE_ASPECT_COLOR_BIT,
            0,
            nMipLevels,
            0,
            1
        }
    };

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        bar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
            bar.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    } else
        bar.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VkPipelineStageFlags srcStage, dstStage;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        bar.srcAccessMask = 0;
        bar.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        bar.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        bar.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        bar.srcAccessMask = 0;
        bar.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
        throw invalid_argument { "unsupported layout transition!" };

    vkCmdPipelineBarrier(cbuf, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &bar);
}

VkImageView VulkanRenderingBackend::createSimple2DImgView(VkImage src, VkFormat fmt, VkImageAspectFlags aspect, uint32_t nMipLevels)
{
    const VkImageViewCreateInfo viewInf = {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        src,
        VK_IMAGE_VIEW_TYPE_2D,
        fmt,
        {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY
        },
        {
            aspect,
            0,
            nMipLevels,
            0,
            1
        }
    };

    VkImageView view;
    CHECK(vkCreateImageView(m_dev, &viewInf, nullptr, &view),
          "failed to create texture view");

    return view;
}

void VulkanRenderingBackend::prepareTexture()
{
    constexpr auto imageSize = TEX_WIDTH * TEX_HEIGHT * 4;
    constexpr auto mipLevels = 1u;

    m_texStgBuf = createBuffer(imageSize,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *tmp = nullptr;
    CHECK(vkMapMemory(m_dev, m_texStgBuf.memory, 0, VK_WHOLE_SIZE, 0, &tmp),
          "failed to map texture's staging buffer memory");
    m_pTexData = static_cast<decltype(m_pTexData)>(tmp);

    m_texture = createTexture(TEX_WIDTH, TEX_HEIGHT,
                              mipLevels,
                              VK_SAMPLE_COUNT_1_BIT,
                              VK_FORMAT_R8G8B8A8_UNORM,
                              VK_IMAGE_TILING_OPTIMAL,
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              VK_IMAGE_ASPECT_COLOR_BIT);

    // Create sampler
    const VkSamplerCreateInfo sampInf = {
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        nullptr,
        0,
        VK_FILTER_LINEAR,                           // mag filter
        VK_FILTER_LINEAR,                           // min filter
        VK_SAMPLER_MIPMAP_MODE_LINEAR,              // mipmap mode
        VK_SAMPLER_ADDRESS_MODE_REPEAT,             // addr. mode U
        VK_SAMPLER_ADDRESS_MODE_REPEAT,             // addr. mode V
        VK_SAMPLER_ADDRESS_MODE_REPEAT,             // addr. mode W
        0.0f,                                       // mip lod bias
        VK_FALSE,                                   // anisotropy: disable
        1.0f,                                       // max anisotropy
        VK_FALSE,                                   // compare enable
        VK_COMPARE_OP_ALWAYS,                       // compare op
        0.0f,                                       // min lod
        static_cast<float>(mipLevels),              // max lod
        VK_BORDER_COLOR_INT_OPAQUE_BLACK,           // border color
        VK_FALSE                                    // unnormalized coordinates
    };
    CHECK(vkCreateSampler(m_dev, &sampInf, nullptr, &m_texture.sampler),
          "failed to create sampler");
}

void VulkanRenderingBackend::createSwapchain()
{
    // Pick extent
    m_surfExtent = m_surfaceCaps.currentExtent;
    if (m_surfaceCaps.currentExtent.width == numeric_limits<uint32_t>::max())
    {
        m_surfExtent.width = ::clamp<uint32_t>(m_width, m_surfaceCaps.minImageExtent.width, m_surfaceCaps.maxImageExtent.width);
        m_surfExtent.height = ::clamp<uint32_t>(m_height, m_surfaceCaps.minImageExtent.height, m_surfaceCaps.maxImageExtent.height);
    }

    Log::d("[Vulkan] Surface extent: %dx%d", m_surfExtent.width, m_surfExtent.height);

    // Pick number of images in a swap chain
    m_imageCount = m_surfaceCaps.maxImageCount > 0 && m_surfaceCaps.maxImageCount == m_surfaceCaps.minImageCount ?
                   m_surfaceCaps.maxImageCount :
                   m_surfaceCaps.minImageCount + 1;
    m_imageCount = std::min<uint32_t>(m_imageCount, MAX_FIF);

    Log::d("[Vulkan] Number of images in a swap chain: %d", m_imageCount);

    const bool separatePresQueue = m_qfIndexGraphical != m_qfIndexPresentation;
    const uint32_t queueIndices[] = { m_qfIndexGraphical, m_qfIndexPresentation };
    const VkSwapchainCreateInfoKHR swapchainCreatInfo = {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        nullptr,
        0,
        m_surface,
        m_imageCount,
        m_surfaceFormat.format,
        m_surfaceFormat.colorSpace,
        m_surfExtent,
        1,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        separatePresQueue ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        separatePresQueue ? 2u : 0u,
        separatePresQueue ? queueIndices : nullptr,
        m_surfaceCaps.currentTransform,
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        m_presentationMode,
        VK_TRUE,
        VK_NULL_HANDLE
    };
    CHECK(vkCreateSwapchainKHR(m_dev, &swapchainCreatInfo, nullptr, &m_swapChain),
          "failed to create swap chain");

    // Retrieve swapchain images
    std::vector<VkImage> swapChainImages;
    uint32_t imgCount = 0;
    CHECK(vkGetSwapchainImagesKHR(m_dev, m_swapChain, &imgCount, nullptr),
          "failed to get image count from swap chain");
    swapChainImages.resize(imgCount);
    CHECK(vkGetSwapchainImagesKHR(m_dev, m_swapChain, &imgCount, swapChainImages.data()),
          "failed to enumerate image count from swap chain");

    // Create image views & framebuffers for each of swapchain images
    m_swapChainData.resize(imgCount);
    for (uint32_t i = 0; i < imgCount; i++)
    {
        auto &r = m_swapChainData[i];

        r.view = createSimple2DImgView(swapChainImages[i], m_surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);

        const VkImageView attachments[] = { r.view };
        const VkFramebufferCreateInfo fbInf = {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            nullptr,
            0,
            m_renderPass,
            sizeof(attachments) / sizeof(VkImageView),
            attachments,
            m_surfExtent.width,
            m_surfExtent.height,
            1
        };
        CHECK(vkCreateFramebuffer(m_dev, &fbInf, nullptr, &r.fb),
              "failed to create FBO for one of swapchain images");

#ifdef USE_IMGUI
        // ImGUI needs a set of its own framebuffer objects
        // (pointing to the same swapchain images provided by surface).
        const VkImageView attachmentsIG[] = { r.view };
        const VkFramebufferCreateInfo fbIGInf = {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            nullptr,
            0,
            m_renderPassIG,
            sizeof(attachmentsIG) / sizeof(VkImageView),
            attachmentsIG,
            m_surfExtent.width,
            m_surfExtent.height,
            1
        };
        CHECK(vkCreateFramebuffer(m_dev, &fbIGInf, nullptr, &r.fbIG),
              "[ImGUI] failed to create FBO for one of swapchain images");
#endif

        // Fill the command buffer for emulator rendering
        prepareTextureRenderingCmdBuf(i);
    }

    m_surfaceSizeChanged = false;
}

void VulkanRenderingBackend::resetSwapchain()
{
    Log::d("[Vulkan] Resizing surface to %dx%d", m_width, m_height);

    CHECK(vkDeviceWaitIdle(m_dev), "failed to wait device idle at surface resize");
    destroySwapchain();

    // Update surface capabilities (we need minimum and maximum extent)
    CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physDev, m_surface, &m_surfaceCaps),
          "failed to check surface capabilities");

    createSwapchain();
}

void VulkanRenderingBackend::prepareTextureRenderingCmdBuf(const int frameIndex)
{
    const auto cmdBuf = m_cmdBufs[frameIndex];
    CHECK(vkResetCommandBuffer(cmdBuf, 0), "failed to reset command buffer");
    const VkCommandBufferBeginInfo cmdBufBeginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        0,
        nullptr
    };
    CHECK(vkBeginCommandBuffer(cmdBuf, &cmdBufBeginInfo), "failed to begin command buffer");

    // Copy staging buffer to texture
    transitionImageLayout(cmdBuf,
                          m_texture.image,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          1);
    copyBufferToImage(cmdBuf,
                      m_texStgBuf.buffer,
                      m_texture.image,
                      TEX_WIDTH,
                      TEX_HEIGHT);
    transitionImageLayout(cmdBuf,
                          m_texture.image,
                          VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          1);

    // Render
    const VkClearValue clear[] = {
        {{{ 0.0f, 0.0f, 0.0f, 1.0f }}},
        {{{ 1.0f, 0.0f }}}
    };
    const VkRenderPassBeginInfo renderPassBeginInfo = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        nullptr,
        m_renderPass,
        m_swapChainData[frameIndex].fb,
        {
            { 0, 0 },
            m_surfExtent
        },
        sizeof(clear) / sizeof(VkClearValue),
        clear
    };
    vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    const VkViewport viewPort = {
        0, 0,
        static_cast<float>(m_surfExtent.width), static_cast<float>(m_surfExtent.height),
        0.0f, 1.0f
    };
    vkCmdSetViewport(cmdBuf, 0, 1, &viewPort);
    const VkRect2D scissors = {
        { 0, 0 },
        m_surfExtent
    };
    vkCmdSetScissor(cmdBuf, 0, 1, &scissors);
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_ufmDescSets[frameIndex], 0, nullptr);
    vkCmdDraw(cmdBuf, 6, 1, 0, 0);
    vkCmdEndRenderPass(cmdBuf);
    CHECK(vkEndCommandBuffer(cmdBuf), "failed to end command buffer recording");
}

Maybe<uint32_t> VulkanRenderingBackend::beginRendering()
{
    const auto fence = m_fncsInFlight[m_curFrame];
    const auto semImgAvail = m_semsImageAvailable[m_curFrame];

    // Fence is in signaled state after creation so no infinite wait here
    CHECK(vkWaitForFences(m_dev, 1, &fence, VK_TRUE, UINT64_MAX),
          "failed to wait for frame-in-flight fence");

    uint32_t imageIndex;
    const auto r = vkAcquireNextImageKHR(m_dev, m_swapChain, UINT64_MAX, semImgAvail, VK_NULL_HANDLE, &imageIndex);
    if (r == VK_ERROR_OUT_OF_DATE_KHR)
    {
        resetSwapchain();
        return { };
    }
    else if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR)
        throw runtime_error { "failed to acquire next swapchain image" };

    CHECK(vkResetFences(m_dev, 1, &fence),
          "failed to reset frame-in-flight fence");

    return imageIndex;
}

void VulkanRenderingBackend::endRendering(const uint32_t imgIndex, const uint32_t numCmdBufs, const VkCommandBuffer *const cmdBufs)
{
    const auto fence = m_fncsInFlight[m_curFrame];
    const auto semImgAvail = m_semsImageAvailable[m_curFrame],
               semRenderFinished = m_semsRenderFinished[m_curFrame];

    // Submit frame to render queue
    const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    const VkSubmitInfo cmdSubmitInfo = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        1,
        &semImgAvail,
        waitStages,
        numCmdBufs,
        cmdBufs,
        1,
        &semRenderFinished
    };
    CHECK(vkQueueSubmit(m_renderQueue, 1, &cmdSubmitInfo, fence),
          "failed to submit command buffer to render queue");

    const VkPresentInfoKHR presentInfo = {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        nullptr,
        1,
        &semRenderFinished,
        1,
        &m_swapChain,
        &imgIndex
    };
    const auto r = vkQueuePresentKHR(m_presentationQueue, &presentInfo);
    if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR || m_surfaceSizeChanged)
        resetSwapchain();
    else if (r != VK_SUCCESS)
        throw runtime_error { "failed to submit frame to presentation queue" };

    // Advance frame index
    m_curFrame = (m_curFrame + 1) % MAX_FIF;
}

void VulkanRenderingBackend::draw()
{
    const auto mbImageIndex = beginRendering();
    if (mbImageIndex.isNothing())
        return;

    const auto imageIndex = mbImageIndex.value();

    // Command buffer containing NES texture rendering commands is already prepared at this point.
#ifdef USE_IMGUI
    auto cmdBufIG = m_cmdBufsIG[imageIndex];

    CHECK(vkResetCommandBuffer(cmdBufIG, 0),
          "[ImGUI] failed to reset command buffer");

    const VkCommandBufferBeginInfo cmdBufBeginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        0,
        nullptr
    };
    CHECK(vkBeginCommandBuffer(cmdBufIG, &cmdBufBeginInfo),
          "[ImGUI] failed to begin command buffer");

    const VkClearValue clear[] = {
        {{{ 0.0f, 0.0f, 0.0f, 1.0f }}},
        {{{ 1.0f, 0.0f }}}
    };
    const VkRenderPassBeginInfo renderPassBeginInfo = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        nullptr,
        m_renderPassIG,
        m_swapChainData[imageIndex].fbIG,
        {
            { 0, 0 },
            m_surfExtent
        },
        sizeof(clear) / sizeof(VkClearValue),
        clear
    };
    vkCmdBeginRenderPass(cmdBufIG, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Fill the ImGUI rendering commands to the buffer
    m_renderFunc(cmdBufIG);

    vkCmdEndRenderPass(cmdBufIG);
    CHECK(vkEndCommandBuffer(cmdBufIG),
          "[ImGUI] failed to end command buffer recording");

    const VkCommandBuffer cmdBufs[] = {
        m_cmdBufs[imageIndex],
        cmdBufIG
    };
#else
    const VkCommandBuffer cmdBufs[] = {
        m_cmdBufs[imageIndex]
    };
#endif

    endRendering(imageIndex, sizeof(cmdBufs) / sizeof(VkCommandBuffer), cmdBufs);
}

void VulkanRenderingBackend::drawIdle()
{
    fillWhiteNoise_RGBA8(m_pTexData);
    draw();
}

void VulkanRenderingBackend::waitDeviceIdle()
{
    CHECK(vkDeviceWaitIdle(m_dev), "failed to wait device idle (external request)");
}

#ifdef USE_IMGUI
VkCommandBuffer VulkanRenderingBackend::beginTransientCmdBufIG()
{
    // Allocate command buffers
    const VkCommandBufferAllocateInfo cmdBufInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        m_cmdPoolIG,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1
    };

    VkCommandBuffer tmpCmdBuf;
    CHECK(vkAllocateCommandBuffers(m_dev, &cmdBufInfo, &tmpCmdBuf),
          "[ImGUI] failed to allocate transiend command buffer");

    const VkCommandBufferBeginInfo begInf = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        nullptr,
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    CHECK(vkBeginCommandBuffer(tmpCmdBuf, &begInf), "[ImGUI] failed to begin transient buffer recording");

    return tmpCmdBuf;
}

void VulkanRenderingBackend::endTransientCmdBufIG(VkCommandBuffer cbuf)
{
    CHECK(vkEndCommandBuffer(cbuf), "[ImGUI] failed to end transient buffer recording");
    const VkSubmitInfo cmdSubmitInfo = {
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
        nullptr,
        0,
        nullptr,
        nullptr,
        1,
        &cbuf,
        0,
        nullptr
    };
    CHECK(vkQueueSubmit(m_renderQueue, 1, &cmdSubmitInfo, VK_NULL_HANDLE),
          "[ImGUI] failed to submit transient buffer to queue");
    CHECK(vkQueueWaitIdle(m_renderQueue), "[ImGUI] failed to wait transient buffer to finish");
    vkFreeCommandBuffers(m_dev, m_cmdPoolIG, 1, &cbuf);
}
#endif
