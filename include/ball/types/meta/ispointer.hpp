#ifndef _INCLUDE_BALL_TYPES_META_ISPOINTER_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISPOINTER_HPP_

// Determine whether T is a (raw) pointer type (excludes member pointers).
template < typename T > constexpr bool IS_POINTER = false;
template < typename U > constexpr bool IS_POINTER< U * > = true;
template < typename U > constexpr bool IS_POINTER< U * const > = true;
template < typename U > constexpr bool IS_POINTER< U * volatile > = true;
template < typename U > constexpr bool IS_POINTER< U * const volatile > = true;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISPOINTER_HPP_ )
