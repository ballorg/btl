#include "common.hpp"

#include <string>
#include <vector>

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

	// ---- Compile-time properties (each gated by a static_assert) --------------
	// Reaching Case07_Hash at run time already proves everything in this block.

	// The generic top-N-bits derivation reproduces the canonical golden-ratio
	// multipliers and keeps them odd at every width.
	template < typename T > constexpr T MULTIPLIER = BTL::MFibonacci< T >::MULTIPLIER;

	static_assert( MULTIPLIER< BTL::uint8_t >  == 0x9Fu, "8-bit A_N" );
	static_assert( MULTIPLIER< BTL::uint16_t > == 0x9E37u, "16-bit A_N" );
	static_assert( MULTIPLIER< BTL::uint32_t > == 0x9E3779B9u, "32-bit A_N" );
	static_assert( MULTIPLIER< BTL::uint64_t > == 0x9E3779B97F4A7C15ull, "64-bit A_N" );
	static_assert( BTL::MFibonacci< bool >::BITS == 1u, "1-bit width" );
	static_assert( BTL::MFibonacci< bool >::MULTIPLIER, "1-bit A_N" );
	static_assert( BTL::MFibonacci< BTL::uint8_t >::ReferenceMultiplier() == 0x9Fu, "consteval multiplier path" );
	static_assert( BTL::MFibonacci< BTL::uint64_t >::ReferenceOffset() == BTL::MFibonacci< BTL::uint64_t >::OFFSET_BASIS, "consteval offset path" );
	static_assert( ( MULTIPLIER< BTL::uint8_t > & MULTIPLIER< BTL::uint16_t > & MULTIPLIER< BTL::uint32_t > & MULTIPLIER< BTL::uint64_t > & 1u ) != 0, "A_N must be odd" );

	// Fibonacci numbers resolve on demand (base cases and larger indices).
	static_assert( sizeof( BTL::fib_t ) * 8u == 64u, "fib_t must be the 64-bit Fibonacci word" );
	static_assert( BTL::Fibonacci_Number( 0 )  == 0ull, "F(0)" );
	static_assert( BTL::Fibonacci_Number( 1 )  == 1ull, "F(1)" );
	static_assert( BTL::Fibonacci_Number( 10 ) == 55ull, "F(10)" );
	static_assert( BTL::Fibonacci_Number( 50 ) == 12586269025ull, "F(50)" );
	static_assert( BTL::Fibonacci_Number( BTL::uint8_t( 10 ) ) == 55ull, "8-bit Fibonacci index" );
	static_assert( BTL::Fibonacci_Number( BTL::uint64_t( 50 ) ) == 12586269025ull, "64-bit Fibonacci index" );
	static_assert( BTL::Fibonacci_NumberConst( 50 ) == BTL::Fibonacci_Number( 50 ), "consteval Fibonacci path" );
	static_assert( BTL::Fibonacci_NumberConst( BTL::uint16_t( 10 ) ) == 55ull, "templated consteval Fibonacci path" );

	using Hash90_t = BTL::CFibonacciHash< size_t, 90 >;
	static_assert( Hash90_t::FOLD_SEED == BTL::Fibonacci_NumberConst( 90 ), "Fibonacci fold seed must be parameterized" );

	// The compile-time hash is a genuine constant expression -- usable both as a
	// static_assert operand and as a non-type template argument.
	constexpr BTL::StringView_t s_ctView( "compile-time" );
	constexpr BTL::uint32_t s_ctHash = BTL::Hash32_t::Make( s_ctView );
	constexpr BTL::StringView_t s_runtimeReferenceView( "fibonacci-hashing" );
	constexpr BTL::uint32_t s_runtimeReferenceHash = BTL::Hash32_t::Make( s_runtimeReferenceView );

	template < BTL::uint32_t V >
	struct MNonTypeProbe { static constexpr BTL::uint32_t VALUE = V; };

	static_assert( MNonTypeProbe< s_ctHash >::VALUE == BTL::Hash32_t::Make( s_ctView ), "hash is not a constant expression" );
	static_assert( BTL::Hash32_t::Make( 42 ) == 42, "integer hash is not a constant expression" );
	static_assert( BTL::Hash32_t::Make( s_ctView ) == s_ctHash, "view-compatible hash is not a constant expression" );
	static_assert( BTL::Hash32_t::Index( s_ctHash, 6 ) < 64, "Index is not a constant expression" );
	static_assert( BTL::Hash32_t::IndexForCapacity( s_ctHash, 1 ) == 0, "single-bucket index must be zero" );
	static_assert( BTL::Hash32_t::IndexForCapacity( s_ctHash, 64 ) == BTL::Hash32_t::Index( s_ctHash, 6 ), "IndexForCapacity is not a constant expression" );
}

void Case07_Hash( TestsOutput_t &sOut )
{
	bool bAllOk = true;

	sOut += "---\n";

	// The static_asserts above gate compilation; surface each group as a line.
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
