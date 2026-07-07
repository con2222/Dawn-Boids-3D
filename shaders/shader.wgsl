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

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    
    let x0 = in.position.x;
    let y0 = in.position.y;
    let z0 = in.position.z;
    
    let cY = cos(0.8);
    let sY = sin(0.8);
    let x1 = x0 * cY + z0 * sY;
    let y1 = y0;
    let z1 = -x0 * sY + z0 * cY;
    
    let cX = cos(0.6);
    let sX = sin(0.6);
    let x2 = x1;
    let y2 = y1 * cX - z1 * sX;
    let z2 = y1 * sX + z1 * cX;
    
    var final_x = x2 * 0.5;
    var final_y = y2 * 0.5;
    var final_z = z2 * 0.5;
    
    final_x = final_x * 0.5625; // 1080 / 1920
    
    final_z = final_z + 0.5;
    
    out.clip_position = vec4<f32>(final_x, final_y, final_z, 1.0);
    out.color = in.color;
    
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    return vec4<f32>(in.color, 1.0);
}