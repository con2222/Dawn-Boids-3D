#include "Renderer.hpp"
#include "CoreData.hpp"
#include "Camera.hpp"
#include "Interface.hpp"

#include "glm/gtc/matrix_transform.hpp"

namespace WGPUBoids {

bool Renderer::init(wgpu::Device dev, wgpu::Queue q, wgpu::ShaderModule renderShader, 
    wgpu::ShaderModule computeShader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat, const std::vector<BoidData>& boids, const SimulationParams& params) {
    device = dev;
    queue = q;

    initBuffers(params);
    initBoidsData(boids);
    initBindGroups(boids.size());
    initCompute(computeShader, boids.size());
    initRenderPipeline(renderShader, surfaceFormat, depthFormat);
    initLinePipeline(renderShader, surfaceFormat, depthFormat);
    initDebugVelocityPipeline(renderShader, surfaceFormat, depthFormat);
    initDebugCoMPipeline(renderShader, surfaceFormat, depthFormat);

    return renderPipeline != nullptr;
}

void Renderer::updateMeshBuffers(const Mesh &model) {
    if (!device) return;

    if (!vertexBuffer || model.getVertexData().size() >= vertexBufferSize) {
        if (model.getVertexData().size() >= vertexBufferSize) {
            vertexBufferSize = model.getVertexData().size() * 1.5;
        }
        if (vertexBuffer) {
            vertexBuffer.Destroy();
        }

        wgpu::BufferDescriptor bufferDesc = {};
        bufferDesc.nextInChain = nullptr;
        bufferDesc.size = vertexBufferSize * sizeof(VertexAttributes);
        bufferDesc.label = "Vertex buffer";
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
        bufferDesc.mappedAtCreation = false;
        vertexBuffer = device.CreateBuffer(&bufferDesc);
    }

    if (!indexBuffer || model.getIndexData().size() >= indexBufferSize) {
        if (model.getIndexData().size() >= indexBufferSize) {
            indexBufferSize = model.getIndexData().size() * 1.5;
        }
        if (indexBuffer) {
            indexBuffer.Destroy();
        }

        wgpu::BufferDescriptor indexBufferDesc = {};
        indexBufferDesc.nextInChain = nullptr;
        indexBufferDesc.size = indexBufferSize * sizeof(uint32_t);
        indexBufferDesc.label = "Index buffer";
        indexBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
        indexBufferDesc.mappedAtCreation = false;
        indexBuffer = device.CreateBuffer(&indexBufferDesc);
    }

    queue.WriteBuffer(vertexBuffer, 0, model.getVertexData().data(), model.getVertexData().size() * sizeof(VertexAttributes));

    queue.WriteBuffer(indexBuffer, 0, model.getIndexData().data(), model.getIndexData().size() * sizeof(uint32_t));
    indexCount = static_cast<uint32_t>(model.getIndexData().size());
}

void Renderer::initBoidsData(const std::vector<BoidData>& boids) {
    if (!device) return;
    
    currentBoidsCount = static_cast<uint32_t>(boids.size());
    size_t bufferSize = currentBoidsCount * sizeof(BoidData);

    wgpu::BufferDescriptor boidDataBufferDesc = {};
    boidDataBufferDesc.nextInChain = nullptr;
    boidDataBufferDesc.size = bufferSize;
    boidDataBufferDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
    boidDataBufferDesc.mappedAtCreation = false;

    boidDataBufferDesc.label = "Boids buffer A";
    boidsBufferA = device.CreateBuffer(&boidDataBufferDesc);

    boidDataBufferDesc.label = "Boids Buffer B";
    boidsBufferB = device.CreateBuffer(&boidDataBufferDesc);

    queue.WriteBuffer(boidsBufferA, 0, boids.data(), bufferSize);
}

void Renderer::initCompute(wgpu::ShaderModule computeShader, size_t boidsCount) {
    std::vector<wgpu::BindGroupLayoutEntry> bglEntries(3);

    bglEntries[0].binding = 0;
    bglEntries[0].buffer.type = wgpu::BufferBindingType::Uniform;
    bglEntries[0].visibility = wgpu::ShaderStage::Compute;

    bglEntries[1].binding = 1;
    bglEntries[1].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
    bglEntries[1].visibility = wgpu::ShaderStage::Compute;

    bglEntries[2].binding = 2;
    bglEntries[2].buffer.type = wgpu::BufferBindingType::Storage;
    bglEntries[2].visibility = wgpu::ShaderStage::Compute;

    wgpu::BindGroupLayoutDescriptor bglDescriptor = {};
    bglDescriptor.entries = bglEntries.data();
    bglDescriptor.entryCount = static_cast<uint32_t>(bglEntries.size());
    bglDescriptor.nextInChain = nullptr;
    bglCompute = device.CreateBindGroupLayout(&bglDescriptor);

    wgpu::PipelineLayoutDescriptor plDesc = {};
    plDesc.bindGroupLayoutCount = 1;
    plDesc.bindGroupLayouts = &bglCompute;
    plDesc.nextInChain = nullptr;
    computePipelineLayout = device.CreatePipelineLayout(&plDesc);

    std::vector<wgpu::BindGroupEntry> bindingsA(3);
    
    bindingsA[0].binding = 0;
    bindingsA[0].buffer = simulationParamsBuffer;
    bindingsA[0].offset = 0;
    bindingsA[0].size = sizeof(SimulationParams);

    bindingsA[1].binding = 1;
    bindingsA[1].buffer = boidsBufferA;
    bindingsA[1].offset = 0;
    bindingsA[1].size = sizeof(BoidData) * boidsCount;

    bindingsA[2].binding = 2;
    bindingsA[2].buffer = boidsBufferB;
    bindingsA[2].offset = 0;
    bindingsA[2].size = sizeof(BoidData) * boidsCount;

    wgpu::BindGroupDescriptor bgDescA = {};
    bgDescA.entries = bindingsA.data();
    bgDescA.entryCount = static_cast<uint32_t>(bindingsA.size());
    bgDescA.layout = bglCompute;
    bgDescA.nextInChain = nullptr;
    computeBindGroupA = device.CreateBindGroup(&bgDescA);


    std::vector<wgpu::BindGroupEntry> bindingsB(3);
    
    bindingsB[0].binding = 0;
    bindingsB[0].buffer = simulationParamsBuffer;
    bindingsB[0].offset = 0;
    bindingsB[0].size = sizeof(SimulationParams);

    bindingsB[1].binding = 1;
    bindingsB[1].buffer = boidsBufferB;
    bindingsB[1].offset = 0;
    bindingsB[1].size = sizeof(BoidData) * boidsCount;

    bindingsB[2].binding = 2;
    bindingsB[2].buffer = boidsBufferA;
    bindingsB[2].offset = 0;
    bindingsB[2].size = sizeof(BoidData) * boidsCount;


    wgpu::BindGroupDescriptor bgDescB = {};
    bgDescB.entries = bindingsB.data();
    bgDescB.entryCount = static_cast<uint32_t>(bindingsB.size());
    bgDescB.layout = bglCompute;
    bgDescB.nextInChain = nullptr;
    computeBindGroupB = device.CreateBindGroup(&bgDescB);


    wgpu::ComputePipelineDescriptor computeDesc = {};
    computeDesc.compute.module = computeShader;
    computeDesc.compute.entryPoint = "compute_main";
    computeDesc.layout = computePipelineLayout;
    computePipeline = device.CreateComputePipeline(&computeDesc);
}

void Renderer::draw(WebGPUContext &gpu, const Camera& camera, Interface& uiLayer) {
    device.Tick();

    auto params = uiLayer.getParams();
    params.visionRadius = glm::cos(glm::radians(params.visionRadius));

    Uniforms boidUniforms, cubeUniforms;
    boidUniforms.time = glfwGetTime();
    boidUniforms.modelMatrix = glm::scale(glm::mat4(1.f), glm::vec3(0.1f, 0.1f, 0.1f));
    boidUniforms.viewMatrix = camera.getViewMatrix();
    camera.debug();
    float aspect = static_cast<float>(gpu.getSurfaceConfig().width) / static_cast<float>(gpu.getSurfaceConfig().height);
    boidUniforms.projectionMatrix = camera.getProjectionMatrix(aspect);
    boidUniforms.cameraPosition = camera.getPosition();
    boidUniforms.color = { 0.2f, 0.1f, 0.3f, 1.f };
    boidUniforms.divideFlocks = params.divideFlocks;


    cubeUniforms.time = glfwGetTime();
    cubeUniforms.modelMatrix = glm::scale(glm::mat4(1.f), glm::vec3(params.cubeSize));
    cubeUniforms.viewMatrix = camera.getViewMatrix();
    cubeUniforms.projectionMatrix = boidUniforms.projectionMatrix;
    cubeUniforms.cameraPosition = camera.getPosition();
    cubeUniforms.color = { 0.2f, 0.1f, 0.3f, 1.f };

    queue.WriteBuffer(boidUniformBuffer, 0, &boidUniforms, sizeof(Uniforms));
    queue.WriteBuffer(cubeUniformBuffer, 0, &cubeUniforms, sizeof(Uniforms));


    wgpu::TextureView targetView = gpu.getNextSurfaceViewData();
    if (!targetView) return;


    queue.WriteBuffer(simulationParamsBuffer, 0, &params, sizeof(SimulationParams));
    // compute
    wgpu::CommandEncoderDescriptor encoderDesc = {};
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);

