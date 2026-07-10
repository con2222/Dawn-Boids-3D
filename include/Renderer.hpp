#pragma once

#include "webgpu/webgpu_cpp.h"
#include "WebGPUContext.hpp"
#include "Mesh.hpp"
#include "CoreData.hpp"
#include <vector>



namespace WGPUBoids {

class Camera;

class Renderer {
  public:
    Renderer() = default;

    bool init(wgpu::Device device, wgpu::Queue queue, wgpu::ShaderModule shader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat, const std::vector<BoidData>& boids);


    void updateMeshBuffers(const Mesh &model);
    void updateBoidsData(const std::vector<BoidData>& boids);
    void draw(WebGPUContext &gpu, const Camera& camera);
  private:
    void initBuffers();
    void initBindGroups(size_t boidsCount);
    void initRenderPipeline(wgpu::ShaderModule shader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat);
    void initLinePipeline(wgpu::ShaderModule shader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat);
    std::vector<wgpu::BindGroupLayout> bindGroupLayouts;
    wgpu::PipelineLayout pipelineLayout;
    std::vector<wgpu::BindGroup> boidBindGroups;
    std::vector<wgpu::BindGroup> cubeBindGroups;

    void setDefault(wgpu::BindGroupLayoutEntry &bindingLayout);

    wgpu::Device device;
    wgpu::Queue queue;

    uint32_t currentBoidsCount = 0;

   /* All pipelines*/
    wgpu::RenderPipeline renderPipeline;
    wgpu::RenderPipeline linePipeline;

    /* All buffers */

    // First Pipeline
    wgpu::Buffer vertexBuffer;
    uint32_t vertexBufferSize = 1024;

    wgpu::Buffer indexBuffer;
    uint32_t indexBufferSize = 1024;
    uint32_t indexCount = 0;

    wgpu::Buffer boidUniformBuffer;
    wgpu::Buffer boidDataBuffer;


    // Second Pipeline
    wgpu::Buffer cubeVertexBuffer;
    wgpu::Buffer cubeIndexBuffer;
    uint32_t cubeIndexCount = 24;

    wgpu::Buffer cubeUniformBuffer;

};





}; // namespace WGPUBoids