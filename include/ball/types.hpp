#ifndef _INCLUDE_BALL_TYPES_HPP_
#	define _INCLUDE_BALL_TYPES_HPP_

#	include "types/c/macros.h"

BALL_EXPORT namespace Ball::Types
{
#	include "types/base.h"
#	include "types/allocator.hpp"
#	include "types/vector.hpp"
#	include "types/elements.hpp"
#	include "types/math.hpp"
#	include "types/memoryview.hpp"
#	include "types/string.hpp"
#	include "types/stringview.hpp"
#	include "types/xvalue.hpp"
};

#endif // !defined( _INCLUDE_BALL_TYPES_HPP_ )
