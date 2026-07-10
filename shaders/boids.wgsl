
const pi: f32 = 3.14159265359;

struct Uniforms {
    modelMatrix: mat4x4f,
    viewMatrix: mat4x4f,
    projectionMatrix: mat4x4f,
    color: vec4f,
    cameraPosition: vec3f,
    time: f32,
};

struct BoidData {
    position: vec4<f32>,
    velocity: vec4<f32>,
};

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) color: vec3<f32>,
    @location(3) uv: vec2<f32>,
};

struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) color: vec3<f32>,
};

@group(0) @binding(0) var<uniform> uniforms: Uniforms;
@group(0) @binding(1) var<storage, read> boidsData: array<BoidData>;

@vertex
fn vs_main(model: VertexInput, @builtin(instance_index) instanceIdx: u32) -> VertexOutput {
    var out: VertexOutput;
    
    let currentBoid = boidsData[instanceIdx];
    
    let scaledPos = uniforms.modelMatrix * vec4<f32>(model.position, 1.0);
    let worldPos = vec4<f32>(scaledPos.xyz + currentBoid.position.xyz, 1.0);

    out.clip_position = uniforms.projectionMatrix * uniforms.viewMatrix * worldPos;
    
    //out.color = uniforms.color.xyz;
    out.color = model.color;

    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return vec4<f32>(in.color, 1.0);
}