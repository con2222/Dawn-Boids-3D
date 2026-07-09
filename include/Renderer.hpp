#pragma once

#include "webgpu/webgpu_cpp.h"
#include "WebGPUContext.hpp"
#include "Mesh.hpp"



namespace WGPUBoids {

class Camera;

class Renderer {
  public:
    Renderer() = default;

    bool init(wgpu::Device device, wgpu::Queue queue, wgpu::ShaderModule shader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat);


    void updateMeshBuffers(const Mesh &model);
    void draw(WebGPUContext &gpu, const Camera& camera);
  private:
    void initBuffers();
    void initBindGroups();
    void initRenderPipeline(wgpu::ShaderModule shader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat);
    std::vector<wgpu::BindGroupLayout> bindGroupLayouts;
    wgpu::PipelineLayout pipelineLayout;
    std::vector<wgpu::BindGroup> bindGroups;

    void setDefault(wgpu::BindGroupLayoutEntry &bindingLayout);

    wgpu::Device device;
    wgpu::Queue queue;

    /* All buffers */
    wgpu::RenderPipeline renderPipeline;

    wgpu::Buffer vertexBuffer;
    unsigned int vertexBufferSize = 1024;

    wgpu::Buffer indexBuffer;
    unsigned int indexBufferSize = 1024;
    uint32_t indexCount = 0;

    wgpu::Buffer uniformBuffer;

};





}; // namespace WGPUBoids