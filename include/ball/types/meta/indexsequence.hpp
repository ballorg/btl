#ifndef _INCLUDE_BALL_TYPES_META_INDEXSEQUENCE_HPP_
#define _INCLUDE_BALL_TYPES_META_INDEXSEQUENCE_HPP_

#pragma once

/// Sequence of integral indices used for compile-time iteration and expansion.
template < typename I, I... Is >
struct MIndexSequence {};

template < bool Z, typename I, I N, I... Is >
struct MMakeIndexSequenceBy;

template < typename I, I N, I... Is >
struct MMakeIndexSequenceBy< false, I, N, Is... > : MMakeIndexSequenceBy< N == I( 1 ), I, N - 1, N - 1, Is... > {};

template < typename I, I N, I... Is >
struct MMakeIndexSequenceBy< true, I, N, Is... >
{
	using Type = MIndexSequence< I, Is... >;
};

/// Builds a zero-based index sequence `[0, N)`.
template < typename I, I N, I... Is >
struct MMakeIndexSequence : MMakeIndexSequenceBy< !N, I, N, Is... > {};

template < typename I, I N >
using MakeIndexSequence_t = typename MMakeIndexSequence< I, N >::Type;

/// Offsets each element in an index sequence by a constant value.
template < typename I, I B, typename S >
struct MIndexSequenceOffset;

template < typename I, I B, I... Is >
struct MIndexSequenceOffset< I, B, MIndexSequence< I, Is... > >
{
	using Type = MIndexSequence< I, ( B + Is )... >;
};

template < typename I, I B, typename S >
using IndexSequenceOffset_t = typename MIndexSequenceOffset< I, B, S >::Type;

#endif // !defined( _INCLUDE_BALL_TYPES_META_INDEXSEQUENCE_HPP_ )
