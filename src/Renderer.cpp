#include "Renderer.hpp"

namespace WGPUBoids {

bool Renderer::init(wgpu::Device dev, wgpu::Queue q, wgpu::ShaderModule shader, wgpu::TextureFormat surfaceFormat, wgpu::TextureFormat depthFormat) {
    device = dev;
    queue = q;

    wgpu::PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.bindGroupLayoutCount = 0;
    layoutDesc.bindGroupLayouts = nullptr;
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

void Renderer::draw(WebGPUContext &gpu) {
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

    pass.DrawIndexed(indexCount, 1, 0, 0, 0);
    pass.End();

    wgpu::CommandBufferDescriptor cmdBufferDesc = {};
    wgpu::CommandBuffer command = encoder.Finish(&cmdBufferDesc);
    queue.Submit(1, &command);
}

}; // namespace WGPUBoids