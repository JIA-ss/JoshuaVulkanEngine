#include "RenderGraphHandle.h"
#include <limits>
#include <stdint.h>

using namespace Render;

const RenderGraphHandleBase RenderGraphHandleBase::InValid(std::numeric_limits<uint64_t>::max());