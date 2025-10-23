#ifndef _INCLUDE_BALL_TYPES_NEW_HPP_
#	define _INCLUDE_BALL_TYPES_NEW_HPP_

#	include "types/base/arch.h"
#	include "types/c/macros.h"

BALL_EXPORT inline void *operator new( size_t, size_t, [[ maybe_unused ]] void *p ) noexcept { return p; }
BALL_EXPORT inline void operator delete( void *, size_t, [[ maybe_unused ]] void *p ) noexcept {}
BALL_EXPORT inline void *operator new( size_t, [[ maybe_unused ]] void *p ) noexcept { return p; }
BALL_EXPORT inline void operator delete( void *, [[ maybe_unused ]] void *p ) noexcept {}

#endif // !defined( _INCLUDE_BALL_TYPES_NEW_HPP_ )
