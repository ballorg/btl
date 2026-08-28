#ifndef _INCLUDE_BALL_TYPES_META_ISMEMMOVESAFE_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISMEMMOVESAFE_HPP_

#	pragma once

#	include "istriviallycopyable.hpp"

// Determine whether T may be relocated with memcpy/memmove. Equivalent to trivial
// copyability: such types carry no invariants tied to their address.
template < typename T > struct MIsMemmoveSafe { static constexpr bool VALUE = MIsTriviallyCopyable< T >::VALUE; };

template < typename T > inline constexpr bool IS_MEMMOVE_SAFE = MIsMemmoveSafe< T >::VALUE;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISMEMMOVESAFE_HPP_ )
