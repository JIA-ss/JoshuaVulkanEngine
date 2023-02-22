#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 camPos;
    alignas(16) glm::vec3 lightPos;
};


RHI_NAMESPACE_END