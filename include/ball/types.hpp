#ifndef _INCLUDE_BALL_TYPES_HPP_
#	define _INCLUDE_BALL_TYPES_HPP_

#	pragma once

#	include "new.hpp"

#	include "types/memory.h"

#	include "types/c/assert.h"
#	include "types/c/assert/static.h"
#	include "types/c/bits.h"
#	include "types/c/math.h"
#	include "types/c/nodiscrad.h"
#	include "types/c/nouniqueaddress.h"
#	include "types/c/prefetch.h"
#	include "types/fixed.h"
#	include "types/hashmap.h"
#	include "types/meta/fixed.h"
#	include "types/rbtree.h"
#	include "types/reflect.h"

BALL_EXPORT namespace BTL
{
#	include "types/meta.hpp"
#	include "types/bits.hpp"
#	include "types/elements.hpp"
#	include "types/fixed.hpp"
#	include "types/math.hpp"
#	include "types/number.hpp"
#	include "types/pair.hpp"
#	include "types/prefetch.hpp"
#	include "types/memoryaligned.h"
#	include "types/allocator.hpp"
#	include "types/array.hpp"
#	include "types/elementspack.hpp"
#	include "types/vectoriterator.hpp"
#	include "types/slotiterator.hpp"
#	include "types/reflect.hpp"
#	include "types/viewbase.hpp"
#	include "types/view.hpp"
#	include "types/stringview.hpp"
#	include "types/hash.hpp"
#	include "types/vector.hpp"
#	include "types/string.hpp"
#	include "types/rbtree.hpp"
#	include "types/hashmap.hpp"
#	include "types/delegate.hpp"
};

#endif // !defined( _INCLUDE_BALL_TYPES_HPP_ )
