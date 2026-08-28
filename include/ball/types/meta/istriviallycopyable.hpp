#ifndef _INCLUDE_BALL_TYPES_META_ISTRIVIALLYCOPYABLE_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISTRIVIALLYCOPYABLE_HPP_

#	pragma once

// Determine whether T is trivially copyable (safe to copy with a raw byte copy).
template < typename T > struct MIsTriviallyCopyable { static constexpr bool VALUE = __is_trivially_copyable( T ); };

template < typename T > inline constexpr bool IS_TRIVIALLY_COPYABLE = MIsTriviallyCopyable< T >::VALUE;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISTRIVIALLYCOPYABLE_HPP_ )
