#include "Light.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/RenderPass/ShadowMapRenderPass.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"

using namespace Render;

Lights::Lights(RHI::VulkanDevice* device,  int lightNum, bool useShadowPass)
    : m_pDevice(device)
    , m_lightNum(lightNum)
{
    initLightUBO();
    m_transformation.resize(lightNum);
    m_color.resize(lightNum, glm::vec4(1.0f));
    if (useShadowPass)
    {
        m_shadowmapPass.reset(new RHI::ShadowMapRenderPass(device, m_lightNum));
    }
}

Lights::Lights(RHI::VulkanDevice* device, const std::vector<Util::Math::VPMatrix>& transformation, bool useShadowPass)
    : m_pDevice(device)
    , m_lightNum(transformation.size())
    , m_transformation(transformation)
{
    initLightUBO();
    m_color.resize(m_lightNum, glm::vec4(1.0f));
    if (useShadowPass)
    {
        m_shadowmapPass.reset(new RHI::ShadowMapRenderPass(device, m_lightNum));
    }
}

Lights::~Lights()
{
    m_lightUniformBuffers.reset();
    m_shadowmapPass.reset();
}

RHI::Model::UBOLayoutInfo Lights::GetUboInfo()
{
    RHI::Model::UBOLayoutInfo uboInfo = RHI::Model::UBOLayoutInfo
    {
        m_lightUniformBuffers.get(),
        RHI::VulkanDescriptorSetLayout::DESCRIPTOR_LIGHTUBO_BINDING_ID,
        sizeof(RHI::LightInforUniformBufferObject)
    };
    return uboInfo;
}

void Lights::UpdateLightUBO()
{
    ZoneScopedN("Lights::UpdateLightUBO");;
    bool uboDirty = m_lightUniformBufferObjects.lightNum != m_lightNum;

    RHI::LightInforUniformBufferObject ubo;
    ubo.lightNum = m_lightNum;

    for (int i = 0; i < m_lightNum; i++)
    {
        ubo.viewProjMatrix[i] = m_transformation[i].GetProjMatrix() * m_transformation[i].GetViewMatrix();
        ubo.position[i] = glm::vec4(m_transformation[i].GetPosition(), 1.0f);
        ubo.color[i] = m_color[i];
        ubo.nearFar[i] = glm::vec4(m_transformation[i].GetNear(),m_transformation[i].GetFar(),0,0);
        ubo.direction[i] = glm::vec4(m_transformation[i].GetFrontDir(), 0.0f);
        uboDirty = (
            uboDirty
            || ubo.viewProjMatrix[i] != m_lightUniformBufferObjects.viewProjMatrix[i]
            || ubo.position[i] != m_lightUniformBufferObjects.position[i]
            || ubo.color[i] != m_lightUniformBufferObjects.color[i]
            || ubo.nearFar[i] != m_lightUniformBufferObjects.nearFar[i]);
    }

    if (uboDirty)
    {
        m_lightUniformBufferObjects = ubo;
        m_lightUniformBuffers->FillingMappingBuffer(&ubo, 0, sizeof(ubo));
    }
}

void Lights::initLightUBO()
{

    m_lightUniformBuffers.reset(
        new RHI::VulkanBuffer(
                m_pDevice, sizeof(RHI::LightInforUniformBufferObject),
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                vk::SharingMode::eExclusive
            )
        );

    m_color.resize(m_lightNum);
}
