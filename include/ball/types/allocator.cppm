module;

#include "memory.h"
#include "c/assert.h"

export module Ball.Types:Allocator;

import Ball.New;
import :Core;

#include "module.h"

export namespace BTL
{
#include "allocator.hpp"
}
