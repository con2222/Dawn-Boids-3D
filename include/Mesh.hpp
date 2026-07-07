#pragma once

#include "glm/glm.hpp"

// std
#include <vector>
#include <string>


struct VertexAttributes {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uv;
};

struct SubMesh {
    uint32_t startIndex;;
    uint32_t indexCount;
    int materialId;
};

struct Material {
    glm::vec3 diffuseColor{ 1.0f, 1.0f, 1.0f };
    std::string diffuseTexture;
};

namespace WGPUBoids {

class Mesh {
public:
    Mesh() = default;

    Mesh(std::vector<VertexAttributes> vertices, std::vector<uint32_t> indices, std::vector<SubMesh> subMeshes, std::vector<Material> materials);

private:
    std::vector<VertexAttributes> vertexData;
    std::vector<uint32_t> indexData;
    std::vector<SubMesh> subMeshes;
    std::vector<Material> materials;
public:
    const std::vector<VertexAttributes>& getVertexData() const { return vertexData; }
    const std::vector<uint32_t>& getIndexData() const { return indexData; }
    const std::vector<SubMesh>& getSubMeshes() const { return subMeshes; }
    const std::vector<Material>& getMaterials() const { return materials; }
};


} // namespace WGPUBoids
