#pragma once
#include "Editor/Editor.h"
#include "Runtime/Platform/PlatformWindow.h"

EDITOR_NAMESPACE_BEGIN

platform::PlatformWindow* InitEditorWindow();
void DestroyEditorWindow();
platform::PlatformWindow* GetPEditorWindow();


EDITOR_NAMESPACE_END