    wgpu::ComputePassDescriptor computePassDescriptor = {};
    wgpu::ComputePassEncoder computePass = encoder.BeginComputePass(&computePassDescriptor);
    
    computePass.SetPipeline(computePipeline);

    if (frameCount % 2 == 0) {
        computePass.SetBindGroup(0, computeBindGroupA, 0, nullptr);
    } else {
        computePass.SetBindGroup(0, computeBindGroupB, 0, nullptr);
    }

    uint32_t workgroupCount = (params.activeBoidsCount + 63) / 64;
    computePass.DispatchWorkgroups(workgroupCount, 1, 1);
    computePass.End();

    // Render
    wgpu::RenderPassColorAttachment colorAttachment = {};
    colorAttachment.view = gpu.getMsaaTextureView();
    colorAttachment.resolveTarget = targetView;
    colorAttachment.loadOp = wgpu::LoadOp::Clear;
    colorAttachment.storeOp = wgpu::StoreOp::Store;
    colorAttachment.clearValue = wgpu::Color{0.1f, 0.1f, 0.1f, 1.0f};

    wgpu::RenderPassDepthStencilAttachment depthAttachment{};
    depthAttachment.view = gpu.getDepthTextureView();
    depthAttachment.depthClearValue = 1.0f;
    depthAttachment.depthLoadOp = wgpu::LoadOp::Clear;
    depthAttachment.depthStoreOp = wgpu::StoreOp::Store;
    depthAttachment.depthReadOnly = false;

