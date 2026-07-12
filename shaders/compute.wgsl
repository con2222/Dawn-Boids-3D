struct SimulationParams {
    deltaTime: f32,
    visualRange: f32,
    protectedRange: f32,
    strangerProtectedRange: f32,
    maxSpeed: f32,
    minSpeed: f32,
    cubeSize: f32,
    cohesionFactor: f32,
    alignmentFactor: f32,
    separationFactor: f32,
    turnFactor: f32,
    strangeForceFactor: f32,
    visionRadius: f32,
    margin: f32,
    activeBoidsCount: u32,
    divideFlocks: u32,
}

struct BoidData {
    position: vec4<f32>,
    velocity: vec4<f32>,
    centerOfMass: vec4<f32>,
};

@group(0) @binding(0) var<uniform> params : SimulationParams;
@group(0) @binding(1) var<storage, read> boidsIn: array<BoidData>;
@group(0) @binding(2) var<storage, read_write> boidsOut: array<BoidData>;

@compute @workgroup_size(64)
fn compute_main(@builtin(global_invocation_id) GlobalInvocationID : vec3<u32>) {
    let index = GlobalInvocationID.x;
    
    if (index >= params.activeBoidsCount) {
        return;
    }

    var currentBoid = boidsIn[index];
    let flockId = currentBoid.position.w;
    let boidCount = arrayLength(&boidsIn);

    var numNeighbors = 0u;
    var centerOfMass = vec4<f32>(0.0);
    var avgVelocity = vec4<f32>(0.0);
    var separationVector = vec4<f32>(0.0);

    let boidDir = normalize(currentBoid.velocity.xyz);
    for (var j: u32 = 0; j < params.activeBoidsCount; j = j + 1u) {
        if (j == index) { continue; }

        let diff = boidsIn[j].position.xyz - currentBoid.position.xyz;
        let dist = length(diff);
        if (dist < 0.0001) { continue; }
        
        let toNeighborDir = diff / dist;

        if (dot(boidDir, toNeighborDir) > params.visionRadius) {
            if (dist <= params.visualRange) {
                let isFriendly = (params.divideFlocks == 0u) || (boidsIn[j].position.w == flockId);
                if (isFriendly) {
                    numNeighbors += 1u;
                    centerOfMass += boidsIn[j].position;
                    avgVelocity += boidsIn[j].velocity;

                    if (dist <= params.protectedRange) {
                        let force = -diff;
                        separationVector += vec4<f32>(force / (dist * dist), 0.0);
                    }
                } else {
                    if (dist <= params.strangerProtectedRange) {
                        let force = -diff * params.strangeForceFactor; // new param
                        separationVector += vec4<f32>(force / (dist * dist), 0.0);
                        
                    }
                }
                
            }
        }
    }

    if (numNeighbors > 0u) {
        let nFloat = f32(numNeighbors);
        centerOfMass /= nFloat;
        avgVelocity /= nFloat;

        let desiredCohesion = normalize(centerOfMass.xyz - currentBoid.position.xyz) * params.maxSpeed;
        let steerCohesion = desiredCohesion - currentBoid.velocity.xyz;
        currentBoid.velocity += vec4<f32>(steerCohesion * params.cohesionFactor, 0.0);

        let oldCoM = currentBoid.centerOfMass.xyz;
        let smoothCoM = mix(oldCoM, centerOfMass.xyz, 0.1);
        currentBoid.centerOfMass = vec4<f32>(smoothCoM, 1.0);

        let desiredAlignment = normalize(avgVelocity.xyz) * params.maxSpeed;
        let steerAlignment = desiredAlignment - currentBoid.velocity.xyz;
        currentBoid.velocity += vec4<f32>(steerAlignment * params.alignmentFactor, 0.0);
    } else {
        currentBoid.centerOfMass = currentBoid.position;
    }

    currentBoid.velocity += separationVector * params.separationFactor;

    var velocityLength = length(currentBoid.velocity.xyz);
    let dir = normalize(currentBoid.velocity.xyz);
    if (velocityLength > params.maxSpeed) {
        currentBoid.velocity = vec4<f32>(dir * params.maxSpeed, 0.0);
    } else if (velocityLength < params.minSpeed) {
        currentBoid.velocity = vec4<f32>(dir * params.minSpeed, 0.0);
    }

    if (currentBoid.position.x < -params.cubeSize + params.margin) {
        currentBoid.velocity.x += params.turnFactor;
    } else if (currentBoid.position.x > params.cubeSize - params.margin) {
        currentBoid.velocity.x -= params.turnFactor;
    }

    if (currentBoid.position.y < -params.cubeSize + params.margin) {
        currentBoid.velocity.y += params.turnFactor;
    } else if (currentBoid.position.y > params.cubeSize - params.margin) {
        currentBoid.velocity.y -= params.turnFactor;
    }

    if (currentBoid.position.z < -params.cubeSize + params.margin) {
        currentBoid.velocity.z += params.turnFactor;
    } else if (currentBoid.position.z > params.cubeSize - params.margin) {
        currentBoid.velocity.z -= params.turnFactor;
    }

    currentBoid.position += vec4<f32>(currentBoid.velocity.xyz * params.deltaTime, 0.0);
    boidsOut[index] = currentBoid;
}