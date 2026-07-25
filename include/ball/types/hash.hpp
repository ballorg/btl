#ifndef _INCLUDE_BALL_TYPES_HASH_HPP_
#	define _INCLUDE_BALL_TYPES_HASH_HPP_

#include "c/assert/static.h"
#	pragma once

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "bits.hpp"
#	include "c/assert.h"
#	include "c/nouniqueaddress.h"
#	include "meta/constevaluated.hpp"
#	include "meta/enableif.hpp"
#	include "meta/fibonacci.hpp"
#	include "meta/fixed.hpp"
#	include "meta/isintegral.hpp"
#	include "math.hpp"
#	include "stringview.hpp"

///-----------------------------------------------------------------------------
/// @brief Stateless Fibonacci (multiplicative golden-ratio) hashing policy.
///
/// @details The policy is the reusable, container-independent building block the
/// hash table plugs in through a template parameter, the same way `CMultiRBTree`
/// takes the `CRBTreeLess` comparator. It splits into two composable steps:
///
///   1. @ref Make "hashing" reduces a key of any supported type to a single
///      working word of type @p U. Integers map to themselves; composite
///      keys (strings, byte-like sequences) are folded unit by unit with an
///      FNV-1a-style mix that reuses the golden-ratio multiplier.
///   2. @ref Index "indexing" maps that word onto a bucket of a power-of-two
///      table via `(word * A_N) >> (N - S)`, where `A_N` is the odd golden-ratio
///      constant for the word width `N` and `S` addresses the current capacity.
///
/// The whole chain is `constexpr`, so a key that is a constant expression (an
/// integer literal, a `constexpr` value, or a `constexpr CStringView`) is hashed
/// by the compiler and the result is usable in constant-expression contexts,
/// while the exact same code runs at run time for dynamic keys -- no separate code paths
/// and, by construction, a bit-identical result either way. `IsConstantEvaluated()`
/// (from `meta/constevaluated.hpp`) is available to future callers that want to
/// fork in a result-preserving way; the fold deliberately needs no such fork.
///
/// The policy is empty (holds no state) so a container can host it at zero cost
/// through empty-base optimization, or as a `BALL_NO_UNIQUE_ADDRESS` member.
///
/// @tparam U Unsigned working word type; its width `BITS = MFibonacci< U >::BITS`
/// bounds the addressable table to `2^BITS` buckets. `MFibonacci< U >` requires it
/// unsigned, so the word is `U` itself -- no separate `U` alias is needed.
/// @tparam INDEX Index used to initialize the byte-fold accumulator;
/// defaults to `90` and is narrowed to the working word width.
/// Integer keys bypass the fold.
///-----------------------------------------------------------------------------
template < typename U = size_t, uint7_t INDEX = 93 >
class CFibonacciHash : public MFibonacci< U >
{
public:
	using Base_t = MFibonacci< U >;

	BALL_STATIC_ASSERT( INDEX <= 93, "Fibonacci index must fit in the 64-bit result" );

	// The width-derived constants live in the trait; a dependent base is not
	// searched by unqualified lookup, so surface the names the policy body and
	// callers use (`BITS`, `MULTIPLIER`, `OFFSET_BASIS`). The working word is the
	// unsigned template parameter `U` itself.
	using Base_t::BITS;
	using Base_t::MULTIPLIER;
	using Base_t::OFFSET_BASIS;

	/// @brief The compile-time Fibonacci seed that primes @ref Fold.
	static constexpr U FOLD_SEED = static_cast< U >( Fibonacci_NumberConst( INDEX ) );

	///-----------------------------------------------------------------------------
	/// @brief Folds one code unit into a running accumulator (FNV-1a shape).
	///
	/// @details The unit is read by numeric value through its unsigned partner, so
	/// the result never depends on platform endianness or on whether the character
	/// type is signed.
	///
	/// @complexity O(1): one xor and one modular multiply.
	///-----------------------------------------------------------------------------
	template < typename T >
	static constexpr U Append( U nAccumulator, const T &value ) noexcept
	{
		using Unsigned_t = typename MFixed< T >::Unsigned_t;

		return static_cast< U >( ( nAccumulator ^ static_cast< U >( static_cast< Unsigned_t >( value ) ) ) * MULTIPLIER );
	}

