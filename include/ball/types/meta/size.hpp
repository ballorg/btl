#ifndef _INCLUDE_BALL_TYPES_META_SIZE_HPP_
#define _INCLUDE_BALL_TYPES_META_SIZE_HPP_

#pragma once

/// Compile-time wrapper for an integral constant used as a meta value carrier.
template < typename I, I N >
struct MSize
{
	using Type_t = I;

	static constexpr I VALUE = N;
};

template < typename I, I N >
using Size_t = MSize< I, N >;

#endif // !defined( _INCLUDE_BALL_TYPES_META_SIZE_HPP_ )
