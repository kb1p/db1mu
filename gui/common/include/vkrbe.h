#ifndef VKRBE_H
#define VKRBE_H

#include <PPU.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <functional>
#include <cstdint>
#include <cassert>

class VulkanRenderingBackend final: public RenderingBackend
{
    struct SwapchainData final
    {
        VkImageView view;
        VkFramebuffer fb;
#ifdef USE_IMGUI
        VkFramebuffer fbIG;
#endif
    };

    struct Buffer
    {
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;

        void dispose(VkDevice dev);
    };

    struct Texture
    {
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;

        void dispose(VkDevice dev);
    };

    struct SampleableTexture final: Texture
    {
        VkSampler sampler = VK_NULL_HANDLE;

        using Texture::operator=;

        void dispose(VkDevice dev);
    };

    VkInstance m_inst = VK_NULL_HANDLE;
    bool m_externalInstance = false;
    VkDebugUtilsMessengerEXT m_dbgMsgr = VK_NULL_HANDLE;
    VkPhysicalDevice m_physDev = VK_NULL_HANDLE;
    VkDevice m_dev = VK_NULL_HANDLE;
    uint32_t m_qfIndexGraphical = 0,
             m_qfIndexPresentation = 0;
    VkQueue m_renderQueue = VK_NULL_HANDLE,
            m_presentationQueue = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descrSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkCommandPool m_cmdPool = VK_NULL_HANDLE,
                  m_cmdTmpPool = VK_NULL_HANDLE;
    VkDescriptorPool m_descPool = VK_NULL_HANDLE;

    // Surface and swapchain data
    int m_width = -1,
        m_height = -1;
    bool m_surfaceSizeChanged = false;
    VkSurfaceCapabilitiesKHR m_surfaceCaps;
    VkSurfaceFormatKHR m_surfaceFormat;
    VkPresentModeKHR m_presentationMode;
    VkExtent2D m_surfExtent;
    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    std::vector<SwapchainData> m_swapChainData;
    uint32_t m_imageCount = 0;

    // Per-frame data
    static constexpr int MAX_FIF = 3;
    VkCommandBuffer m_cmdBufs[MAX_FIF];
    VkDescriptorSet m_ufmDescSets[MAX_FIF];
    void *m_ufmBufsMapping[MAX_FIF];
    VkSemaphore m_semsRenderFinished[MAX_FIF],
                m_semsImageAvailable[MAX_FIF];
    VkFence m_fncsInFlight[MAX_FIF];
    int m_curFrame = 0;

    VkPhysicalDeviceProperties m_physDevProps;
    VkPhysicalDeviceFeatures m_physDevFeats;

    // Texture data
    // Texture object
    SampleableTexture m_texture;

    // Texture's staging buffer
    Buffer m_texStgBuf;

    // Pointer to texture's host-visible staging buffer memory
    uint8_t *m_pTexData = nullptr;

#ifdef USE_IMGUI
    // Additional stuff required for ImGUI rendering
    VkDescriptorPool m_descPoolIG = VK_NULL_HANDLE;
    VkRenderPass m_renderPassIG = VK_NULL_HANDLE;
    VkCommandPool m_cmdPoolIG = VK_NULL_HANDLE;
    VkCommandBuffer m_cmdBufsIG[MAX_FIF];
    std::function<void(VkCommandBuffer)> m_renderFunc;
#endif

    // Handling validation layer messages
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                        VkDebugUtilsMessageTypeFlagsEXT type,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                        void *pThis);

    void createSwapchain();
    void destroySwapchain();
    void resetSwapchain();
    void prepareTexture();
    void prepareTextureRenderingCmdBuf(const int frameIndex);
    Maybe<uint32_t> beginRendering();
    void endRendering(const uint32_t imgIndex, const uint32_t numCmdBufs, const VkCommandBuffer *const cmdBufs);

    Buffer createBuffer(const VkDeviceSize size,
                        const VkBufferUsageFlags usage,
                        const VkMemoryPropertyFlags properties);
    Texture createTexture(uint32_t width,
                          uint32_t height,
                          uint32_t nMipLevels,
                          VkSampleCountFlagBits numSamples,
                          VkFormat format,
                          VkImageTiling tiling,
                          VkImageUsageFlags usage,
                          VkMemoryPropertyFlags props,
                          VkImageAspectFlags aspect);
    VkCommandBuffer beginTransientCmdBuf();
    void endTransientCmdBuf(VkCommandBuffer cbuf);
    void transitionImageLayout(VkCommandBuffer cbuf, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t nMipLevels);
    void copyBufferToImage(VkCommandBuffer cbuf, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    VkImageView createSimple2DImgView(VkImage src, VkFormat fmt, VkImageAspectFlags aspect, uint32_t nMipLevels);
    uint32_t findMemoryTypeIndex(const uint32_t typeFilter, const VkMemoryPropertyFlags reqProps);
    VkFormat findSupportedFormat(VkImageTiling tiling, VkFormatFeatureFlags features, std::initializer_list<VkFormat> fmts);

public:
    ~VulkanRenderingBackend()
    {
        release();
    }

    void init(uint32_t nExts, const char **pExts);
    void init(VkInstance inst);
    void release();

    void setupOutput(VkSurfaceKHR surf);
    void resize(int w, int h) noexcept
    {
        m_width = w;
        m_height = h;
        m_surfaceSizeChanged = true;
    }

    void setLine(const int n,
                 const c6502_byte_t *pColorData,
                 const c6502_byte_t bgColor) override
    {
        assert(m_pTexData != nullptr);
        setLineToBuf(m_pTexData, n, pColorData, bgColor);
    }

    void draw() override;
    void drawIdle() override;

    VkInstance instance() const noexcept
    {
        return m_inst;
    }

    VkPhysicalDevice physicalDevice() const noexcept
    {
        return m_physDev;
    }

    VkDevice logicalDevice() const noexcept
    {
        return m_dev;
    }

    uint32_t renderQueueFamilyIndex() const noexcept
    {
        return m_qfIndexGraphical;
    }

    VkQueue renderQueue() const noexcept
    {
        return m_renderQueue;
    }

    VkDescriptorPool descriptorPool() const noexcept
    {
        return m_descPool;
    }

    void waitDeviceIdle();

#ifdef USE_IMGUI
    VkDescriptorPool descriptorPoolIG() const noexcept
    {
        return m_descPoolIG;
    }

    VkRenderPass renderPassIG() const noexcept
    {
        return m_renderPassIG;
    }

    VkCommandBuffer beginTransientCmdBufIG();
    void endTransientCmdBufIG(VkCommandBuffer cbuf);

    void setRenderFunctionIG(std::function<void(VkCommandBuffer)> func) noexcept
    {
        m_renderFunc = std::move(func);
    }
#endif

    uint32_t swcMinImageCount() const noexcept
    {
        return m_surfaceCaps.minImageCount;
    }

    uint32_t swcImageCount() const noexcept
    {
        return m_imageCount;
    }
};

#endif
