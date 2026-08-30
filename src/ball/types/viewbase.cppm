module;

#include <ball/types/memory.h>
#include <ball/types/c/assert/static.h>
#include <ball/types/c/assert.h>

export module Ball.Types:ViewBase;

import Ball.New;
import :Core;
import :Elements;
import :ElementsPack;
import :Fixed;
import :Math;
import :Number;
import :Prefetch;

#include <ball/types/module.h>

export namespace BTL
{
#include <ball/types/viewbase.hpp>
}
