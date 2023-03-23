#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN
// SET0 BINDING0
struct CameraUniformBufferObject
{
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat4 model;
    alignas(16) glm::vec4 camPos;
};

// SET0 BINDING1
struct  LightInforUniformBufferObject
{
    alignas(16) glm::mat4 viewProjMatrix[5];
    alignas(16) glm::vec4 direction[5];
    alignas(16) glm::vec4 position[5];
    alignas(16) glm::vec4 color[5];
    alignas(16) glm::vec4 nearFar[5];
    alignas(16) float lightNum;
};

// SET0 BINDING2
struct ModelUniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::vec4 color;
};

// SET0 BINDING3 CUSTOM UBO
RHI_NAMESPACE_END