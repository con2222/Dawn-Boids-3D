#pragma once


struct Uniforms {
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::vec4 color;
    glm::vec3 cameraPosition;
    float time;
};

struct BoidData {
    glm::vec4 position = {0.f, 0.f, 0.f, 0.f};
    glm::vec4 velocity;

    BoidData(glm::vec4 a, glm::vec4 b) : position(a), velocity(b) {}
};

struct SimulationParams {
    float deltaTime;
    float visualRange = 2.0f;
    float protectedRange = 0.5f;
    float maxSpeed = 5.0f;
    float minSpeed = 2.0f;
    float cubeSize = 4.5f;
    float cohesionFactor = 0.005f;
    float alignmentFactor = 0.05f;
    float separationFactor = 0.05f;
    float turnFactor = 0.05f;
    float visionRadius = 240;
    float pad;
};

static_assert(sizeof(Uniforms) % 16 == 0);
static_assert(sizeof(BoidData) % 16 == 0);
static_assert(sizeof(SimulationParams) % 16 == 0);