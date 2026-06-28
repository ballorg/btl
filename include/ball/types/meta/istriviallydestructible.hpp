#ifndef _INCLUDE_BALL_TYPES_META_ISTRIVIALLYDESTRUCTIBLE_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISTRIVIALLYDESTRUCTIBLE_HPP_

#	pragma once

#	if defined( _MSC_VER ) || defined( __clang__ )
#		define BALL_META_IS_TRIVIALLY_DESTRUCTIBLE( T ) __is_trivially_destructible( T )
#	elif defined( __GNUC__ )
#		define BALL_META_IS_TRIVIALLY_DESTRUCTIBLE( T ) __has_trivial_destructor( T )
#	else
#		define BALL_META_IS_TRIVIALLY_DESTRUCTIBLE( T ) __is_trivial( T )
#	endif

// Determine whether T's destructor is trivial (no-op cleanup).
template < typename T > struct MIsTriviallyDestructible { static constexpr bool VALUE = BALL_META_IS_TRIVIALLY_DESTRUCTIBLE( T ); };

template < typename T > static constexpr bool IS_TRIVIALLY_DESTRUCTIBLE = MIsTriviallyDestructible< T >::VALUE;

#	undef BALL_META_IS_TRIVIALLY_DESTRUCTIBLE

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISTRIVIALLYDESTRUCTIBLE_HPP_ )
