#pragma once
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/RenderPass/ShadowMapRenderPass.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Util/Mathutil.h"
#include <vulkan/vulkan.hpp>

namespace Render {

class Lights
{
public:
    Lights(RHI::VulkanDevice* device,  int lightNum = 1, bool useShadowPass = false);
    Lights(RHI::VulkanDevice* device, const std::vector<Util::Math::VPMatrix>& transformation, bool useShadowPass = false);
    ~Lights();

    std::array<RHI::Model::UBOLayoutInfo, MAX_FRAMES_IN_FLIGHT> GetUboInfo();

    void UpdateLightUBO(int frameId);
    inline Util::Math::VPMatrix& GetLightTransformation(int lightIdx = 0) { return m_transformation[lightIdx]; }
    inline void SetLightColor(const glm::vec4& color, int lightIdx = 0) { m_color[lightIdx] = color; }
    inline RHI::ShadowMapRenderPass* GetPShadowPass() { return m_shadowmapPass.get(); }
    inline int GetLightNum() { return m_lightNum; }
    inline bool ShadowMapValid() { return m_shadowmapPass != nullptr; }
private:
    void initLightUBO();
private:
    int m_lightNum;
    RHI::VulkanDevice* m_pDevice;
    std::vector<Util::Math::VPMatrix> m_transformation;
    std::vector<glm::vec4> m_color;

    std::array<std::unique_ptr<RHI::VulkanBuffer>, MAX_FRAMES_IN_FLIGHT> m_lightUniformBuffers;
    std::array<RHI::LightInforUniformBufferObject, MAX_FRAMES_IN_FLIGHT> m_lightUniformBufferObjects;

    std::unique_ptr<RHI::ShadowMapRenderPass> m_shadowmapPass;
};



}