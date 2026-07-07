#include "Mesh.hpp"


namespace WGPUBoids {

Mesh::Mesh(std::vector<VertexAttributes> vertices, std::vector<uint32_t> indices, std::vector<SubMesh> subMeshes, std::vector<Material> materials) :
	vertexData(std::move(vertices)), 
	indexData(std::move(indices)), 
	subMeshes(std::move(subMeshes)),
	materials(materials)
{

}

} // namespace WGPUBoids
