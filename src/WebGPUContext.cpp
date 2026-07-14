#include "WebGPUContext.hpp"

#include <iostream>

namespace WGPUBoids {

bool WebGPUContext::init(GLFWwindow* window, int width, int height) {
    if (!initInstance()) return false;

    WGPUSurface c_surface = glfwGetWGPUSurface(instance.Get(), window);
    if (!setSurface(c_surface)) return false;

    if (!initAdapter()) return false;
    if (!initDevice()) return false;
    if (!initQueue()) return false;

    setupSurfaceConfig(width, height);
    resizeSwapchain(width, height);

    return true;
}

bool WebGPUContext::initInstance() {
    const char* toggleName = "enable_immediate_error_handling";
    wgpu::DawnTogglesDescriptor toggles{};
    toggles.enabledToggles = &toggleName;
    toggles.enabledToggleCount = 1;
    wgpu::InstanceDescriptor instanceDesc{};
    instanceDesc.nextInChain = &toggles;
    instance = wgpu::CreateInstance(&instanceDesc);
    if (!instance) {
        std::cerr << "Could not intialize WebGPU!" << std::endl;
        return false;
    }
    return true;
}

bool WebGPUContext::setSurface(WGPUSurface c_surface) {
    if (!c_surface) {
        std::cerr << "Could not set surface!" << std::endl;
        return false;
    }
    surface = wgpu::Surface::Acquire(c_surface);
    return true;
}

bool WebGPUContext::initAdapter() {
    wgpu::RequestAdapterOptions adapterOpts{};
    adapterOpts.nextInChain = nullptr;
    adapterOpts.compatibleSurface = surface;
    
    bool requestEnded = false;

    instance.RequestAdapter(
        &adapterOpts,
        wgpu::CallbackMode::AllowProcessEvents,
        [this, &requestEnded](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter_, const char* message) {
            if (status == wgpu::RequestAdapterStatus::Success) {
                adapter = adapter_;
            } else {
                std::cerr << "Could not get WebGPU adapter: ";
                if (message) std::cerr << message;
                std::cerr << std::endl;
            }
            requestEnded = true;
        }
    );

    while (!requestEnded) {
        instance.ProcessEvents();
    }

    return adapter != nullptr;
}

bool WebGPUContext::initDevice() {
    wgpu::DeviceDescriptor deviceDesc{};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "My device";
    deviceDesc.requiredFeatureCount = 0;
    deviceDesc.requiredLimits = nullptr;
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";

    deviceDesc.deviceLostCallbackInfo.callback = [](WGPUDevice const * device, WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
        std::cerr << "Device lost: reason " << reason << " device: " << device;
        if (message) std::cerr << " (" << message << ")";
        std::cerr << std::endl;
    };
    wgpu::RequiredLimits requiredLimits = getRequiredLimits();
    deviceDesc.requiredLimits = &requiredLimits;
    
    bool requestEnded = false;

    adapter.RequestDevice(
        &deviceDesc,
        wgpu::CallbackMode::AllowProcessEvents,
        [this, &requestEnded](wgpu::RequestDeviceStatus status, wgpu::Device device_, char const* message) {
            if (status == wgpu::RequestDeviceStatus::Success) {
                device = device_;
            } else {
                std::cerr << "Could not get WebGPU device: ";
                if (message) std::cerr << message;
                std::cerr << std::endl;
            }
            requestEnded = true;
        }
    );

    while (!requestEnded) {
        instance.ProcessEvents();
    }

    device.SetUncapturedErrorCallback(
        [](WGPUErrorType type, char const* message, void*) {
            std::cerr << "Uncaptured device error: type" << static_cast<uint32_t>(type);
            if (message) std::cerr << " (" << message << ")";
            std::cerr << std::endl;
        },
        nullptr
    );

    return device != nullptr;
}

bool WebGPUContext::initQueue() {
    queue = device.GetQueue();
    
    queue.OnSubmittedWorkDone(
        wgpu::CallbackMode::AllowProcessEvents,
        [](wgpu::QueueWorkDoneStatus status) {
            std::cout << "Queued work finished with status: " << static_cast<uint32_t>(status) << std::endl;
        }
    );

    return queue != nullptr;
}

void WebGPUContext::setupSurfaceConfig(int width, int height) {
    surfaceConfig.nextInChain = nullptr;
    surfaceConfig.device = device;
    surfaceConfig.usage = wgpu::TextureUsage::RenderAttachment;
    surfaceConfig.presentMode = wgpu::PresentMode::Fifo;
    surfaceConfig.viewFormatCount = 0;
    surfaceConfig.viewFormats = nullptr;

    wgpu::SurfaceCapabilities capabilities;
    surface.GetCapabilities(adapter, &capabilities);
    if (capabilities.formatCount > 0) {
        surfaceConfig.format = capabilities.formats[0];
    }
    else {
        surfaceConfig.format = wgpu::TextureFormat::BGRA8Unorm;
    }
    if (capabilities.alphaModeCount > 0) {
        surfaceConfig.alphaMode = capabilities.alphaModes[0];
    }
    else {
        surfaceConfig.alphaMode = wgpu::CompositeAlphaMode::Auto;
    }
    surfaceFormat = surfaceConfig.format;
    surfaceConfig.width = width;
    surfaceConfig.height = height;
    surface.Configure(&surfaceConfig);
}


wgpu::RequiredLimits WebGPUContext::getRequiredLimits() const {
    wgpu::SupportedLimits supportedLimits;
    supportedLimits.nextInChain = nullptr;
    adapter.GetLimits(&supportedLimits);

    wgpu::RequiredLimits requiredLimits = {};
    setDefault(requiredLimits.limits);

    requiredLimits.limits.maxVertexAttributes = 4;
    requiredLimits.limits.maxVertexBuffers = 3;
    requiredLimits.limits.maxBufferSize = supportedLimits.limits.maxBufferSize;
    requiredLimits.limits.maxBindGroups = 2;
    requiredLimits.limits.maxUniformBuffersPerShaderStage = 2;
    requiredLimits.limits.maxUniformBufferBindingSize = supportedLimits.limits.maxStorageBufferBindingSize;
    requiredLimits.limits.maxVertexBufferArrayStride = 20;
    requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
    requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
    requiredLimits.limits.maxInterStageShaderComponents = 16;
    requiredLimits.limits.maxTextureDimension1D = supportedLimits.limits.maxTextureDimension1D;
    requiredLimits.limits.maxTextureDimension2D = supportedLimits.limits.maxTextureDimension2D;
    requiredLimits.limits.maxTextureArrayLayers = 1;
    requiredLimits.limits.maxSampledTexturesPerShaderStage = 1;
    requiredLimits.limits.maxSamplersPerShaderStage = 1;
    requiredLimits.limits.maxTextureDimension1D = 2048;
    requiredLimits.limits.maxTextureDimension2D = 2048;

    return requiredLimits;
}

void WebGPUContext::setDefault(wgpu::Limits& limits) const {
    limits.maxTextureDimension1D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension2D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension3D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureArrayLayers = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBindGroups = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBindGroupsPlusVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBindingsPerBindGroup = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxDynamicUniformBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxDynamicStorageBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxSampledTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxSamplersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxStorageBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxStorageTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxUniformBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxUniformBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
    limits.maxStorageBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
    limits.minUniformBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
    limits.minStorageBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBufferSize = WGPU_LIMIT_U64_UNDEFINED;
    limits.maxVertexAttributes = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxVertexBufferArrayStride = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxInterStageShaderComponents = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxInterStageShaderVariables = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxColorAttachments = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxColorAttachmentBytesPerSample = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupStorageSize = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeInvocationsPerWorkgroup = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupSizeX = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupSizeY = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupSizeZ = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupsPerDimension = WGPU_LIMIT_U32_UNDEFINED;
}

wgpu::TextureView WebGPUContext::getNextSurfaceViewData() {
    wgpu::SurfaceTexture surfaceTexture;
    surface.GetCurrentTexture(&surfaceTexture);

    if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::Success) {
        return nullptr;
    }

