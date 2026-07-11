struct SimulationParams {
    deltaTime: f32,
    visualRange: f32,
    protectedRange: f32,
    maxSpeed: f32,
    minSpeed: f32,
    cubeSize: f32,
    cohesionFactor: f32,
    alignmentFactor: f32,
    separationFactor: f32,
}

struct BoidData {
    position: vec4<f32>,
    velocity: vec4<f32>,
};

@group(0) @binding(0) var<uniform> params : SimulationParams;
@group(0) @binding(1) var<storage, read> boidsIn: array<BoidData>;
@group(0) @binding(2) var<storage, read_write> boidsOut: array<BoidData>;

@compute @workgroup_size(64)
fn compute_main(@builtin(global_invocation_id) GlobalInvocationID : vec3<u32>) {
    let index = GlobalInvocationID.x;
    
    if (index >= arrayLength(&boidsIn)) {
        return;
    }

    var currentBoid = boidsIn[index];
    let boidCount = arrayLength(&boidsIn);

    var numNeighbors = 0u;
    var centerOfMass = vec4<f32>(0.0);
    var avgVelocity = vec4<f32>(0.0);
    var separationVector = vec4<f32>(0.0);

    for (var j: u32 = 0; j < boidCount; j = j + 1u) {
        if (j == index) { continue; }

        var dist = distance(currentBoid.position.xyz, boidsIn[j].position.xyz);
        if (dist <= params.visualRange) {
            numNeighbors += 1u;
            centerOfMass += boidsIn[j].position;
            avgVelocity += boidsIn[j].velocity;
            if (dist <= params.protectedRange) {
                separationVector += currentBoid.position - boidsIn[j].position;
            }
        }

    }

    if (numNeighbors > 0u) {
        let nFloat = f32(numNeighbors);
        centerOfMass /= nFloat;
        avgVelocity /= nFloat;

        currentBoid.velocity += (centerOfMass - currentBoid.position) * params.cohesionFactor; // factor
        currentBoid.velocity += (avgVelocity - currentBoid.velocity) * params.alignmentFactor;
        currentBoid.velocity += separationVector * params.separationFactor;
    }   

    var velocityLength = length(currentBoid.velocity.xyz);
    let dir = normalize(currentBoid.velocity.xyz);
    if (velocityLength > params.maxSpeed) {
        currentBoid.velocity = vec4<f32>(dir * params.maxSpeed, 0.0);
    } else if (velocityLength < params.minSpeed) {
        currentBoid.velocity = vec4<f32>(dir * params.minSpeed, 0.0);
    }

    currentBoid.position += currentBoid.velocity * params.deltaTime;

    let margin = 0.2; 
    let limit = params.cubeSize - margin;

    if (currentBoid.position.x > limit) {
        currentBoid.position.x = limit;
        currentBoid.velocity.x *= -1.0;
    } else if (currentBoid.position.x < -limit) {
        currentBoid.position.x = -limit;
        currentBoid.velocity.x *= -1.0;
    }

    if (currentBoid.position.y > limit) {
        currentBoid.position.y = limit;
        currentBoid.velocity.y *= -1.0;
    } else if (currentBoid.position.y < -limit) {
        currentBoid.position.y = -limit;
        currentBoid.velocity.y *= -1.0;
    }

    if (currentBoid.position.z > limit) {
        currentBoid.position.z = limit;
        currentBoid.velocity.z *= -1.0;
    } else if (currentBoid.position.z < -limit) {
        currentBoid.position.z = -limit;
        currentBoid.velocity.z *= -1.0;
    }

    boidsOut[index] = currentBoid;
}