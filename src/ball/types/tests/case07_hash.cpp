module;

#include <ball/types/c/assert/static.h>
#include <ball/time/macros.h>

#include <string>
#include <vector>

module Ball.Types;

import Ball.New;
import Ball.Time;
import :Bits;
import :Core;
import :Hash;
import :Math;
import :String;
import :StringView;
import :Tests.Case07;

using TestsOutput_t = BTL::BufferString_t< 4096 >;

namespace
{
	static void LogHashCheck( TestsOutput_t &sOut, BTL::StringView_t svLabel, bool bOk )
	{
		sOut.AppendMultiple( "BTL::Hash_t: ", svLabel, ": " );
		if ( bOk )
			sOut += "ok\n";
		else
			sOut += "mismatch\n";
	}

	template < typename U >
	static bool CheckCapacityIndexing()
	{
		volatile U vHash = 0xA5u;
		U nCapacity = 1;

		for ( ;; )
		{
			volatile U vCapacity = nCapacity;
			const U nRuntimeCapacity = vCapacity;
			const U nHash = vHash;
			const BTL::bits_t nIndexBits = static_cast< BTL::bits_t >( BTL::Math_Log2_Floor( nRuntimeCapacity ) );

			if ( BTL::CFibonacciHash< U >::Make( nHash ) != nHash || BTL::CFibonacciHash< U >::Index( nHash, nIndexBits ) != BTL::CFibonacciHash< U >::IndexForCapacity( nHash, nRuntimeCapacity ) )
				return false;

			if ( BTL::Math_Log2_Floor( nCapacity ) == BTL::MFibonacci< U >::BITS - 1 )
				break;

			nCapacity <<= 1;
		}

		return true;
	}

	// ---- Compile-time properties (each gated by BALL_STATIC_ASSERT) -----------
	// Reaching Case07_Hash at run time already proves everything in this block.

	// The generic top-N-bits derivation reproduces the canonical golden-ratio
	// multipliers and keeps them odd at every width.
	template < typename T > constexpr T MULTIPLIER = BTL::MFibonacci< T >::MULTIPLIER;

	BALL_STATIC_ASSERT( MULTIPLIER< BTL::uint8_t >  == 0x9Fu, "The 8-bit golden-ratio multiplier must match the canonical value" );
	BALL_STATIC_ASSERT( MULTIPLIER< BTL::uint16_t > == 0x9E37u, "The 16-bit golden-ratio multiplier must match the canonical value" );
	BALL_STATIC_ASSERT( MULTIPLIER< BTL::uint32_t > == 0x9E3779B9u, "The 32-bit golden-ratio multiplier must match the canonical value" );
	BALL_STATIC_ASSERT( MULTIPLIER< BTL::uint64_t > == 0x9E3779B97F4A7C15ull, "The 64-bit golden-ratio multiplier must match the canonical value" );
	BALL_STATIC_ASSERT( BTL::MFibonacci< bool >::BITS == 1u, "The bool Fibonacci word must be one bit wide" );
	BALL_STATIC_ASSERT( BTL::MFibonacci< bool >::MULTIPLIER, "The one-bit golden-ratio multiplier must be non-zero" );
	BALL_STATIC_ASSERT( BTL::MFibonacci< BTL::uint8_t >::ReferenceMultiplier() == 0x9Fu, "The consteval multiplier path must produce the canonical value" );
	BALL_STATIC_ASSERT( BTL::MFibonacci< BTL::uint64_t >::ReferenceOffset() == BTL::MFibonacci< BTL::uint64_t >::OFFSET_BASIS, "The consteval offset path must match OFFSET_BASIS" );
	BALL_STATIC_ASSERT( ( MULTIPLIER< BTL::uint8_t > & MULTIPLIER< BTL::uint16_t > & MULTIPLIER< BTL::uint32_t > & MULTIPLIER< BTL::uint64_t > & 1u ) != 0, "Every golden-ratio multiplier must be odd" );

