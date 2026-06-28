#ifndef _SRC_BALL_TYPES_TESTS_CASE04_MULTIVECTOR_FIXED_COMMON_HPP_
#	define _SRC_BALL_TYPES_TESTS_CASE04_MULTIVECTOR_FIXED_COMMON_HPP_

#include "../common.hpp"

template < typename F, BTL::bits_t BITS >
bool RunFixedMultiVectorCase( TestsOutput_t &sOut, BTL::StringView_t svFamily )
{
	using T = typename F::Type;
	using Traits_t = BTL::MFixedMetadata< F >;
	using U = typename Traits_t::Unsigned_t;
	using C = BTL::MultiVector_t< F, float >;

	static_assert( Traits_t::BITS == BITS, "MFixed* mapping mismatch" );

	C vec;
	std::vector< F > arrFirst;
	std::vector< float > arrSecond;

	auto funcAddToTail = [&]( T nFirst, float nSecond )
	{
		vec.AddToTail( nFirst, nSecond );
		arrFirst.push_back( nFirst );
		arrSecond.push_back( nSecond );
	};

	auto funcAddToHead = [&]( T nFirst, float nSecond )
	{
		vec.AddToHead( nFirst, nSecond );
		arrFirst.insert( arrFirst.begin(), nFirst );
		arrSecond.insert( arrSecond.begin(), nSecond );
	};

	auto funcInsert = [&]( size_t i, T nFirst, float nSecond )
	{
		vec.Insert( i, nFirst, nSecond );
		arrFirst.insert( arrFirst.begin() + i, nFirst );
		arrSecond.insert( arrSecond.begin() + i, nSecond );
	};

	auto funcInsertMultiple = [&]( size_t i, size_t nCount, T nFirst, float nSecond )
	{
		vec.InsertMultiple( i, nCount, nFirst, nSecond );
		arrFirst.insert( arrFirst.begin() + i, nCount, nFirst );
		arrSecond.insert( arrSecond.begin() + i, nCount, nSecond );
	};

	auto funcRemove = [&]( size_t i, size_t nCount = 1u )
	{
		vec.Remove( i, nCount );
		arrFirst.erase( arrFirst.begin() + i, arrFirst.begin() + i + nCount );
		arrSecond.erase( arrSecond.begin() + i, arrSecond.begin() + i + nCount );
	};

	const U nValueMask = static_cast< U >( Traits_t::VALUE_MASK );
	const U nSignBit = Traits_t::IS_SIGNED ? static_cast< U >( U( 1 ) << ( Traits_t::BITS - 1 ) ) : 0;
	const U nRawPositive = Traits_t::IS_SIGNED ? static_cast< U >( nSignBit - 1 ) : nValueMask;
	const U nRawAlt = Traits_t::IS_SIGNED ? nValueMask : ( nValueMask > 0 ? static_cast< U >( nValueMask - 1 ) : 0 );
	const U nRawEdge = Traits_t::IS_SIGNED ? nSignBit : ( nValueMask > 1 ? static_cast< U >( nValueMask >> 1 ) : nValueMask );

	funcAddToTail( static_cast< T >( 0 ), 10.0f );
	funcAddToTail( static_cast< T >( 1 ), 20.0f );
	funcAddToHead( static_cast< T >( nRawPositive ), 30.0f );
	funcInsert( 1u, static_cast< T >( nRawAlt ), 40.0f );
	funcInsertMultiple( 2u, 2u, static_cast< T >( nRawEdge ), 50.0f );
	funcRemove( 3u );

	bool bOk = vec.Count() == static_cast< size_t >( arrFirst.size() );
	BTL::StringView_t svFailStage = "count";

	if ( bOk )
	{
		svFailStage = "values";

		for ( size_t i = 0; i < arrFirst.size(); ++i )
		{
			const F nFirst = vec.template At< F >( static_cast< size_t >( i ) );
			const float nSecond = vec.template At< float >( static_cast< size_t >( i ) );

			if ( !( nFirst == arrFirst[ i ] ) || !( nSecond == arrSecond[ i ] ) )
			{
				bOk = false;
				break;
			}
		}
	}

	if ( bOk && !arrFirst.empty() )
	{
		svFailStage = "front_back";

		const F nFront0 = vec.template Front< F >();
		const float nFront1 = vec.template Front< float >();
		const F nBack0 = vec.template Back< F >();
		const float nBack1 = vec.template Back< float >();

		if ( !( nFront0 == arrFirst.front() ) || !( nFront1 == arrSecond.front() ) || !( nBack0 == arrFirst.back() ) || !( nBack1 == arrSecond.back() ) )
		{
			bOk = false;
		}
	}

	if ( bOk && !arrFirst.empty() )
	{
		svFailStage = "finds";

		const size_t iProbe = arrFirst.size() / 2u;
		const F nProbeFirst = arrFirst[ iProbe ];
		const float nProbeSecond = arrSecond[ iProbe ];

		size_t iRefFindBy = C::INVALID_INDEX;
		size_t iRefRFindBy = C::INVALID_INDEX;
		size_t iRefFindRow = C::INVALID_INDEX;
		size_t iRefRFindRow = C::INVALID_INDEX;

		for ( size_t i = 0; i < arrFirst.size(); ++i )
		{
			if ( iRefFindBy == C::INVALID_INDEX && arrFirst[ i ] == nProbeFirst )
				iRefFindBy = i;

			if ( iRefFindRow == C::INVALID_INDEX && arrFirst[ i ] == nProbeFirst && arrSecond[ i ] == nProbeSecond )
				iRefFindRow = i;
		}

		for ( size_t i = arrFirst.size(); i > 0; --i )
		{
			const size_t j = i - 1u;

			if ( iRefRFindBy == C::INVALID_INDEX && arrFirst[ j ] == nProbeFirst )
				iRefRFindBy = j;

			if ( iRefRFindRow == C::INVALID_INDEX && arrFirst[ j ] == nProbeFirst && arrSecond[ j ] == nProbeSecond )
				iRefRFindRow = j;
		}

		const size_t iFindBy = vec.template FindBy< F >( nProbeFirst );
		const size_t iRFindBy = vec.template RFindBy< F >( nProbeFirst );
		const size_t iFindRow = vec.Find( nProbeFirst, nProbeSecond );
		const size_t iRFindRow = vec.RFind( nProbeFirst, nProbeSecond );
		const size_t iMissingRow = vec.Find( nProbeFirst, -1.0f );

		if ( iFindBy != iRefFindBy || iRFindBy != iRefRFindBy || iFindRow != iRefFindRow || iRFindRow != iRefRFindRow || vec.IsValidIndex( iMissingRow ) )
			bOk = false;
	}

	if ( !bOk )
		sOut.AppendMultiple( "BTL::MultiVector_t fixed case mismatch: ", svFamily, "<", BITS, "> stage=", svFailStage, "\n" );

	return bOk;
}

void Case04_MultiVector_Fixed_Signed( TestsOutput_t &sOut, size_t &nTotal, size_t &nFailed );
void Case04_MultiVector_Fixed_Uncertain( TestsOutput_t &sOut, size_t &nTotal, size_t &nFailed );
void Case04_MultiVector_Fixed_Unsiged( TestsOutput_t &sOut, size_t &nTotal, size_t &nFailed );

#endif // !defined( _SRC_BALL_TYPES_TESTS_CASE04_MULTIVECTOR_FIXED_COMMON_HPP_ )
