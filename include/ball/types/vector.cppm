module;

#include "memory.h"
#include "c/assert.h"

export module Ball.Types:Vector;

import Ball.New;
import :Core;
import :Allocator;
import :Bits;
import :Elements;
import :Fixed;
import :Math;
import :VectorIterator;
import :View;
import :ViewBase;

#include "module.h"

export namespace BTL
{
#include "vector.hpp"
}
