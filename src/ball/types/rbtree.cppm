module;

#include <ball/types/memory.h>
#include <ball/types/c/assert.h>
#include <ball/types/c/nouniqueaddress.h>
#include <ball/types/meta/fixed.h>
#include <ball/types/reflect.h>
#include <ball/types/rbtree.h>

export module Ball.Types:RBTree;

import Ball.New;
import :Core;
import :Elements;
import :Fixed;
import :Pair;
import :Reflect;
import :SlotIterator;
import :Vector;

#include <ball/types/module.h>

export namespace BTL
{
#include <ball/types/rbtree.hpp>
}