    wgpu::RenderPassDescriptor passDesc = {};
    passDesc.colorAttachmentCount = 1;
    passDesc.colorAttachments = &colorAttachment;
    passDesc.depthStencilAttachment = &depthAttachment;

    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);
    pass.SetPipeline(renderPipeline);
    pass.SetVertexBuffer(0, vertexBuffer);
    pass.SetIndexBuffer(indexBuffer, wgpu::IndexFormat::Uint32, 0, indexCount * sizeof(uint32_t));
    if (frameCount % 2 == 0) {
        pass.SetBindGroup(0, boidBindGroups[1], 0, nullptr); 
    } else {
        pass.SetBindGroup(0, boidBindGroups[0], 0, nullptr);
    }
    pass.DrawIndexed(indexCount, params.activeBoidsCount, 0, 0, 0);

    pass.SetPipeline(linePipeline);
    pass.SetVertexBuffer(0, cubeVertexBuffer);
    pass.SetIndexBuffer(cubeIndexBuffer, wgpu::IndexFormat::Uint32, 0, cubeIndexCount * sizeof(uint32_t));
    pass.SetBindGroup(0, cubeBindGroups[0], 0, nullptr);
    pass.DrawIndexed(cubeIndexCount, 1, 0, 0, 0);
    
    if (uiLayer.getShowVelocity()) {
        pass.SetPipeline(debugVelocityPipeline);
        if (frameCount % 2 == 0) pass.SetBindGroup(0, boidBindGroups[1], 0, nullptr);
        else pass.SetBindGroup(0, boidBindGroups[0], 0, nullptr);
        pass.Draw(2, params.activeBoidsCount, 0, 0);
    }
    if (uiLayer.getShowCoM()) {
        pass.SetPipeline(debugCoMPipeline);
        if (frameCount % 2 == 0) pass.SetBindGroup(0, boidBindGroups[1], 0, nullptr);
        else pass.SetBindGroup(0, boidBindGroups[0], 0, nullptr);
        pass.Draw(2, params.activeBoidsCount, 0, 0); 
    }

    pass.End();

    /* Render imGui UI */
    wgpu::RenderPassColorAttachment uiAttachment = {};
    uiAttachment.view = targetView;
    uiAttachment.resolveTarget = nullptr;
    uiAttachment.loadOp = wgpu::LoadOp::Load;
    uiAttachment.storeOp = wgpu::StoreOp::Store;

    wgpu::RenderPassDescriptor pass2Desc = {};
    pass2Desc.colorAttachmentCount = 1;
    pass2Desc.colorAttachments = &uiAttachment;
    pass2Desc.depthStencilAttachment = nullptr;

    wgpu::RenderPassEncoder pass2 = encoder.BeginRenderPass(&pass2Desc);
    uiLayer.draw(pass2);
    pass2.End();
    

    wgpu::CommandBufferDescriptor cmdBufferDesc = {};
    wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDesc);
    queue.Submit(1, &command);

    frameCount++;
}

