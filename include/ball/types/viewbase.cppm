module;

#include "memory.h"
#include "c/assert/static.h"
#include "c/assert.h"

export module Ball.Types:ViewBase;

import Ball.New;
import :Core;
import :Elements;
import :ElementsPack;
import :Fixed;
import :Math;
import :Number;
import :Prefetch;

#include "module.h"

export namespace BTL
{
#include "viewbase.hpp"
}
