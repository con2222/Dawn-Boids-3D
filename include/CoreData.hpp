#pragma once


struct Uniforms {
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::vec4 color;
    glm::vec3 cameraPosition;
    float time;
};

static_assert(sizeof(Uniforms) % 16 == 0);