#pragma once

#include "Runtime/Platform/PlatformWindow.h"
namespace editor {

void StartUp();
void Run();
void ShutDown();
platform::PlatformWindow* GetWindow();
}