module;

#include <ball/types/memory.h>
#include <ball/types/c/prefetch.h>

export module Ball.Types:Prefetch;

import Ball.New;
import :Core;

#include <ball/types/module.h>

export namespace BTL
{
#include <ball/types/prefetch.hpp>
}
