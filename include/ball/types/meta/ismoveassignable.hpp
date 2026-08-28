#ifndef _INCLUDE_BALL_TYPES_META_ISMOVEASSIGNABLE_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISMOVEASSIGNABLE_HPP_

#	pragma once

// Determine whether a T lvalue can be assigned from an rvalue of T.
template < typename T > struct MIsMoveAssignable { static constexpr bool VALUE = __is_assignable( T &, T && ); };

template < typename T > inline constexpr bool IS_MOVE_ASSIGNABLE = MIsMoveAssignable< T >::VALUE;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISMOVEASSIGNABLE_HPP_ )
