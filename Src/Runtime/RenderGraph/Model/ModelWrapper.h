#pragma once

#include "Runtime/RenderGraph/RenderGraph.h"
#include "Runtime/RenderGraph/Resource/RenderGraphHandle.h"
#include "Runtime/RenderGraph/Resource/RenderGraphResourceNode.h"
#include "Runtime/VulkanRHI/Graphic/Model.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Util/Modelutil.h"
#include <glm/fwd.hpp>
#include <map>
#include <memory>
namespace Render
{

class ModelWrapper
{
public:
    ModelWrapper(RHI::VulkanDevice* device, const boost::filesystem::path& modelPath);
    void registerToRenderGraph(RenderGraphBuilder& builder, RenderGraphRegistry& registry);

    void uploadBuffers(RenderGraphRegistry& registry);
    void updateUniformBuffer(RenderGraphRegistry& registry);

    void readByPassNode(RenderGraphBuilder& builder, RenderGraphRegistry& registry);

    inline Util::Math::SRTMatrix& getTransformation() { return m_transformation; }

private:
    void registerTextures(RenderGraphBuilder& builder, RenderGraphRegistry& registry);
    void registerBuffers(RenderGraphBuilder& builder, RenderGraphRegistry& registry);

    std::string getResourceNameInRenderGraph(const std::string& name) const;

private:
    RHI::VulkanDevice* m_device;
    std::string m_name;
    Util::Model::ModelData m_modelData;
    std::map<std::string, RenderGraphHandle<RHI::VulkanImageSampler>> m_samplerHandles;
    std::map<std::string, RenderGraphHandle<RHI::VulkanVertexBuffer>> m_vertexBufferHandles;
    std::map<std::string, RenderGraphHandle<RHI::VulkanVertexIndexBuffer>> m_indexBufferHandles;
    std::map<RenderGraphHandle<RHI::VulkanVertexBuffer>, std::vector<RenderGraphHandle<RHI::VulkanImageSampler>>>
        m_vertexBufferToSamplerHandles;
    RenderGraphHandle<RHI::VulkanBuffer> m_uniformBufferHandle{RenderGraphHandleBase::InValid};
    RHI::ModelUniformBufferObject m_uniformBufferObject;
    Util::Math::SRTMatrix m_transformation;
    glm::vec4 m_color = glm::vec4(1.0f);
};

}; // namespace Render