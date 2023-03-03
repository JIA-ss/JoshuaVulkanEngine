#include "EditorWindow.h"
#include "Editor/Editor.h"
#include "Runtime/Platform/PlatformWindow.h"
#include "Runtime/VulkanRHI/VulkanInstance.h"
#include "Runtime/VulkanRHI/VulkanPhysicalDevice.h"

EDITOR_NAMESPACE_USING

std::unique_ptr<platform::PlatformWindow> s_pEditorWindow{};

platform::PlatformWindow* editor::InitEditorWindow()
{
    assert(!s_pEditorWindow);
    s_pEditorWindow = platform::CreatePlatformWindow( {1920, 1080, "Editor Window", 2, {-1920,-300}, false});
    s_pEditorWindow->Init();
    return s_pEditorWindow.get();
}

void editor::DestroyEditorWindow()
{
    s_pEditorWindow->Destroy();
    s_pEditorWindow.reset();
}

platform::PlatformWindow* editor::GetPEditorWindow()
{
    assert(s_pEditorWindow);
    return s_pEditorWindow.get();
}
