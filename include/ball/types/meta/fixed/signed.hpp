#ifndef _INCLUDE_BALL_TYPES_META_FIXED_SIGNED_HPP_
#	define _INCLUDE_BALL_TYPES_META_FIXED_SIGNED_HPP_

#	pragma once

#	include "bits.h"

template < typename T, typename U, bits_t NBITS = sizeof( T ) * 8u >
struct MFixedSignedImpl
{
	static_assert( 0 < NBITS, "NBITS must be > 0" );
	static_assert( NBITS <= sizeof( T ) * 8u, "NBITS exceeds storage width" );

	static constexpr T SIZE = static_cast< T >( sizeof( T ) );
	static constexpr T BYTES = SIZE;
	static constexpr T BITS = static_cast< T >( NBITS );
	static constexpr bits_t LAST_BIT = NBITS - 1u;

	static constexpr bool IS_UNSIGNED = false;
	static constexpr bool IS_SIGNED = true;

	static constexpr U MIN_UNSIGNED = static_cast< U >( 0ull );
	static constexpr U MAX_UNSIGNED = static_cast< U >( ~0ull );
	static constexpr U SIGN_MASK = static_cast< U >( 1 ) << LAST_BIT;
	static constexpr U MAX_SIGNED_U = static_cast< U >( SIGN_MASK - static_cast< U >( 1 ) );
	static constexpr T MAX_SIGNED = static_cast< T >( MAX_SIGNED_U );
	static constexpr T MIN_SIGNED = static_cast< T >( -MAX_SIGNED - static_cast< T >( 1 ) );
	static constexpr T ALL_BITS = static_cast< T >( MAX_UNSIGNED );
	static constexpr T INVALID = ALL_BITS;
	static constexpr U MIN = static_cast< U >( MIN_SIGNED );
	static constexpr U MAX = static_cast< U >( MAX_SIGNED );
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_FIXED_SIGNED_HPP_ )
