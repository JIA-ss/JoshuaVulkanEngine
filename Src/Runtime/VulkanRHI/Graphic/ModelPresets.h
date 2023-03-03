#pragma once

#include "Model.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Util/Mathutil.h"


RHI_NAMESPACE_BEGIN


namespace ModelPresets
{

    std::unique_ptr<Model> CreatePlaneModel(VulkanDevice* device, VulkanDescriptorSetLayout* layout);
    std::unique_ptr<Model> CreateCubeModel(VulkanDevice* device, VulkanDescriptorSetLayout* layout);
    std::unique_ptr<Model> CreateFrustumModel(VulkanDevice* device, VulkanDescriptorSetLayout* layout, const Util::Math::VPMatrix& matrix);


}

RHI_NAMESPACE_END