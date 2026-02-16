#ifndef _INCLUDE_BALL_TYPES_META_TRAITS_HPP_
#	define _INCLUDE_BALL_TYPES_META_TRAITS_HPP_

#	pragma once

#	include "removecv.hpp"
#	include "removereference.hpp"

#	if defined( _MSC_VER ) || defined( __clang__ )
#		define BALL_META_IS_TRIVIALLY_DESTRUCTIBLE( T ) __is_trivially_destructible( T )
#	elif defined( __GNUC__ )
#		define BALL_META_IS_TRIVIALLY_DESTRUCTIBLE( T ) __has_trivial_destructor( T )
#	else
#		define BALL_META_IS_TRIVIALLY_DESTRUCTIBLE( T ) __is_trivial( T )
#	endif

template < typename T >
struct MTraits
{
	using Type = RemoveCV_t< typename MRemoveReference< T >::Type >;

	static constexpr bool IS_TRIVIAL = __is_trivial( Type );
	static constexpr bool IS_COPYABLE = __is_trivially_copyable( Type );
	static constexpr bool IS_MEMMOVE_SAFE = IS_COPYABLE;

	static constexpr bool IS_DEFAULT_CONSTRUCTIBLE = __is_constructible( Type );
	static constexpr bool IS_TRIVIALLY_CONSTRUCTIBLE = __is_trivially_constructible( Type );
	static constexpr bool IS_TRIVIALLY_DESTRUCTIBLE = BALL_META_IS_TRIVIALLY_DESTRUCTIBLE( Type );

	static constexpr bool IS_COPY_CONSTRUCTIBLE = __is_constructible( Type, const Type & );
	static constexpr bool IS_COPY_ASSIGNABLE = __is_assignable( Type &, const Type & );
	static constexpr bool IS_MOVE_CONSTRUCTIBLE = __is_constructible( Type, Type && );
	static constexpr bool IS_MOVE_ASSIGNABLE = __is_assignable( Type &, Type && );
};

template < typename T > static constexpr bool IS_TRIVIAL = MTraits< T >::IS_TRIVIAL;
template < typename T > static constexpr bool IS_TRIVIALLY_COPYABLE = MTraits< T >::IS_COPYABLE;
template < typename T > static constexpr bool IS_MEMMOVE_SAFE = MTraits< T >::IS_MEMMOVE_SAFE;

#	undef BALL_META_IS_TRIVIALLY_DESTRUCTIBLE

#endif // !defined( _INCLUDE_BALL_TYPES_META_TRAITS_HPP_ )
