#ifndef _SRC_BALL_TYPES_TESTS_COMMON_HPP_
#	define _SRC_BALL_TYPES_TESTS_COMMON_HPP_

#	ifdef BALL_ENABLE_MODULES
import Ball.Time;
import Ball.Types;
#	else
#		include <ball/time.hpp>
#		include <ball/types.hpp>
#	endif

#	include <algorithm>
#	include <stdint.h>
#	include <stdio.h>
#	include <vector>

using TestsOutput_t = BTL::BufferString_t< 4096 >;
using TestPair_t = BTL::Pair_t< size_t, size_t >;

template < class A >
bool CheckContainerMatches( const typename A::C &container, const std::vector< TestPair_t > &expected, TestsOutput_t &sOut )
{
	const size_t nContainerCount = A::Count( container );
	const size_t nExpectedCount = expected.size();

	bool bMatches = nContainerCount == nExpectedCount;

	if ( bMatches )
	{
		for ( size_t i = 0; i < nContainerCount; ++i )
		{
			if ( !( A::At( container, i ) == expected[ i ] ) )
			{
				bMatches = false;
				break;
			}
		}
	}

	if ( bMatches )
		sOut.AppendMultiple( " - ok (", nContainerCount, " elements)" );
	else
		sOut.AppendMultiple( " - mismatch (C=", nContainerCount, ", Ref=", nExpectedCount, ")" );

	return bMatches;
}

