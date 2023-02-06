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
};


RHI_NAMESPACE_END