module;

#include "memory.h"
#include "c/assert.h"
#include "c/nouniqueaddress.h"
#include "meta/fixed.h"
#include "reflect.h"
#include "rbtree.h"

export module Ball.Types:RBTree;

import Ball.New;
import :Core;
import :Elements;
import :Fixed;
import :Pair;
import :Reflect;
import :SlotIterator;
import :Vector;

#include "module.h"

export namespace BTL
{
#include "rbtree.hpp"
}
