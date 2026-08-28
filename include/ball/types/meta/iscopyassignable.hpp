#ifndef _INCLUDE_BALL_TYPES_META_ISCOPYASSIGNABLE_HPP_
#	define _INCLUDE_BALL_TYPES_META_ISCOPYASSIGNABLE_HPP_

#	pragma once

// Determine whether a T lvalue can be assigned from a const lvalue of T.
template < typename T > struct MIsCopyAssignable { static constexpr bool VALUE = __is_assignable( T &, const T & ); };

template < typename T > inline constexpr bool IS_COPY_ASSIGNABLE = MIsCopyAssignable< T >::VALUE;

#endif // !defined( _INCLUDE_BALL_TYPES_META_ISCOPYASSIGNABLE_HPP_ )
