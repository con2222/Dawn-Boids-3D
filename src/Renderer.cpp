#include "Renderer.hpp"
#include "CoreData.hpp"
#include "Camera.hpp"

#include "glm/gtc/matrix_transform.hpp"

namespace WGPUBoids {

bool Renderer::init(wgpu::Device dev, wgpu::Queue q, wgpu::ShaderModule shader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat) {
    device = dev;
    queue = q;

    initBuffers();
    initBindGroups();
    initRenderPipeline(shader, surfaceFormat, depthFormat);
    initLinePipeline(shader, surfaceFormat, depthFormat);

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

        wgpu::BufferDescriptor bufferDesc{};
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

        wgpu::BufferDescriptor indexBufferDesc{};
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

void Renderer::draw(WebGPUContext &gpu, const Camera& camera) {
    device.Tick();

    Uniforms boidUniforms, cubeUniforms;
    boidUniforms.time = glfwGetTime();
    boidUniforms.modelMatrix = glm::scale(glm::mat4(1.f), glm::vec3(0.1f, 0.1f, 0.1f));
    boidUniforms.viewMatrix = camera.getViewMatrix();
    camera.debug();
    float aspect = static_cast<float>(gpu.getSurfaceConfig().width) / static_cast<float>(gpu.getSurfaceConfig().height);
    boidUniforms.projectionMatrix = camera.getProjectionMatrix(aspect);
    boidUniforms.cameraPosition = camera.getPosition();
    boidUniforms.color = { 0.2f, 0.1f, 0.3f, 1.f };


    cubeUniforms.time = glfwGetTime();
    cubeUniforms.modelMatrix = glm::mat4(1.f);
    cubeUniforms.viewMatrix = camera.getViewMatrix();
    cubeUniforms.projectionMatrix = boidUniforms.projectionMatrix;
    cubeUniforms.cameraPosition = camera.getPosition();
    cubeUniforms.color = { 0.2f, 0.1f, 0.3f, 1.f };

    queue.WriteBuffer(boidUniformBuffer, 0, &boidUniforms, sizeof(Uniforms));
    queue.WriteBuffer(cubeUniformBuffer, 0, &cubeUniforms, sizeof(Uniforms));


    wgpu::TextureView targetView = gpu.getNextSurfaceViewData();
    if (!targetView) return;

    wgpu::CommandEncoderDescriptor encoderDesc = {};
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);

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
    pass.SetBindGroup(0, boidBindGroups[0], 0, nullptr);
    pass.DrawIndexed(indexCount, 1, 0, 0, 0);

    pass.SetPipeline(linePipeline);
    pass.SetVertexBuffer(0, cubeVertexBuffer);
    pass.SetIndexBuffer(cubeIndexBuffer, wgpu::IndexFormat::Uint32, 0, cubeIndexCount * sizeof(uint32_t));
    pass.SetBindGroup(0, cubeBindGroups[0], 0, nullptr);
    pass.DrawIndexed(cubeIndexCount, 1, 0, 0, 0);
    pass.End();

    wgpu::CommandBufferDescriptor cmdBufferDesc = {};
    wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDesc);
    queue.Submit(1, &command);
}

void Renderer::initBuffers() {
    wgpu::BufferDescriptor bufferDesc;
    bufferDesc.label = "Boid uniforms buffer";
    bufferDesc.mappedAtCreation = false;
    bufferDesc.nextInChain = nullptr;
    bufferDesc.size = sizeof(Uniforms);
    bufferDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    boidUniformBuffer = device.CreateBuffer(&bufferDesc);

    bufferDesc.label = "Cube uniforms buffer";
    cubeUniformBuffer = device.CreateBuffer(&bufferDesc);
    

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

    queue.WriteBuffer(cubeVertexBuffer, 0, cubeVertex.data(), cubeVertex.size() * sizeof(VertexAttributes));
    queue.WriteBuffer(cubeIndexBuffer, 0, cubeIndices.data(), cubeIndices.size() * sizeof(uint32_t));
}

void Renderer::initBindGroups() {
    std::vector<wgpu::BindGroupLayoutEntry> bindingLayoutEntries(1);
    
    wgpu::BindGroupLayoutEntry& bindingLayoutEntry = bindingLayoutEntries[0];
    bindingLayoutEntry.binding = 0;
    bindingLayoutEntry.buffer.type = wgpu::BufferBindingType::Uniform;
    bindingLayoutEntry.nextInChain = nullptr;
    bindingLayoutEntry.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
    bindingLayoutEntry.buffer.minBindingSize = sizeof(Uniforms);
    bindingLayoutEntry.buffer.hasDynamicOffset = false;

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

    std::vector<wgpu::BindGroupEntry> bindings(1);

    bindings[0].binding = 0;
    bindings[0].buffer = boidUniformBuffer;
    bindings[0].offset = 0;
    bindings[0].size = sizeof(Uniforms);

    wgpu::BindGroupDescriptor bindGroupDescriptor = {};
    bindGroupDescriptor.nextInChain = nullptr;
    bindGroupDescriptor.layout = bindGroupLayouts[0];
    bindGroupDescriptor.entryCount = static_cast<uint32_t>(bindings.size());
    bindGroupDescriptor.entries = &bindings[0];
    boidBindGroups.push_back(device.CreateBindGroup(&bindGroupDescriptor));

    wgpu::BindGroupEntry cubeBinding;
    cubeBinding.binding = 0;
    cubeBinding.buffer = cubeUniformBuffer;
    cubeBinding.offset = 0;
    cubeBinding.size = sizeof(Uniforms);

    wgpu::BindGroupDescriptor cubeBindGroupDescriptor = {};
    cubeBindGroupDescriptor.nextInChain = nullptr;
    cubeBindGroupDescriptor.layout = bindGroupLayouts[0];
    cubeBindGroupDescriptor.entryCount = 1;
    cubeBindGroupDescriptor.entries = &cubeBinding;
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
    pipelineDesc.vertex.entryPoint = "vs_main";

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

}; // namespace WGPUBoids