void Renderer::initBuffers(const SimulationParams& params) {
    // Uniform buffer
    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.label = "Boid uniforms buffer";
    bufferDesc.mappedAtCreation = false;
    bufferDesc.nextInChain = nullptr;
    bufferDesc.size = sizeof(Uniforms);
    bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    boidUniformBuffer = device.CreateBuffer(&bufferDesc);

    bufferDesc.label = "Cube uniforms buffer";
    cubeUniformBuffer = device.CreateBuffer(&bufferDesc);
    
    // Cube buffer
    std::vector<VertexAttributes> cubeVertex(8);
    cubeVertex[0].position = {-1.0f, -1.0f, -1.0f};
    cubeVertex[1].position = { 1.0f, -1.0f, -1.0f};
    cubeVertex[2].position = { 1.0f, -1.0f,  1.0f};
    cubeVertex[3].position = {-1.0f, -1.0f,  1.0f};
    cubeVertex[4].position = {-1.0f,  1.0f, -1.0f};
    cubeVertex[5].position = { 1.0f,  1.0f, -1.0f};
    cubeVertex[6].position = { 1.0f,  1.0f,  1.0f};
    cubeVertex[7].position = {-1.0f,  1.0f,  1.0f};
    for (int i = 0; i < 8; ++i) {
        cubeVertex[i].color = {1.0f, 1.0f, 1.0f};
        cubeVertex[i].normal = {0.0f, 0.0f, 0.0f};
        cubeVertex[i].uv = {0.0f, 0.0f};
    }
    std::vector<uint32_t> cubeIndices = {
        0, 1,  1, 2,  2, 3,  3, 0,
        4, 5,  5, 6,  6, 7,  7, 4,
        0, 4,  1, 5,  2, 6,  3, 7
    };
    cubeIndexCount = static_cast<uint32_t>(cubeIndices.size());

    bufferDesc.label = "Cube Vertex buffer";
    bufferDesc.size = cubeVertex.size() * sizeof(VertexAttributes);
    bufferDesc.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
    cubeVertexBuffer = device.CreateBuffer(&bufferDesc);

    bufferDesc.label = "Cube Index Buffer";
    bufferDesc.size = cubeIndices.size() * sizeof(uint32_t);
    bufferDesc.usage = wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst;
    cubeIndexBuffer = device.CreateBuffer(&bufferDesc);

    // SimulationParamsBuffer
    wgpu::BufferDescriptor simParamsBufferDesc = {};
    simParamsBufferDesc.label = "Params buffer";
    simParamsBufferDesc.mappedAtCreation = false;
    simParamsBufferDesc.nextInChain = nullptr;
    simParamsBufferDesc.size = sizeof(SimulationParams);
    simParamsBufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
    simulationParamsBuffer = device.CreateBuffer(&simParamsBufferDesc);

    queue.WriteBuffer(cubeVertexBuffer, 0, cubeVertex.data(), cubeVertex.size() * sizeof(VertexAttributes));
    queue.WriteBuffer(cubeIndexBuffer, 0, cubeIndices.data(), cubeIndices.size() * sizeof(uint32_t));
}

