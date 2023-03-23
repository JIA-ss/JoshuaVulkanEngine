#include "Light.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/RenderPass/ShadowMapRenderPass.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <iostream>
#include <vcruntime_string.h>

using namespace Render;

Lights::Lights(RHI::VulkanDevice* device,  int lightNum, bool useShadowPass)
    : m_pDevice(device)
    , m_lightNum(lightNum)
    , m_customLightUBO()
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
    , m_customLightUBO()
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
    m_pCustomUniformBuffer.reset();
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
RHI::Model::UBOLayoutInfo Lights::GetLargeUboInfo()
{
    RHI::Model::UBOLayoutInfo uboInfo = RHI::Model::UBOLayoutInfo
    {
        m_pCustomUniformBuffer.get(),
        RHI::VulkanDescriptorSetLayout::DESCRIPTOR_CUSTOMUBO_BINDING_ID,
        m_customLightUBO->GetSize()
    };
    return uboInfo;
}

void Lights::UpdateLightUBO()
{
    bool uboDirty = false;
    if (m_lightUniformBufferObjects.lightNum != m_lightNum)
    {
        if (m_customLightUBO)
        {
            m_customLightUBO->viewProjMatrix.resize(m_lightNum);
            m_customLightUBO->position_near.resize(m_lightNum);
            m_customLightUBO->color_far.resize(m_lightNum);
            m_customLightUBO->direction.resize(m_lightNum);
        }
        uboDirty = true;
    }

    // update small lightInfo
    {
        RHI::LightInforUniformBufferObject ubo;
        ubo.lightNum = m_lightNum;
        for (int i = 0; i < m_lightNum && i < 5; i++)
        {
            ubo.viewProjMatrix[i] = m_transformation[i].GetProjMatrix() * m_transformation[i].GetViewMatrix();
            ubo.position[i] = glm::vec4(m_transformation[i].GetPosition(), 1.0f);
            ubo.color[i] = m_color[i];
            ubo.nearFar[i] = glm::vec4(m_transformation[i].GetNear(),m_transformation[i].GetFar(),0,0);
            ubo.direction[i] = glm::vec4(m_transformation[i].GetFrontDir(), 0.0f);
            bool dirty = (
                            ubo.viewProjMatrix[i] != m_lightUniformBufferObjects.viewProjMatrix[i]
                            || ubo.position[i] != m_lightUniformBufferObjects.position[i]
                            || ubo.color[i] != m_lightUniformBufferObjects.color[i]
                            || ubo.nearFar[i] != m_lightUniformBufferObjects.nearFar[i]);
            uboDirty |= dirty;
        }
        if (uboDirty)
        {
            m_lightUniformBufferObjects = ubo;
            m_lightUniformBuffers->FillingMappingBuffer(&ubo, 0, sizeof(ubo));
        }
    }

    // update big(custom) lightInfo
    if (m_lightNum > 5)
    {
        for (int i = 0; i < m_lightNum; i++)
        {
            glm::mat4 viewProjMatrix = m_transformation[i].GetProjMatrix() * m_transformation[i].GetViewMatrix();
            glm::vec4 position = glm::vec4(m_transformation[i].GetPosition(), 1.0f);
            glm::vec4 color = m_color[i];
            glm::vec4 nearFar = glm::vec4(m_transformation[i].GetNear(),m_transformation[i].GetFar(),0,0);
            glm::vec4 direction = glm::vec4(m_transformation[i].GetFrontDir(), 0.0f);

            if (m_customLightUBO->viewProjMatrix[i] != viewProjMatrix)
            {
                uboDirty = true;
            }

            m_customLightUBO->viewProjMatrix[i] = viewProjMatrix;
            m_customLightUBO->color_far[i] = color;
            m_customLightUBO->color_far[i].w = nearFar.y;
            m_customLightUBO->position_near[i] = position;
            m_customLightUBO->position_near[i].w = nearFar.x;
            m_customLightUBO->direction[i] = direction;
        }

        if (uboDirty)
        {
            try{
            std::size_t offset = 0;
            std::size_t elementSize = sizeof(glm::vec4);
            // fill header
            memcpy(m_pMappingMemory, &m_customLightUBO->header, elementSize);

            offset += elementSize;
            elementSize = sizeof(glm::mat4) * m_lightNum;
            // fill viewProjMatrix
            memcpy((char*)m_pMappingMemory + offset, m_customLightUBO->viewProjMatrix.data(), elementSize);

            offset += elementSize;
            elementSize = sizeof(glm::vec4) * m_lightNum;
            // fill position_near
            memcpy((char*)m_pMappingMemory + offset, m_customLightUBO->position_near.data(), elementSize);

            offset += elementSize;
            elementSize = sizeof(glm::vec4) * m_lightNum;
            // fill color_far
            memcpy((char*)m_pMappingMemory + offset, m_customLightUBO->color_far.data(), elementSize);

            offset += elementSize;
            elementSize = sizeof(glm::vec4) * m_lightNum;
            // fill direction
            memcpy((char*)m_pMappingMemory + offset, m_customLightUBO->direction.data(), elementSize);
            }
            catch(...)
            {
                std::cout << "error" << std::endl;
            }

        }
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

    if (m_lightNum > 5)
    {
        m_customLightUBO.reset(new CustomLargeLightUBO());
        m_customLightUBO->header.x = m_lightNum;
        m_customLightUBO->viewProjMatrix.resize(m_lightNum);
        m_customLightUBO->position_near.resize(m_lightNum);
        m_customLightUBO->color_far.resize(m_lightNum);
        m_customLightUBO->direction.resize(m_lightNum);
        m_pCustomUniformBuffer.reset(
            new RHI::VulkanBuffer(
                    m_pDevice, m_customLightUBO->GetSize(),
                    vk::BufferUsageFlagBits::eUniformBuffer,
                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                    vk::SharingMode::eExclusive
                )
            );
        m_pMappingMemory = m_pCustomUniformBuffer->MappingBuffer(0, m_customLightUBO->GetSize());
    }
}
