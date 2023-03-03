#pragma once
#include "Runtime/Render/RendererBase.h"
#include <memory>

#define EDITOR_NAMESPACE_BEGIN namespace editor {
#define EDITOR_NAMESPACE_END }
#define EDITOR_NAMESPACE_USING using namespace editor;


EDITOR_NAMESPACE_BEGIN

Render::RendererBase* Init(Render::RendererBase* runtime_renderer);
void UnInit();
EDITOR_NAMESPACE_END