void Renderer::initBindGroups(size_t boidsCount) {
    std::vector<wgpu::BindGroupLayoutEntry> bindingLayoutEntries(2);
    
    // Uniforms
    wgpu::BindGroupLayoutEntry& bindingLayoutEntry = bindingLayoutEntries[0];
    bindingLayoutEntry.binding = 0;
    bindingLayoutEntry.buffer.type = wgpu::BufferBindingType::Uniform;
    bindingLayoutEntry.nextInChain = nullptr;
    bindingLayoutEntry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
    bindingLayoutEntry.buffer.minBindingSize = sizeof(Uniforms);
    bindingLayoutEntry.buffer.hasDynamicOffset = false;

    // Boids
    wgpu::BindGroupLayoutEntry& BoidsBindingLayoutEntry = bindingLayoutEntries[1];
    BoidsBindingLayoutEntry.binding = 1;
    BoidsBindingLayoutEntry.buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
    BoidsBindingLayoutEntry.visibility = wgpu::ShaderStage::Vertex;
    BoidsBindingLayoutEntry.buffer.minBindingSize = sizeof(BoidData);
    BoidsBindingLayoutEntry.buffer.hasDynamicOffset = false;
    BoidsBindingLayoutEntry.nextInChain = nullptr;


    wgpu::BindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.nextInChain = nullptr;
    bindGroupLayoutDescriptor.entryCount = static_cast<uint32_t>(bindingLayoutEntries.size());
    bindGroupLayoutDescriptor.entries = bindingLayoutEntries.data();
    bindGroupLayouts.push_back(device.CreateBindGroupLayout(&bindGroupLayoutDescriptor));

    wgpu::PipelineLayoutDescriptor pipeLayoutDescriptor = {};
    pipeLayoutDescriptor.nextInChain = nullptr;
    pipeLayoutDescriptor.bindGroupLayoutCount = bindGroupLayouts.size();
    pipeLayoutDescriptor.bindGroupLayouts = bindGroupLayouts.data();
    pipelineLayout = device.CreatePipelineLayout(&pipeLayoutDescriptor);

    std::vector<wgpu::BindGroupEntry> bindingsA(2);

    bindingsA[0].binding = 0;
    bindingsA[0].buffer = boidUniformBuffer;
    bindingsA[0].offset = 0;
    bindingsA[0].size = sizeof(Uniforms);

    bindingsA[1].binding = 1;
    bindingsA[1].buffer = boidsBufferA;
    bindingsA[1].offset = 0;
    bindingsA[1].size = boidsCount * sizeof(BoidData);

    wgpu::BindGroupDescriptor bindGroupDescriptorA = {};
    bindGroupDescriptorA.nextInChain = nullptr;
    bindGroupDescriptorA.layout = bindGroupLayouts[0];
    bindGroupDescriptorA.entryCount = static_cast<uint32_t>(bindingsA.size());
    bindGroupDescriptorA.entries = bindingsA.data();
    boidBindGroups.push_back(device.CreateBindGroup(&bindGroupDescriptorA));

    std::vector<wgpu::BindGroupEntry> bindingsB(2);

    bindingsB[0].binding = 0;
    bindingsB[0].buffer = boidUniformBuffer;
    bindingsB[0].offset = 0;
    bindingsB[0].size = sizeof(Uniforms);

    bindingsB[1].binding = 1;
    bindingsB[1].buffer = boidsBufferB;
    bindingsB[1].offset = 0;
    bindingsB[1].size = boidsCount * sizeof(BoidData);

    wgpu::BindGroupDescriptor bindGroupDescriptorB = {};
    bindGroupDescriptorB.nextInChain = nullptr;
    bindGroupDescriptorB.layout = bindGroupLayouts[0];
    bindGroupDescriptorB.entryCount = static_cast<uint32_t>(bindingsB.size());
    bindGroupDescriptorB.entries = bindingsB.data();
    boidBindGroups.push_back(device.CreateBindGroup(&bindGroupDescriptorB));

    std::vector<wgpu::BindGroupEntry> cubeBindings(2);

    wgpu::BindGroupEntry& cubeBinding = cubeBindings[0];
    cubeBinding.binding = 0;
    cubeBinding.buffer = cubeUniformBuffer;
    cubeBinding.offset = 0;
    cubeBinding.size = sizeof(Uniforms);

    wgpu::BindGroupEntry& cubeBoidsT = cubeBindings[1];
    cubeBoidsT.binding = 1;
    cubeBoidsT.buffer = boidsBufferB; // заглушка
    cubeBoidsT.offset = 0;
    cubeBoidsT.size = boidsCount * sizeof(BoidData);


    wgpu::BindGroupDescriptor cubeBindGroupDescriptor = {};
    cubeBindGroupDescriptor.nextInChain = nullptr;
    cubeBindGroupDescriptor.layout = bindGroupLayouts[0];
    cubeBindGroupDescriptor.entryCount = static_cast<uint32_t>(cubeBindings.size());
    cubeBindGroupDescriptor.entries = cubeBindings.data();
    cubeBindGroups.push_back(device.CreateBindGroup(&cubeBindGroupDescriptor));
}

