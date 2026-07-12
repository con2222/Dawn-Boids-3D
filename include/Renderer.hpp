#pragma once

#include "webgpu/webgpu_cpp.h"
#include "WebGPUContext.hpp"
#include "Mesh.hpp"
#include "CoreData.hpp"
#include <vector>



namespace WGPUBoids {

class Camera;
class Interface;

class Renderer {
  public:
    Renderer() = default;

    bool init(wgpu::Device device, wgpu::Queue queue, wgpu::ShaderModule renderShader, wgpu::ShaderModule computeShader, wgpu::TextureFormat surfaceFormat, 
      wgpu::TextureFormat depthFormat, const std::vector<BoidData>& boids, const SimulationParams& params);


    void updateMeshBuffers(const Mesh &model);
    void initBoidsData(const std::vector<BoidData>& boids);
    void draw(WebGPUContext &gpu, const Camera& camera, Interface& uiLayer);
  private:
    void initBuffers(const SimulationParams& params);
    void initBindGroups(size_t boidsCount);
    void initRenderPipeline(wgpu::ShaderModule shader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat);
    void initLinePipeline(wgpu::ShaderModule shader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat);

    void initCompute(wgpu::ShaderModule computeShader, size_t boidsCount);

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


    // Second Pipeline
    wgpu::Buffer cubeVertexBuffer;
    wgpu::Buffer cubeIndexBuffer;
    uint32_t cubeIndexCount = 24;

    wgpu::Buffer cubeUniformBuffer;

    // Compute pipeline
    wgpu::ComputePipeline computePipeline;
    wgpu::PipelineLayout computePipelineLayout;
    wgpu::BindGroupLayout bglCompute;

    wgpu::Buffer boidUniformBuffer;
    wgpu::Buffer simulationParamsBuffer;

    wgpu::Buffer boidsBufferA;
    wgpu::Buffer boidsBufferB;

    wgpu::BindGroup computeBindGroupA, computeBindGroupB;

    // Debug pipeline
    wgpu::RenderPipeline debugVelocityPipeline;
    wgpu::RenderPipeline debugCoMPipeline;

    void initDebugVelocityPipeline(wgpu::ShaderModule shader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat);
    void initDebugCoMPipeline(wgpu::ShaderModule shader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat);

    int frameCount = 0;
};





}; // namespace WGPUBoids