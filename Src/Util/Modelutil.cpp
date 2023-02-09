#include "Modelutil.h"
#include "Runtime/VulkanRHI/Graphic/Vertex.h"
#include "Util/Fileutil.h"
#include <glm/fwd.hpp>
#include <stdexcept>

#include <unordered_map>
#include <vector>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>


namespace Util {

Model::TinyObj::TinyObj(const boost::filesystem::path& objPath)
{
    if (!Util::File::fileExist(objPath))
    {
        throw std::runtime_error("load model failed, File not exist: " + objPath.string() );
        return;
    }

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn,err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath.string().c_str()))
    {
        throw std::runtime_error("load model failed: " + warn + err);
    }

    std::unordered_map<RHI::Vertex, uint32_t> uniqueVertices;
    for (auto& shape : shapes)
    {
        for (auto& index : shape.mesh.indices)
        {
            RHI::Vertex vertex;

            vertex.position = glm::vec3
            {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = glm::vec2
            {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = glm::vec3(1.0f);

            if (uniqueVertices.find(vertex) == uniqueVertices.end())
            {
                uniqueVertices[vertex] = m_verticies.size();
                m_verticies.emplace_back(vertex);
            }
            m_indices.emplace_back(uniqueVertices[vertex]);
        }
    }
}

}