void Renderer::initRenderPipeline(wgpu::ShaderModule shader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat) {
    wgpu::PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
    wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&layoutDesc);

    wgpu::RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.layout = pipelineLayout;

    // (Position, Normal, Color, UV)
    std::vector<wgpu::VertexAttribute> vertexAttributes(4);
    vertexAttributes[0].shaderLocation = 0;
    vertexAttributes[0].format = wgpu::VertexFormat::Float32x3;
    vertexAttributes[0].offset = offsetof(VertexAttributes, position);

    //
    vertexAttributes[1].shaderLocation = 1;
    vertexAttributes[1].format = wgpu::VertexFormat::Float32x3;
    vertexAttributes[1].offset = offsetof(VertexAttributes, normal);
    vertexAttributes[2].shaderLocation = 2;
    vertexAttributes[2].format = wgpu::VertexFormat::Float32x3;
    vertexAttributes[2].offset = offsetof(VertexAttributes, color);
    vertexAttributes[3].shaderLocation = 3;
    vertexAttributes[3].format = wgpu::VertexFormat::Float32x2;
    vertexAttributes[3].offset = offsetof(VertexAttributes, uv);

    wgpu::VertexBufferLayout vertexBufferLayout{};
    vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttributes.size());
    vertexBufferLayout.attributes = vertexAttributes.data();
    vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
    pipelineDesc.vertex.module = shader;
    pipelineDesc.vertex.entryPoint = "vs_main";

    pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    pipelineDesc.primitive.cullMode = wgpu::CullMode::None;

    wgpu::BlendState blendState{};
    blendState.color.srcFactor = wgpu::BlendFactor::One;
    blendState.color.dstFactor = wgpu::BlendFactor::Zero;
    blendState.color.operation = wgpu::BlendOperation::Add;
    blendState.alpha.srcFactor = wgpu::BlendFactor::One;
    blendState.alpha.dstFactor = wgpu::BlendFactor::Zero;
    blendState.alpha.operation = wgpu::BlendOperation::Add;

    wgpu::ColorTargetState colorTarget{};
    colorTarget.format = surfaceFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = wgpu::ColorWriteMask::All;

    wgpu::FragmentState fragmentState{};
    fragmentState.module = shader;
    fragmentState.entryPoint = "fs_main";
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;

    wgpu::DepthStencilState depthStencilState{};
    depthStencilState.format = depthFormat;
    depthStencilState.depthWriteEnabled = true;
    depthStencilState.depthCompare = wgpu::CompareFunction::Less;
    pipelineDesc.depthStencil = &depthStencilState;

    pipelineDesc.multisample.count = 4;
    pipelineDesc.multisample.mask = ~0u;

    renderPipeline = device.CreateRenderPipeline(&pipelineDesc);
}

