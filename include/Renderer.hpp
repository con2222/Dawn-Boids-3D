#pragma once

#include "webgpu/webgpu_cpp.h"
#include "WebGPUContext.hpp"
#include "Mesh.hpp"

namespace WGPUBoids {
class Renderer {
  public:
    Renderer() = default;

    bool init(wgpu::Device device, wgpu::Queue queue, wgpu::ShaderModule shader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat);


    void updateMeshBuffers(const Mesh &model);
    void draw(WebGPUContext &gpu);
  private:
    wgpu::Device device;
    wgpu::Queue queue;

    wgpu::RenderPipeline renderPipeline;

    wgpu::Buffer vertexBuffer;
    unsigned int vertexBufferSize = 1024;

    wgpu::Buffer indexBuffer;
    unsigned int indexBufferSize = 1024;
    uint32_t indexCount = 0;
};





}; // namespace WGPUBoids