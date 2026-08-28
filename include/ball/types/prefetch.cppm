module;

#include "memory.h"
#include "c/prefetch.h"

export module Ball.Types:Prefetch;

import Ball.New;
import :Core;

#include "module.h"

export namespace BTL
{
#include "prefetch.hpp"
}