void Renderer::initLinePipeline(wgpu::ShaderModule shader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat) {
    wgpu::PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
    wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&layoutDesc);

    wgpu::RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.layout = pipelineLayout;

    // (Position, Normal, Color, UV)
    std::vector<wgpu::VertexAttribute> vertexAttributes(4);
    vertexAttributes[0].shaderLocation = 0;
    vertexAttributes[0].format = wgpu::VertexFormat::Float32x3;
    vertexAttributes[0].offset = offsetof(VertexAttributes, position);

    //
    vertexAttributes[1].shaderLocation = 1;
    vertexAttributes[1].format = wgpu::VertexFormat::Float32x3;
    vertexAttributes[1].offset = offsetof(VertexAttributes, normal);
    vertexAttributes[2].shaderLocation = 2;
    vertexAttributes[2].format = wgpu::VertexFormat::Float32x3;
    vertexAttributes[2].offset = offsetof(VertexAttributes, color);
    vertexAttributes[3].shaderLocation = 3;
    vertexAttributes[3].format = wgpu::VertexFormat::Float32x2;
    vertexAttributes[3].offset = offsetof(VertexAttributes, uv);

    wgpu::VertexBufferLayout vertexBufferLayout{};
    vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttributes.size());
    vertexBufferLayout.attributes = vertexAttributes.data();
    vertexBufferLayout.stepMode = wgpu::VertexStepMode::Vertex;

    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
    pipelineDesc.vertex.module = shader;
    pipelineDesc.vertex.entryPoint = "vs_cube";

    pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::LineList;
    pipelineDesc.primitive.cullMode = wgpu::CullMode::None;

    wgpu::BlendState blendState{};
    blendState.color.srcFactor = wgpu::BlendFactor::One;
    blendState.color.dstFactor = wgpu::BlendFactor::Zero;
    blendState.color.operation = wgpu::BlendOperation::Add;
    blendState.alpha.srcFactor = wgpu::BlendFactor::One;
    blendState.alpha.dstFactor = wgpu::BlendFactor::Zero;
    blendState.alpha.operation = wgpu::BlendOperation::Add;

    wgpu::ColorTargetState colorTarget{};
    colorTarget.format = surfaceFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = wgpu::ColorWriteMask::All;

    wgpu::FragmentState fragmentState{};
    fragmentState.module = shader;
    fragmentState.entryPoint = "fs_main";
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;

    wgpu::DepthStencilState depthStencilState{};
    depthStencilState.format = depthFormat;
    depthStencilState.depthWriteEnabled = true;
    depthStencilState.depthCompare = wgpu::CompareFunction::Less;
    pipelineDesc.depthStencil = &depthStencilState;

    pipelineDesc.multisample.count = 4;
    pipelineDesc.multisample.mask = ~0u;

    linePipeline = device.CreateRenderPipeline(&pipelineDesc);
}

void Renderer::setDefault(wgpu::BindGroupLayoutEntry &bindingLayout) {
    bindingLayout.buffer.nextInChain = nullptr;
    bindingLayout.buffer.type = wgpu::BufferBindingType::Undefined;
    bindingLayout.buffer.hasDynamicOffset = false;

    bindingLayout.sampler.nextInChain = nullptr;
    bindingLayout.sampler.type = wgpu::SamplerBindingType::Undefined;

    bindingLayout.storageTexture.nextInChain = nullptr;
    bindingLayout.storageTexture.access = wgpu::StorageTextureAccess::Undefined;
    bindingLayout.storageTexture.format = wgpu::TextureFormat::Undefined;
    bindingLayout.storageTexture.viewDimension =
        wgpu::TextureViewDimension::Undefined;

    bindingLayout.texture.nextInChain = nullptr;
    bindingLayout.texture.multisampled = false;
    bindingLayout.texture.sampleType = wgpu::TextureSampleType::Undefined;
    bindingLayout.texture.viewDimension = wgpu::TextureViewDimension::Undefined;
}

