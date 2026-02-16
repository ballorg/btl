#ifndef _INCLUDE_BALL_TYPES_META_FIXED_UNSIGNED_H_
#	define _INCLUDE_BALL_TYPES_META_FIXED_UNSIGNED_H_

#	pragma once

#	include "bits.h"

template < typename T, typename U, bits_t NBITS = sizeof( T ) * 8u >
struct MFixedUnsignedImpl
{
	static_assert( 0 < NBITS, "NBITS must be > 0" );
	static_assert( NBITS <= sizeof( T ) * 8u, "NBITS exceeds storage width" );

	static constexpr T SIZE = static_cast< T >( sizeof( T ) );
	static constexpr T BYTES = SIZE;
	static constexpr T BITS = static_cast< T >( NBITS );
	static constexpr bits_t LAST_BIT = NBITS - 1u;

	static constexpr bool IS_UNSIGNED = true;
	static constexpr bool IS_SIGNED = false;

	static constexpr U MIN_UNSIGNED = static_cast< U >( 0ll );
	static constexpr U MAX_UNSIGNED = static_cast< U >( ~0ll );
	static constexpr U SIGN_MASK = static_cast< U >( static_cast< U >( 1 ) << LAST_BIT );
	static constexpr U MAX_SIGNED_U = static_cast< U >( SIGN_MASK - static_cast< U >( 1 ) );
	static constexpr T MIN_SIGNED = static_cast< T >( SIGN_MASK );
	static constexpr T ALL_BITS = static_cast< T >( MAX_UNSIGNED );
	static constexpr T INVALID = ALL_BITS;
	static constexpr T MAX_SIGNED = static_cast< T >( MAX_SIGNED_U );
	static constexpr U MIN = MIN_UNSIGNED;
	static constexpr U MAX = MAX_UNSIGNED;
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_FIXED_UNSIGNED_H_ )
