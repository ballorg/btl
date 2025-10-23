#ifndef _INCLUDE_BALL_TYPES_BITS_HPP_
#	define _INCLUDE_BALL_TYPES_BITS_HPP_

#	include "base/arch.h"
#	include "meta/number.hpp"
#	include "c/bits.h"

/// @brief Returns the smallest power of two greater than or equal to @p v,
///        saturating to the highest power-of-two representable in T.
/// @tparam T Unsigned integral type (e.g., uint32_t, uint64_t).
///
/// @details
/// - For v <= 1, returns 1.
/// - For v already a power of two, returns v.
/// - If the exact ceiling power-of-two does not fit in T, returns the
///   highest power-of-two of T (i.e., 1 << (bit_width(T) - 1 ) ).
/// - Uses compiler intrinsics where available; otherwise falls back to a
///   portable bit-spreading implementation.
/// - Precondition: T must be an uint_t integral type.
///
/// @note This function implements semantics similar to std::bit_ceil 
///       but saturates instead of overflowing/returning 0 on overflow.
///
/// @return ceil_pow2(v) clamped to the type’s top power-of-two.
template < typename T >
constexpr T NextPowerOfTwo_Unified( T v ) noexcept
{
	using N = MNumber< T >;
	constexpr T NUM_BITS = N::BITS;

	if ( v <= 1 )
		return T( 1 );

	--v;
	v |= ( v >> 1 );
	v |= ( v >> 2 );
	v |= ( v >> 4 );

	if constexpr ( NUM_BITS >  8 ) v |= ( v >> 8  );
	if constexpr ( NUM_BITS > 16 ) v |= ( v >> 16 );
	if constexpr ( NUM_BITS > 32 ) v |= ( v >> 32 );

	++v;

	return v ? v : T( 1 ) << ( NUM_BITS - 1 );
}

template < typename T >
consteval T NextPowerOfTwo_Const( T v )
{
	return NextPowerOfTwo_Unified( v );
}

template < typename T, T N = 0 >
constexpr T NextPowerOfTwo( T v ) noexcept
{
	using Number_t = MNumber< T >;
	constexpr T NUM_BITS = Number_t::BITS;
	constexpr T INVALID = Number_t::BITS;

	if ( v <= BALL_MAX( 1, N ) )
	{
		return NextPowerOfTwo_Unified( v );
	}
#	if defined( _MSC_VER )
	// MSVC (_BitScanReverse/_BitScanReverse64), supports to 64 bits
	else if constexpr ( NUM_BITS <= 32 )
	{
		ulong_t idx;
		uint_t x = static_cast< uint_t >( v - 1 );

		_BitScanReverse( &idx, x );

		uint_t s = static_cast< uint_t >( idx ) + 1;

		return ( s >= NUM_BITS ) ? INVALID : ( T( 1 ) << s );
	}
	else
	{
		ulong_t idx;
		ullong_t x = static_cast< ullong_t >( v - 1 );

		_BitScanReverse64( &idx, x );
		uint_t s = static_cast< uint_t >( idx ) + 1;

		return ( s >= NUM_BITS ) ? INVALID : ( T( 1 ) << s );
	}

#	elif defined( __GNUC__ ) || defined( __clang__ )
	// GCC/Clang: __builtin_clz*/__builtin_clzll, supports to 64 bits
	else if constexpr ( NUM_BITS <= 32 )
	{
		uint_t x = static_cast< uint_t >( v - 1 );
		// clz(0) — UB, но x != 0 т.к. v > 1
		uint_t s = 32u - static_cast< uint_t >( __builtin_clz(x) );

		return ( s >= NUM_BITS ) ? INVALID : ( T( 1 ) << s );
	}
	else if constexpr ( NUM_BITS <= 64 )
	{
		ullong_t x = static_cast< ullong_t >( v - 1 );
		uint_t s = 64u - static_cast< uint_t >( __builtin_clzll(x) );

		return ( s >= NUM_BITS ) ? INVALID : ( T( 1 ) << s );
	}
	else
	{
		return NextPowerOfTwo_Unified( v );
	}

#	else
#		error Unsupported platform!
#	endif
}

/// @brief Computes the new count by repeatedly doubling old_count until it is
///        at least requested_count, with the result clamped to [min_count, max_count].
/// 
/// @tparam T Any integral type (int_t or uint_t).
/// 
/// @details
/// - If requested_count <= old_count, returns old_count (no clamping is applied in this case),
///   matching the original loop semantics.
/// - Otherwise, returns the smallest value of the form old_count * 2^k that is
///   >= max(requested_count, min_count), but not exceeding max_count.
/// - All intermediate calculations are performed in the uint_t counterpart of T to
///   avoid undefined behavior on shifts and to reduce overflow risk.
/// - Multiplication is checked: if old_count * growth would exceed max_count, the result
///   is saturated to max_count.
/// 
/// @pre old_count > 0, 0 < min_count <= max_count (recommended invariants; enforce with asserts if needed).
/// 
/// @return The adjusted count as T, preserving the original function’s semantics.
template < typename T, typename U = Unsigned_t< T > >
constexpr T NextDoublingCapacityT( T nOldCount, T nRequestedCount, T nMinCount, T nMaxCount ) noexcept
{
	// Fast path: if request already satisfied, return old_count verbatim
	// (original code does NOT clamp to [min,max] in this branch).
	if ( nRequestedCount <= nOldCount )
		return nOldCount;

	// Map inputs into uint_t domain for safe bit operations / divisions.
	// Precondition is that counts are non-negative; if you can't guarantee it,
	// add runtime checks or casts with defensive handling.
	const U nOld = static_cast< U >( nOldCount );
	const U nRequired = static_cast< U >( nRequestedCount );
	const U nMin = static_cast< U >( nMinCount );
	const U nMax = static_cast< U >( nMaxCount );
	const U nMinRequired = BALL_MIN( nRequired, nMin );

	// If old_count is zero (violates the usual precondition), we cannot grow by doubling.
	// Define behavior: jump directly to max(mn, min(rq, mx)).
	if ( nOld == U{ 0 } )
		return static_cast<T>( BALL_MIN( nMinRequired, nMax ) );

	// Target we must reach by doubling (respect the lower bound).
	const U nTarget = BALL_MAX( nRequired, nMin );

	// Compute ratio = ceil(target / oc) without overflow: 1 + (target - 1) / oc
	// (valid because target >= 1 and oc >= 1 here).
	const U nRatio = U( 1 ) + ( nTarget - U( 1 ) ) / nOld;

	// Minimal power-of-two multiplier >= ratio.
	const U nGrow = NextPowerOfTwo< U >( nRatio ); // >= 1

	// Checked multiply against mx to avoid overflow and to clamp like the original.
	// If oc * growth would exceed mx, we saturate to mx.
	// Use division to avoid intermediate overflow: if growth > mx / oc => clamp.
	U nResult;

	if ( nGrow > ( nMax / nOld ) )
	{
		nResult = nMax;
	}
	else
	{
		const U nCandidate = nOld * nGrow;

		nResult = BALL_MIN( nMax, nCandidate );
	}

	return static_cast< T >( nResult );
}

#endif // !defined( _INCLUDE_BALL_TYPES_BITS_HPP_ )