template < class A >
void RunVectorCase( TestsOutput_t &sOut, const BTL::StringView_t &svCaseLabel )
{
	using C = typename A::C;

	C vecContainer;
	std::vector< TestPair_t > vecReference;

	A::Reserve( vecContainer, 16 );

	sOut.AppendMultiple( svCaseLabel, ": First address - ", A::Base( vecContainer ), "\n" );

	auto funcAppendRange = [ & ]( size_t start, size_t count )
	{
		for ( size_t i = 0; i < count; ++i )
		{
			const TestPair_t value{ start + i, ( start + i ) * 3 + 1 };

			A::Append( vecContainer, value );
			vecReference.push_back( value );
		}
	};

	auto funcLogState = [ & ]( const BTL::StringView_t &svLabel, const BTL::CTimeNS &elapsed = BTL::CTimeNS() )
	{
		sOut.AppendMultiple( svCaseLabel, ": ", svLabel );
		CheckContainerMatches< A >( vecContainer, vecReference, sOut );

		if ( elapsed.IsValid() )
			sOut.AppendMultiple( " - elapsed ", elapsed.AsMillisF(), " ms\n" );
		else
			sOut += "\n";
	};

	BALL_PROF_BEGIN( AppendRange );
	funcAppendRange( 0, 10'000'000 );

	sOut.AppendMultiple( svCaseLabel, ": Filled address - ", A::Base( vecContainer ), "\n" );

	auto nsAppendRange = BALL_PROF_END( AppendRange );

	funcLogState( "Initial fill", nsAppendRange );

	if ( A::Count( vecContainer ) >= 2 )
	{
		BALL_PROF_BEGIN( RemoveHead );
		{
			A::Remove( vecContainer, A::FirstIndex( vecContainer ) );
		}
		vecReference.erase( vecReference.begin() );
		funcLogState( "Removed head", BALL_PROF_END( RemoveHead ) );

		if ( A::Count( vecContainer ) > 0 )
		{
			const size_t tailIndex = A::Count( vecContainer ) - 1;

			BALL_PROF_BEGIN( RemoveTail );
			{
				A::Remove( vecContainer, tailIndex );
			}
			vecReference.erase( vecReference.begin() + tailIndex );
			funcLogState( "Removed tail", BALL_PROF_END( RemoveTail ) );
		}
	}

	const TestPair_t middle{ 0xDEAD, 0xBEEF };

	{
		const size_t middleIndex = A::Count( vecContainer ) / 2;

		BALL_PROF_BEGIN( InsertMiddle );
		{
			A::Insert( vecContainer, middleIndex, middle );
		}
		vecReference.insert( vecReference.begin() + middleIndex, middle );
		funcLogState( "Inserted middle element", BALL_PROF_END( InsertMiddle ) );
	}

	const TestPair_t head{ 0xC0DE, 0xFEED };

	{
		BALL_PROF_BEGIN( InsertHead );
		{
			A::Insert( vecContainer, A::FirstIndex( vecContainer ), head );
		}
		vecReference.insert( vecReference.begin(), head );
		funcLogState( "Inserted at head", BALL_PROF_END( InsertHead ) );
	}

	const TestPair_t tail{ 0x1234, 0x5678 };

	{
		BALL_PROF_BEGIN( InsertTail );
		{
			A::Insert( vecContainer, A::Count( vecContainer ), tail );
		}
		vecReference.push_back( tail );
		funcLogState( "Inserted at tail", BALL_PROF_END( InsertTail ) );
	}

	const TestPair_t searchValue = middle;

	BALL_PROF_BEGIN( Find );

	const size_t iFound = A::Find( vecContainer, searchValue );
	const bool bContainerFound = A::IsValidIndex( vecContainer, iFound );

	auto nsFind = BALL_PROF_END( Find );

	const auto itReference = std::find( vecReference.begin(), vecReference.end(), searchValue );
	const bool bReferenceFound = itReference != vecReference.end();
	const size_t iReference = bReferenceFound ? static_cast< size_t >( itReference - vecReference.begin() ) : A::INVALID_INDEX;

	sOut.AppendMultiple( svCaseLabel, ": " );

	if ( bContainerFound )
		sOut.AppendMultiple( "Find: found element at index ", iFound, " " );
	else
		sOut.AppendMultiple( "Find: element not found " );

	if ( bReferenceFound )
		sOut.AppendMultiple( "(ref index ", iReference, ")" );
	else
		sOut.AppendMultiple( "(reference missing)" );

	if ( nsFind.IsValid() )
		sOut.AppendMultiple( " - elapsed ", nsFind.AsMillisF(), " ms\n" );
	else
		sOut += "\n";

	if ( bContainerFound != bReferenceFound || ( bContainerFound && bReferenceFound && iFound != iReference ) )
		sOut.AppendMultiple( svCaseLabel, " - Find mismatch between container and reference\n" );

	BALL_PROF_BEGIN( MissFind );

	const TestPair_t missing{ 0xFFFF, 0xFFFF };
	const size_t iMissing = A::Find( vecContainer, missing );

	auto nsMissFind = BALL_PROF_END( MissFind );

	sOut.AppendMultiple( svCaseLabel, ": " );

	if ( A::IsValidIndex( vecContainer, iMissing ) )
		sOut.AppendMultiple( "Missing element was unexpectedly found at ", iMissing );
	else
		sOut.AppendMultiple( "Missing element not found as expected" );

	if ( nsFind.IsValid() )
		sOut.AppendMultiple( " - elapsed ", nsMissFind.AsMillisF(), " ms\n" );
	else
		sOut += "\n";

	sOut += "---\n";
}

inline void LogDelegateCheck( TestsOutput_t &sOut, BTL::StringView_t svContainerName, BTL::StringView_t svLabel, bool bOk )
{
	sOut.AppendMultiple( svContainerName, ": ", svLabel, ": " );

	if ( bOk )
		sOut += "ok\n";
	else
		sOut += "mismatch\n";
}

void Case01_STLVector( TestsOutput_t &sOut );
void Case02_Vector( TestsOutput_t &sOut );
void Case03_MultiVector( TestsOutput_t &sOut );
void Case04_MultiVector_Fixed( TestsOutput_t &sOut );
void Case05_RBTree( TestsOutput_t &sOut );
void Case06_Reflection( TestsOutput_t &sOut );
void Case07_Hash( TestsOutput_t &sOut );
void Case08_HashMap( TestsOutput_t &sOut );
void Case10_Delegate( TestsOutput_t &sOut );
void Case11_MapBenchmark( TestsOutput_t &sOut );

#endif // !defined( _SRC_BALL_TYPES_TESTS_COMMON_HPP_ )