    wgpu::TextureViewDescriptor viewDescriptor;
    viewDescriptor.nextInChain = nullptr;
    viewDescriptor.label = "Surface texture view";
    viewDescriptor.format = surfaceTexture.texture.GetFormat();
    viewDescriptor.dimension = wgpu::TextureViewDimension::e2D;
    viewDescriptor.baseMipLevel = 0;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.baseArrayLayer = 0;
    viewDescriptor.arrayLayerCount = 1;
    viewDescriptor.aspect = wgpu::TextureAspect::All;

    return surfaceTexture.texture.CreateView(&viewDescriptor);
}

void WebGPUContext::resizeSwapchain(int width, int height) {
    if (width <= 0 || height <= 0) return;

    msaaTextureView = nullptr;
    depthTextureView = nullptr;
    msaaTexture = nullptr;
    depthTexture = nullptr;

    surfaceConfig.width = static_cast<uint32_t>(width);
    surfaceConfig.height = static_cast<uint32_t>(height);
    surface.Configure(&surfaceConfig);

    wgpu::TextureDescriptor msaaDesc{};
    msaaDesc.size = { surfaceConfig.width, surfaceConfig.height, 1 };
    msaaDesc.sampleCount = 4;
    msaaDesc.format = surfaceFormat;
    msaaDesc.usage = wgpu::TextureUsage::RenderAttachment;
    msaaTexture = device.CreateTexture(&msaaDesc);
    msaaTextureView = msaaTexture.CreateView();

    depthTextureFormat = wgpu::TextureFormat::Depth24Plus;

    depthTextureDescriptor = {};
    depthTextureDescriptor.dimension = wgpu::TextureDimension::e2D;
    depthTextureDescriptor.format = depthTextureFormat;
    depthTextureDescriptor.usage = wgpu::TextureUsage::RenderAttachment;
    depthTextureDescriptor.mipLevelCount = 1;
    depthTextureDescriptor.sampleCount = 4;
    depthTextureDescriptor.size = {surfaceConfig.width, surfaceConfig.height, 1};

    depthTexture = device.CreateTexture(&depthTextureDescriptor);
    depthTextureView = depthTexture.CreateView();
}

}; // namespace WGPUBoids