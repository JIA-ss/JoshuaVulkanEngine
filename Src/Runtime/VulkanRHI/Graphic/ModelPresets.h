#pragma once

#include "Model.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Util/Mathutil.h"
#include "Util/Modelutil.h"


RHI_NAMESPACE_BEGIN


namespace ModelPresets
{

    Util::Model::MeshData GetCubeMeshData();
    Util::Model::MaterialData GetSkyboxMaterialData();

    std::unique_ptr<Model> CreatePlaneModel(VulkanDevice* device, VulkanDescriptorSetLayout* layout);
    std::unique_ptr<Model> CreateCubeModel(VulkanDevice* device, VulkanDescriptorSetLayout* layout);
    std::unique_ptr<Model> CreateFrustumModel(VulkanDevice* device, VulkanDescriptorSetLayout* layout, const Util::Math::VPMatrix& matrix);


    // pbr gun
    std::unique_ptr<Model> CreateCerberusPBRModel(VulkanDevice* device, VulkanDescriptorSetLayout* layout);

    // skybox
    std::unique_ptr<Model> CreateSkyboxModel(VulkanDevice* device, VulkanDescriptorSetLayout* layout);

}

RHI_NAMESPACE_END