	///-----------------------------------------------------------------------------
	/// @brief Folds @p nCount code units starting at @p pUnits into one hash word.
	///
	/// @details Constant-expression friendly: when @p pUnits points at a string
	/// literal (or any object with static storage) the whole loop runs at compile
	/// time; the identical loop serves the run-time path for dynamic keys.
	///
	/// @complexity O(n) in the number of units.
	///-----------------------------------------------------------------------------
	template < typename I, typename T >
	static constexpr U Fold( I nCount, const T *pUnits ) noexcept
	{
		U nAccumulator = FOLD_SEED;

		const T *pUnitsEnd = pUnits + nCount;

		while ( pUnits < pUnitsEnd )
		{
			nAccumulator = Append( nAccumulator, *pUnits );
			pUnits++;
		}

		return nAccumulator;
	}

	/// @brief View-compatible storage overload: hashes exactly the stored units.
	template < typename S >
	static constexpr U Fold( const S &storage ) noexcept
	{
		U nAccumulator = FOLD_SEED;

		for( const auto &elem : storage )
		{
			nAccumulator = Append( nAccumulator, elem );
		}

		return nAccumulator;
	}

	///-----------------------------------------------------------------------------
	/// @brief Single hashing entry point: reduces a key to a working word @p U.
	///
	/// @details Integer keys are their own hash word (the Fibonacci multiply in
	/// @ref Index does the mixing); string-like keys are folded. Overloads never
	/// require the caller to know whether evaluation is compile-time or run-time.
	///
	/// @complexity O(1) for integers, O(length) for strings.
	///-----------------------------------------------------------------------------
	template < typename K, EnableIf_t< IS_INTEGRAL< K >, int > = 0 >
	static constexpr U Make( K key ) noexcept
	{
		using Unsigned_t = typename MFixed< K >::Unsigned_t;

		return static_cast< U >( static_cast< Unsigned_t >( key ) );
	}

	/// @brief `CStringView` overload: hashes exactly the viewed characters.
	template < typename I, typename T, I N >
	static constexpr U Make( const CStringView< I, T, N > &view ) noexcept
	{
		return Fold( view );
	}

	///-----------------------------------------------------------------------------
	/// @brief Maps a hash word onto a bucket of a table addressed by @p nIndexBits.
	///
	/// @details Implements `(word * A_N) >> (N - S)` with `S == nIndexBits`. This
	/// is the only step that depends on the current table size; the multiplier is
	/// size-independent. `nIndexBits == 0` (a single bucket) collapses to index 0.
	///
	/// @tparam TI Index type the bucket is returned in (the container's index type).
	///
	/// @complexity O(1): one modular multiply and one shift.
	///-----------------------------------------------------------------------------
	template < typename TI = U >
	static constexpr TI Index( U nHash, bits_t nIndexBits ) noexcept
	{
		BALL_ASSERT_MESSAGE( nIndexBits <= BITS, "Table index width cannot exceed the hash word width" );

		return nIndexBits ? static_cast< TI >( ( nHash * MULTIPLIER ) >> ( BITS - nIndexBits ) ) : 0;
	}

	///-----------------------------------------------------------------------------
	/// @brief Convenience form of @ref Index taking a power-of-two capacity.
	///
	/// @details This sits at the head of every hash-table probe, so the log2 of the
	/// power-of-two capacity is one hardware popcount of `nCapacity - 1` at run time
	/// (a shift-loop log2 would cost more than the rest of the lookup); constant
	/// evaluation, where intrinsics are unavailable and speed is free, takes the
	/// result-identical shift walk.
	///
	/// @complexity O(1): one popcount, one multiply and one shift.
	///-----------------------------------------------------------------------------
	template < typename TI = U >
	static constexpr TI IndexForCapacity( U nHash, U nCapacity ) noexcept
	{
		BALL_ASSERT_MESSAGE( nCapacity && !( nCapacity & ( nCapacity - 1 ) ), "Fibonacci hashing requires a power-of-two capacity" );

		const bits_t nIndexBits = IsConstantEvaluated() ? static_cast< bits_t >( Math_Log2_Floor< U >( nCapacity ) ) : static_cast< bits_t >( PopCount< U >( static_cast< U >( nCapacity - 1 ) ) );

		return Index< TI >( nHash, nIndexBits );
	}
};

/// Convenience spellings parameterized by hash-word width. The unsuffixed alias
/// uses the platform word (`size_t`); the suffixed forms pin the width.
using Hash_t = CFibonacciHash< size_t >;
using Hash8_t = CFibonacciHash< size8_t >;
using Hash16_t = CFibonacciHash< size16_t >;
using Hash32_t = CFibonacciHash< size32_t >;
using Hash64_t = CFibonacciHash< size64_t >;

#endif // !defined( _INCLUDE_BALL_TYPES_HASH_HPP_ )
