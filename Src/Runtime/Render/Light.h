#pragma once
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/RenderPass/ShadowMapRenderPass.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Util/Mathutil.h"
#include <glm/fwd.hpp>
#include <vulkan/vulkan.hpp>

namespace Render {

class Lights
{
public:
    Lights(RHI::VulkanDevice* device,  int lightNum = 1, bool useShadowPass = false);
    Lights(RHI::VulkanDevice* device, const std::vector<Util::Math::VPMatrix>& transformation, bool useShadowPass = false);
    ~Lights();

    RHI::Model::UBOLayoutInfo GetUboInfo();
    RHI::Model::UBOLayoutInfo GetLargeUboInfo();

    void UpdateLightUBO();
    inline Util::Math::VPMatrix& GetLightTransformation(int lightIdx = 0) { return m_transformation[lightIdx]; }
    inline void SetLightColor(const glm::vec4& color, int lightIdx = 0) { m_color[lightIdx] = color; }
    inline RHI::ShadowMapRenderPass* GetPShadowPass() { return m_shadowmapPass.get(); }
    inline int GetLightNum() { return m_lightNum; }
    inline bool ShadowMapValid() { return m_shadowmapPass != nullptr; }
private:
    void initLightUBO();
private:

    struct CustomLargeLightUBO
    {
        glm::vec4 header;
        std::vector<glm::mat4> viewProjMatrix;
        std::vector<glm::vec4> position_near;
        std::vector<glm::vec4> color_far;
        std::vector<glm::vec4> direction;
        inline uint32_t GetSize()
        {
            uint32_t lightNum = viewProjMatrix.size();
            return sizeof(glm::vec4) + lightNum * (sizeof(glm::mat4) + sizeof(glm::vec4) * 3);
        }
    };
private:
    int m_lightNum;
    RHI::VulkanDevice* m_pDevice;
    std::vector<Util::Math::VPMatrix> m_transformation;
    std::vector<glm::vec4> m_color;

    std::unique_ptr<RHI::VulkanBuffer> m_lightUniformBuffers;
    RHI::LightInforUniformBufferObject m_lightUniformBufferObjects;

    std::unique_ptr<RHI::VulkanBuffer> m_pCustomUniformBuffer;
    void* m_pMappingMemory = nullptr;
    std::unique_ptr<CustomLargeLightUBO> m_customLightUBO;

    std::unique_ptr<RHI::ShadowMapRenderPass> m_shadowmapPass;
};



}