	// Fibonacci numbers resolve on demand (base cases and larger indices).
	BALL_STATIC_ASSERT( sizeof( BTL::fib_t ) * 8u == 64u, "fib_t must be the 64-bit Fibonacci word" );
	BALL_STATIC_ASSERT( BTL::Fibonacci_Number( 0 )  == 0ull, "Fibonacci number F(0) must equal 0" );
	BALL_STATIC_ASSERT( BTL::Fibonacci_Number( 1 )  == 1ull, "Fibonacci number F(1) must equal 1" );
	BALL_STATIC_ASSERT( BTL::Fibonacci_Number( 10 ) == 55ull, "Fibonacci number F(10) must equal 55" );
	BALL_STATIC_ASSERT( BTL::Fibonacci_Number( 50 ) == 12586269025ull, "Fibonacci number F(50) must match the reference value" );
	BALL_STATIC_ASSERT( BTL::Fibonacci_Number( BTL::uint8_t( 10 ) ) == 55ull, "Fibonacci_Number must accept an 8-bit index" );
	BALL_STATIC_ASSERT( BTL::Fibonacci_Number( BTL::uint64_t( 50 ) ) == 12586269025ull, "Fibonacci_Number must accept a 64-bit index" );
	BALL_STATIC_ASSERT( BTL::Fibonacci_NumberConst( 50 ) == BTL::Fibonacci_Number( 50 ), "The consteval Fibonacci path must match the constexpr path" );
	BALL_STATIC_ASSERT( BTL::Fibonacci_NumberConst( BTL::uint16_t( 10 ) ) == 55ull, "The templated consteval Fibonacci path must accept a 16-bit index" );

	using Hash90_t = BTL::CFibonacciHash< size_t, 90 >;
	BALL_STATIC_ASSERT( Hash90_t::FOLD_SEED == BTL::Fibonacci_NumberConst( 90 ), "The Fibonacci fold seed must follow the hash-policy index" );

	// The compile-time hash is a genuine constant expression -- usable both as a
	// compile-time assertion operand and as a non-type template argument.
	constexpr BTL::StringView_t s_ctView( "compile-time" );
	constexpr BTL::uint32_t s_ctHash = BTL::Hash32_t::Make( s_ctView );
	constexpr BTL::StringView_t s_runtimeReferenceView( "fibonacci-hashing" );
	constexpr BTL::uint32_t s_runtimeReferenceHash = BTL::Hash32_t::Make( s_runtimeReferenceView );
	constexpr BTL::uint32_t s_ctSingleBucketRequest = 1;
	constexpr BTL::uint32_t s_ctSixtyFourBucketRequest = 63;

	template < BTL::uint32_t V >
	struct MNonTypeProbe { static constexpr BTL::uint32_t VALUE = V; };

	BALL_STATIC_ASSERT( MNonTypeProbe< s_ctHash >::VALUE == BTL::Hash32_t::Make( s_ctView ), "A compile-time hash must be usable as a non-type template argument" );
	BALL_STATIC_ASSERT( BTL::Hash32_t::Make( 42 ) == 42, "Integer hashing must be a constant expression" );
	BALL_STATIC_ASSERT( BTL::Hash32_t::Make( s_ctView ) == s_ctHash, "String-view hashing must be a constant expression" );
	BALL_STATIC_ASSERT( BTL::Hash32_t::Index( s_ctHash, 6 ) < 64, "Index must produce a constant-expression bucket inside the requested range" );
	BALL_STATIC_ASSERT( BTL::BitWidth_Unified( BTL::uint32_t( 64 ) ) == 6, "BitWidth_Unified must produce log2 for a power-of-two capacity" );
	BALL_STATIC_ASSERT( BTL::BitWidth_Const( BTL::uint32_t( 65 ) ) == 7, "BitWidth_Const must round a non-power-of-two request to the next index width" );
	BALL_STATIC_ASSERT( BTL::Hash32_t::IndexForCapacity< s_ctSingleBucketRequest >( s_ctHash ) == 0, "A single-bucket table must always select bucket zero" );
	BALL_STATIC_ASSERT( BTL::Hash32_t::IndexForCapacity< s_ctSixtyFourBucketRequest >( s_ctHash ) == BTL::Hash32_t::Index( s_ctHash, 6 ), "IndexForCapacity must round a compile-time bucket request up to a power of two" );
}

