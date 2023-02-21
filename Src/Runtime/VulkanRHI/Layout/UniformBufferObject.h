#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_BEGIN

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 camPos;
    glm::vec3 lightPos;
};


RHI_NAMESPACE_END