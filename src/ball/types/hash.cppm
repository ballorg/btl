module;

#include <ball/types/memory.h>
#include <ball/types/c/assert.h>
#include <ball/types/c/assert/static.h>
#include <ball/types/c/nouniqueaddress.h>

export module Ball.Types:Hash;

import Ball.New;
import :Core;
import :Bits;
import :Math;
import :StringView;

#include <ball/types/module.h>

export namespace BTL
{
#include <ball/types/hash.hpp>
}
