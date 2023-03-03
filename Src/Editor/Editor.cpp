#include "Editor.h"
#include "Editor/Render/EditorRenderer.h"
#include "Editor/Window/EditorWindow.h"
#include "Runtime/VulkanRHI/VulkanInstance.h"
#include "Runtime/VulkanRHI/VulkanPhysicalDevice.h"

std::unique_ptr<editor::EditorRenderer> s_pEditorRenderer{};

Render::RendererBase* editor::Init(Render::RendererBase* runtime_renderer)
{
    platform::PlatformWindow* window = InitEditorWindow();

    auto extensions = window->GetRequiredExtensions();
    std::vector<const char*> enabledInstanceExtensions;

    vk::PhysicalDeviceFeatures feature;
    feature.setSamplerAnisotropy(VK_TRUE)
            .setFillModeNonSolid(VK_TRUE);

    s_pEditorRenderer.reset(
        new editor::EditorRenderer(
            runtime_renderer,
            RHI::VulkanInstance::Config { true, "RHI", "RHI", VK_API_VERSION_1_2, extensions },
            RHI::VulkanPhysicalDevice::Config { window, feature, {}, vk::SampleCountFlagBits::e1 }
        )
    );
    return s_pEditorRenderer.get();
}

void editor::UnInit()
{
    s_pEditorRenderer.reset();
    DestroyEditorWindow();
}