#ifndef __cplusplus
  #error "C++ is required"
#endif

#ifndef TINY_RENDERER_MESH_H
#define TINY_RENDERER_MESH_H

#define GLM_FORCE_RADIANS 
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <string.h> 
#include <vector>

#ifndef TINYOBJLOADER_IMPLEMENTATION
  #define TINYOBJLOADER_IMPLEMENTATION
  #include <tiny_obj_loader.h>
#endif

#if defined(TINY_RENDERER_DX)
  #include "tinydx.h"
#elif defined(TINY_RENDERER_VK)
  #include "tinyvk.h"
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

  static tr_vertex_layout DefaultVertexLayout() {
    tr_vertex_layout vertex_layout = {};        
    // Attribute count
    vertex_layout.attrib_count = 3;
    // Position
    vertex_layout.attribs[0].semantic = tr_semantic_position;
    vertex_layout.attribs[0].format   = tr_format_r32g32b32_float;
    vertex_layout.attribs[0].binding  = 0;
    vertex_layout.attribs[0].location = 0;
    vertex_layout.attribs[0].offset   = 0;
    // Normal
    vertex_layout.attribs[1].semantic = tr_semantic_normal;
    vertex_layout.attribs[1].format   = tr_format_r32g32b32_float;
    vertex_layout.attribs[1].binding  = 0;
    vertex_layout.attribs[1].location = 1;   
    vertex_layout.attribs[1].offset   = vertex_layout.attribs[0].offset + tr_util_format_stride(vertex_layout.attribs[0].format);
    // Tex Coord
    vertex_layout.attribs[2].semantic = tr_semantic_texcoord0;
    vertex_layout.attribs[2].format   = tr_format_r32g32_float;
    vertex_layout.attribs[2].binding  = 0;
    vertex_layout.attribs[2].location = 2;
    vertex_layout.attribs[2].offset   = vertex_layout.attribs[1].offset + tr_util_format_stride(vertex_layout.attribs[1].format);
    // Return
    return vertex_layout;
  }

  const std::vector<uint32_t>& GetIndices() const {
    return m_indices; 
  }

  const std::vector<Vertex>& GetVertices() const { 
    return m_vertices; 
  }

  uint32_t GetIndexCount() const { 
    uint32_t count = (uint32_t)m_indices.size();
    return count;
  }

  uint32_t GetVertexStride() const {
    uint32_t stride = (uint32_t)sizeof(Vertex);
    return stride;
  }

  uint32_t GetVertexCount() const { 
    uint32_t count = (uint32_t)m_vertices.size();
    return count;
  }

  uint32_t GetVertexDataSize() const {
    uint32_t size = GetVertexStride() * GetVertexCount();
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

#endif // TINY_RENDERER_MESH_H