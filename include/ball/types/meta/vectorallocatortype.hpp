#ifndef _INCLUDE_BALL_TYPES_META_VECTORALLOCATORTYPE_HPP_
#	define _INCLUDE_BALL_TYPES_META_VECTORALLOCATORTYPE_HPP_

#	pragma once

template < class A, bool = requires { typename A::Base_t; } >
struct MVectorAllocatorType
{
	using Type = A;
};

template < class A >
struct MVectorAllocatorType< A, true >
{
	using Type = typename A::Base_t;
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_VECTORALLOCATORTYPE_HPP_ )
