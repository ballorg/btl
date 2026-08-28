module;

#include "memory.h"
#include "c/assert.h"
#include "c/assert/static.h"
#include "c/nouniqueaddress.h"

export module Ball.Types:Hash;

import Ball.New;
import :Core;
import :Bits;
import :Math;
import :StringView;

#include "module.h"

export namespace BTL
{
#include "hash.hpp"
}
