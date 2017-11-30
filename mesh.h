#ifndef __cplusplus
  #error "C++ is required"
#endif

#ifndef MESH_H
#define MESH_H

#define GLM_FORCE_RADIANS 
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string.h>
#include <vector>

#ifndef TINYOBJLOADER_IMPLEMENTATION
  #define TINYOBJLOADER_IMPLEMENTATION
  #include <tiny_obj_loader.h>
#endif

namespace tr {

using float2   = glm::vec2;
using float3   = glm::vec3;
using float4   = glm::vec4;
using float2x2 = glm::mat2x2;
using float2x3 = glm::mat2x3;
using float2x4 = glm::mat2x4;
using float3x2 = glm::mat3x2;
using float3x3 = glm::mat3x3;
using float3x4 = glm::mat3x4;
using float4x2 = glm::mat4x2;
using float4x3 = glm::mat4x3;
using float4x4 = glm::mat4x4;

struct Vertex {
  float3 position;
  float3 normal;
  float2 tex_coord;
};

class Mesh {
public:
  Mesh() {}
  ~Mesh() {}

  const std::vector<uint32_t>& GetIndices() const {
    return m_indices; 
  }

  const std::vector<Vertex>& GetVertices() const { 
    return m_vertices; 
  }

  size_t GetIndexCount() const { 
    size_t count = m_indices.size();
    return count;
  }

  size_t GetVertexStride() const {
    size_t stride = sizeof(Vertex);
    return stride;
  }

  size_t GetVertexCount() const { 
    size_t count = m_vertices.size();
    return count;
  }

  size_t GetVertexDataSize() const {
    size_t size = GetVertexStride() * GetVertexCount();
    return size;
  }

  const Vertex* GetVertexData() const {
    const Vertex* p_data = m_vertices.data();
    return p_data;
  }

  static bool Load(const std::string& file_path, Mesh* p_mesh) {
    if (p_mesh == nullptr) {
      return false;
    }

    p_mesh->m_indices.clear();
    p_mesh->m_vertices.clear();
    
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;
    std::string                       err;
    bool                              triangulate = true;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, file_path.c_str(), nullptr, triangulate);
    if (!ret || shapes.empty() || attrib.vertices.empty()) {
      return false;
    }

    size_t element_count = shapes[0].mesh.indices.size(); 
    p_mesh->m_vertices.resize(element_count);

    Vertex* p_vertex = p_mesh->m_vertices.data();
    for (const auto& index : shapes[0].mesh.indices) {
      // Position
      size_t vertex_index = 3 * index.vertex_index;
      p_vertex->position.x = attrib.vertices[vertex_index + 0];
      p_vertex->position.y = attrib.vertices[vertex_index + 1];
      p_vertex->position.z = attrib.vertices[vertex_index + 2];
      // Normal
      size_t normal_index = 3 * index.normal_index;
      p_vertex->normal.x = attrib.normals.empty() ? 0.0f : attrib.normals[normal_index + 0];
      p_vertex->normal.y = attrib.normals.empty() ? 0.0f : attrib.normals[normal_index + 1];
      p_vertex->normal.z = attrib.normals.empty() ? 0.0f : attrib.normals[normal_index + 2];
      // Tex coord
      size_t tex_coord_index = 2 * index.texcoord_index;
      p_vertex->tex_coord.x = attrib.texcoords.empty() ? 0.0f : attrib.texcoords[tex_coord_index  + 0];
      p_vertex->tex_coord.y = attrib.texcoords.empty() ? 0.0f : attrib.texcoords[tex_coord_index  + 1];
      // Next vertex
      ++p_vertex;
    }
  
    return true;
  }

private:
  std::vector<uint32_t> m_indices;
  std::vector<Vertex>   m_vertices;
};

} // namespace mesh

#endif // MESH_H