void Case07_Hash( TestsOutput_t &sOut )
{
	bool bAllOk = true;

	sOut += "---\n";

	// The named BALL_STATIC_ASSERT checks above gate compilation; surface each
	// human-readable group as a run-time test line as well.
	LogHashCheck( sOut, "golden-ratio multipliers (canonical, odd)", true );
	LogHashCheck( sOut, "Fibonacci numbers on demand", true );
	LogHashCheck( sOut, "compile-time hashing (constant expression, seedable)", true );

	// Hashing a key at run time must be bit-identical to hashing it at compile
	// time. The run-time inputs are laundered so they cannot be constant-folded.
	{
		volatile size_t vInt = 123456789;
		std::string sStr = "fibonacci-hashing";
		const BTL::StringView_t svStr( sStr.c_str() );

		const bool bIntOk = BTL::Hash32_t::Make( vInt ) == BTL::Hash32_t::Make( 123456789 );
		const bool bStrOk = BTL::Hash32_t::Make( svStr ) == s_runtimeReferenceHash;

		bAllOk &= bIntOk;
		bAllOk &= bStrOk;
		LogHashCheck( sOut, "run-time == compile-time (integer key)", bIntOk );
		LogHashCheck( sOut, "run-time == compile-time (string key)", bStrOk );
	}

	{
		bool bOk = true;

		bOk &= CheckCapacityIndexing< BTL::uint8_t >();
		bOk &= CheckCapacityIndexing< BTL::uint16_t >();
		bOk &= CheckCapacityIndexing< BTL::uint32_t >();
		bOk &= CheckCapacityIndexing< BTL::uint64_t >();
		bAllOk &= bOk;
		LogHashCheck( sOut, "capacity indexing (8/16/32/64-bit)", bOk );
	}

	// A spread of keys should scatter near-uniformly over a power-of-two table.
	// A loose chi-squared ceiling catches gross clustering; the whole table must
	// be reached.
	{
		constexpr size_t nBuckets = 1024;
		constexpr size_t nKeys = 16384;
		constexpr BTL::bits_t nBits = 10; // log2( nBuckets )

		std::vector< size_t > vecCounts( nBuckets, 0 );

		BALL_PROF_BEGIN( HashDistribution );

		for ( size_t i = 0; i < nKeys; ++i )
		{
			const size_t nKey = i * 2654435761u + 12345u; // odd stride, so keys are not contiguous

			++vecCounts[ BTL::Hash32_t::Index< size_t >( BTL::Hash32_t::Make( nKey ), nBits ) ];
		}

		auto nsDist = BALL_PROF_END( HashDistribution );

		const double dExpected = double( nKeys ) / double( nBuckets );
		double dChiSquared = 0.0;
		size_t nUsed = 0;

		for ( size_t nCount : vecCounts )
		{
			if ( nCount != 0 )
				++nUsed;

			const double dDelta = double( nCount ) - dExpected;

			dChiSquared += ( dDelta * dDelta ) / dExpected;
		}

		bool bOk = true;

		bOk &= dChiSquared < 1500.0;
		bOk &= nUsed == nBuckets;
		bAllOk &= bOk;

		const size_t nChiSquared = static_cast< size_t >( dChiSquared );

		sOut.AppendMultiple( "BTL::Hash_t: distribution: chi2=", nChiSquared, ", used ", nUsed, "/", nBuckets, ", ", nsDist.AsMillisF(), " ms: " );

		if ( bOk )
			sOut += "ok\n";
		else
			sOut += "mismatch\n";
	}

	sOut.AppendMultiple( "BTL::Hash_t: " );

	if ( bAllOk )
		sOut += "ok\n";
	else
		sOut += "mismatch\n";

	sOut += "---\n";
}
