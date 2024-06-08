#ifndef VKRBE_H
#define VKRBE_H

#include <PPU.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <cassert>

class VulkanRenderingBackend final: public RenderingBackend
{
    struct SwapchainData final
    {
        VkImageView view;
        VkFramebuffer fb;
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

    // Per-frame data
    static constexpr int MAX_FIF = 2;
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

    // Handling validation layer messages
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                        VkDebugUtilsMessageTypeFlagsEXT type,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                        void *pThis);

    void createSwapchain();
    void destroySwapchain();
    void resetSwapchain();
    void prepareTexture();

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

    VkInstance instance() const noexcept
    {
        return m_inst;
    }

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
    void drawError() override
    {
    }
};

#endif
