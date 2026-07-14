#pragma once

#include "webgpu/webgpu_cpp.h"
#include "glfw3webgpu.h"


namespace WGPUBoids {

class WebGPUContext {
  public:
    bool init(GLFWwindow* window, int width, int height);
    void resizeSwapchain(int width, int height);
  private:
    bool initInstance();
    bool initAdapter();
    bool initDevice();
    bool initQueue();

    bool setSurface(WGPUSurface c_surface);
    void setupSurfaceConfig(int width, int height);

    void setDefault(wgpu::Limits& limits) const;
    wgpu::RequiredLimits getRequiredLimits() const;

    wgpu::Instance instance;
    wgpu::Adapter adapter;
    wgpu::Device device;
    wgpu::Queue queue;
    wgpu::Surface surface;
    wgpu::SurfaceConfiguration surfaceConfig;
    wgpu::TextureFormat surfaceFormat;

    wgpu::Texture depthTexture;
    wgpu::Texture msaaTexture;
    wgpu::TextureView msaaTextureView;
    wgpu::TextureView depthTextureView;
    wgpu::TextureDescriptor depthTextureDescriptor;
    wgpu::TextureViewDescriptor depthTextureViewDescriptor;
    wgpu::TextureFormat depthTextureFormat;
   
  public:
   const wgpu::Device& getDevice() const { return device; }
   const wgpu::Queue& getQueue() const { return queue; }
   wgpu::TextureFormat getSurfaceFormat() const { return surfaceFormat; }
   wgpu::SurfaceConfiguration getSurfaceConfig() const { return surfaceConfig; }
   wgpu::TextureView getMsaaTextureView() const { return msaaTextureView; }
   wgpu::TextureView getDepthTextureView() const { return depthTextureView; }
   wgpu::TextureView getNextSurfaceViewData();
   wgpu::TextureFormat getDepthTextureFormat() const { return depthTextureFormat; }

   void present() const { surface.Present(); }

};

}; // namespace WGPUBoids