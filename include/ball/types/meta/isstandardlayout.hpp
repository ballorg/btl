#ifndef _INCLUDE_BALL_TYPES_META_ISSTANDARDLAYOUT_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISSTANDARDLAYOUT_HPP_

#	pragma once

// Determine whether a type is a standard-layout type. offsetof is only well-defined
// (and free of the -Winvalid-offsetof diagnostic) on standard-layout types; the
// reflection layer falls back to a run-time measurement for everything else.
template < typename T >
constexpr bool IS_STANDARD_LAYOUT = __is_standard_layout( T );

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISSTANDARDLAYOUT_HPP_ )
