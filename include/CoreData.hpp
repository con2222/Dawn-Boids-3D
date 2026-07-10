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

static_assert(sizeof(Uniforms) % 16 == 0);