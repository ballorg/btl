#ifndef _INCLUDE_BALL_TYPES_META_SEQUENCE_HPP_
#	define _INCLUDE_BALL_TYPES_META_SEQUENCE_HPP_

#	pragma once

template < size_t ...Is >
struct MSequence
{
};

template < size_t N, size_t ...Is >
struct MMakeSequence : MMakeSequence< N - 1, N - 1, Is... >
{
};

template < size_t ...Is >
struct MMakeSequence< 0, Is... >
{
	using Type = MSequence< Is... >;
};

template < typename ...Ts > using Sequence_t = typename MMakeSequence< sizeof...( Ts ) >::Type;

#endif // !defined( _INCLUDE_BALL_TYPES_META_SEQUENCE_HPP_ )