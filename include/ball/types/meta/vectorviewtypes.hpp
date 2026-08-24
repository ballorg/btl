#ifndef _INCLUDE_BALL_TYPES_META_VECTORVIEWTYPES_HPP_
#	define _INCLUDE_BALL_TYPES_META_VECTORVIEWTYPES_HPP_

#	pragma once

template < typename I, typename T, I N >
class CView;

template < class B, typename I, I N, typename T, bool = requires { typename B::View_t; typename B::ConstView_t; } >
struct MVectorViewTypes
{
	using View_t = CView< I, T, N >;
	using ConstView_t = CView< I, const T, N >;
};

template < class B, typename I, I N, typename T >
struct MVectorViewTypes< B, I, N, T, true >
{
	using View_t = typename B::View_t;
	using ConstView_t = typename B::ConstView_t;
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_VECTORVIEWTYPES_HPP_ )
