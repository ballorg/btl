module;

#include <ball/types/memory.h>
#include <ball/types/c/assert.h>

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

#include <ball/types/module.h>

export namespace BTL
{
#include <ball/types/vector.hpp>
}