void Renderer::initDebugVelocityPipeline(wgpu::ShaderModule shader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat) {
    wgpu::PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
    wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&layoutDesc);

    wgpu::RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.layout = pipelineLayout;

    pipelineDesc.vertex.bufferCount = 0; 
    pipelineDesc.vertex.buffers = nullptr;
    pipelineDesc.vertex.module = shader;
    pipelineDesc.vertex.entryPoint = "vs_debug_velocity";

    pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::LineList;
    pipelineDesc.primitive.cullMode = wgpu::CullMode::None;

    wgpu::BlendState blendState{};
    blendState.color.srcFactor = wgpu::BlendFactor::One;
    blendState.color.dstFactor = wgpu::BlendFactor::Zero;
    blendState.color.operation = wgpu::BlendOperation::Add;
    blendState.alpha.srcFactor = wgpu::BlendFactor::One;
    blendState.alpha.dstFactor = wgpu::BlendFactor::Zero;
    blendState.alpha.operation = wgpu::BlendOperation::Add;

    wgpu::ColorTargetState colorTarget{};
    colorTarget.format = surfaceFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = wgpu::ColorWriteMask::All;

    wgpu::FragmentState fragmentState{};
    fragmentState.module = shader;
    fragmentState.entryPoint = "fs_main";
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;

    wgpu::DepthStencilState depthStencilState{};
    depthStencilState.format = depthFormat;
    depthStencilState.depthWriteEnabled = true;
    depthStencilState.depthCompare = wgpu::CompareFunction::Less;
    pipelineDesc.depthStencil = &depthStencilState;

    pipelineDesc.multisample.count = 4;
    pipelineDesc.multisample.mask = ~0u;

    debugVelocityPipeline = device.CreateRenderPipeline(&pipelineDesc);
}

void Renderer::initDebugCoMPipeline(wgpu::ShaderModule shader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat) {
    wgpu::PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
    wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&layoutDesc);

    wgpu::RenderPipelineDescriptor pipelineDesc{};
    pipelineDesc.layout = pipelineLayout;

    pipelineDesc.vertex.bufferCount = 0; 
    pipelineDesc.vertex.buffers = nullptr;
    pipelineDesc.vertex.module = shader;
    pipelineDesc.vertex.entryPoint = "vs_debug_com";

    pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::LineList;
    pipelineDesc.primitive.cullMode = wgpu::CullMode::None;

    wgpu::BlendState blendState{};
    blendState.color.srcFactor = wgpu::BlendFactor::One;
    blendState.color.dstFactor = wgpu::BlendFactor::Zero;
    blendState.color.operation = wgpu::BlendOperation::Add;
    blendState.alpha.srcFactor = wgpu::BlendFactor::One;
    blendState.alpha.dstFactor = wgpu::BlendFactor::Zero;
    blendState.alpha.operation = wgpu::BlendOperation::Add;

    wgpu::ColorTargetState colorTarget{};
    colorTarget.format = surfaceFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = wgpu::ColorWriteMask::All;

    wgpu::FragmentState fragmentState{};
    fragmentState.module = shader;
    fragmentState.entryPoint = "fs_main";
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;

    wgpu::DepthStencilState depthStencilState{};
    depthStencilState.format = depthFormat;
    depthStencilState.depthWriteEnabled = true;
    depthStencilState.depthCompare = wgpu::CompareFunction::Less;
    pipelineDesc.depthStencil = &depthStencilState;

    pipelineDesc.multisample.count = 4;
    pipelineDesc.multisample.mask = ~0u;

    debugCoMPipeline = device.CreateRenderPipeline(&pipelineDesc);
}

}; // namespace WGPUBoids