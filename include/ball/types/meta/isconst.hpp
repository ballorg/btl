#ifndef _INCLUDE_BALL_TYPES_META_ISCONST_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISCONST_HPP_

#	pragma once

// Determine whether T is a (raw) pointer type (excludes member pointers).
template < typename T > constexpr bool IS_CONST = false;
template < typename U > constexpr bool IS_CONST< U const > = true;
template < typename U > constexpr bool IS_CONST< U volatile > = false;
template < typename U > constexpr bool IS_CONST< U const volatile > = true;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISCONST_HPP_ )
