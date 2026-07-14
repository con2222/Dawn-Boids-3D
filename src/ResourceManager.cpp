#include "ResourceManager.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "std_image.h"

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>


bool operator==(const VertexAttributes& a, const VertexAttributes& b) {
    return a.position == b.position && a.normal == b.normal && a.uv == b.uv && a.color == b.color;
}

namespace WGPUBoids {

ResourceManager::ResourceManager(std::filesystem::path sPath, std::filesystem::path oPath) : shaderPath(sPath), objPath(oPath) {}

wgpu::ShaderModule ResourceManager::loadShaderModule(std::string_view shaderName, wgpu::Device device) {
    std::filesystem::path currentPath = std::filesystem::current_path();

    shaderPath = currentPath / "shaders" / shaderName;

    std::ifstream file(shaderPath);

    if (!file.is_open()) {
        std::cerr << "Error: Can't open shader code file at: " << shaderPath.string() << std::endl;
        return nullptr;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string shaderCode = buffer.str();

    if (shaderCode.empty()) {
        std::cerr << "Shader's code is empty" << std::endl;
        return nullptr;
    }

    wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
    wgslDesc.code = shaderCode.c_str();

    wgpu::ShaderModuleDescriptor shaderDesc{};
    shaderDesc.nextInChain = &wgslDesc;

    return device.CreateShaderModule(&shaderDesc);
}

Mesh ResourceManager::loadObj(std::string_view objFileName) {

    objPath /= objFileName;

    auto mtlPath = objPath.parent_path().string() + "/";



    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath.string().c_str(), mtlPath.c_str());
    std::cout << "Materials loaded: " << materials.size() << std::endl;
    
    std::cout << objPath.string() << std::endl;
    std::cout << mtlPath << std::endl;

    if (!warn.empty()) {
        std::cout << warn << std::endl;
    }

    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    if (!ret) {
        // WARN: need return nullptr
        return Mesh();
    }


    std::vector<VertexAttributes> vertexData;
    std::vector<uint32_t> indexData;
    std::vector<SubMesh> subMeshes;

    std::unordered_map<VertexAttributes, uint32_t, VertexHasher> uniqueVertices;

    for (const auto& shape : shapes) {

        int currentMaterialId = -1;
        uint32_t startIndex = static_cast<uint32_t>(indexData.size());
        uint32_t indexCount = 0;

        size_t index_offset = 0;

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) { // for each polygon
            int faceMaterialId = shape.mesh.material_ids[f];

            if (faceMaterialId != currentMaterialId && indexCount > 0) {
                subMeshes.push_back({ startIndex, indexCount, currentMaterialId });
                startIndex = static_cast<uint32_t>(indexData.size());
                indexCount = 0;
            }
            currentMaterialId = faceMaterialId;

            for (size_t v = 0; v < 3; v++) { // for each angle
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];

                VertexAttributes vertex{};
                vertex.position = {
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    -attrib.vertices[3 * idx.vertex_index + 2]
                };

                if (idx.normal_index >= 0) {
                    vertex.normal = {
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        -attrib.normals[3 * idx.normal_index + 2]
                    };
                }

                if (idx.texcoord_index >= 0) {
                    vertex.uv = {
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]
                    };
                }

                if (faceMaterialId >= 0) {
                    vertex.color = { materials[faceMaterialId].diffuse[0], materials[faceMaterialId].diffuse[1], materials[faceMaterialId].diffuse[2] };
                }
                else if (!attrib.colors.empty()) {
                    vertex.color = {
                        attrib.colors[3 * idx.vertex_index + 0],
                        attrib.colors[3 * idx.vertex_index + 1],
                        attrib.colors[3 * idx.vertex_index + 2]
                    };
                } else {
                    vertex.color = { 1.0f, 1.0f, 1.0f };
                }

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertexData.size());
                    vertexData.push_back(vertex);
                }
                indexData.push_back(uniqueVertices[vertex]);
                indexCount++;
            }
            index_offset += 3;
        }
        if (indexCount > 0) {
            subMeshes.push_back({ startIndex, indexCount, currentMaterialId });
        }
    }

    std::cout << "Loaded model: " << vertexData.size() << " vertices, "
        << indexData.size() << " indices, "
        << subMeshes.size() << " submeshes." << std::endl;

    std::vector<Material> parsedMaterials;

    for (const auto& tinyMat : materials) {
        Material mat;
        mat.diffuseColor = glm::vec3(
            tinyMat.diffuse[0],
            tinyMat.diffuse[1],
            tinyMat.diffuse[2]
        );
        parsedMaterials.push_back(mat);
    }

    return Mesh(vertexData, indexData, subMeshes, parsedMaterials);
}

} // namespace WGPUBoids
