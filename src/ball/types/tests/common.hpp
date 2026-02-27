#ifndef _SRC_BALL_TYPES_TESTS_COMMON_HPP_
#	define _SRC_BALL_TYPES_TESTS_COMMON_HPP_

#ifdef BALL_ENABLE_MODULES
import Ball.Time;
import Ball.Types;
#else
#	include <ball/time.hpp>
#	include <ball/types.hpp>
#endif

#include <algorithm>
#include <stdint.h>
#include <stdio.h>
#include <vector>

using TestsOutput_t = BTL::BufferString_t< 4096 >;

struct pair_t
{
	size_t first;
	size_t second;

	bool operator==( const pair_t &rhs ) const { return first == rhs.first && second == rhs.second; }
};

template < class A >
bool CheckContainerMatches( const typename A::C &container, const std::vector< pair_t > &expected, TestsOutput_t &sOut )
{
	const size_t nContainerCount = A::Count( container );
	const size_t nExpectedCount  = expected.size();

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
		sOut.AppendMultiple( "ok (", nContainerCount, " elements)" );
	else
		sOut.AppendMultiple( "mismatch (C=", nContainerCount, ", Ref=", nExpectedCount, ")" );

	return bMatches;
}

template < class A >
void RunVectorCase( TestsOutput_t &sOut, const char *pszCaseLabel )
{
	using C = typename A::C;

	C vecContainer;
	std::vector< pair_t > vecReference;

	A::Reserve( vecContainer, 16 );

	auto funcAppendRange = [&]( size_t start, size_t count )
	{
		for ( size_t i = 0; i < count; ++i )
		{
			const pair_t value{ start + i, ( start + i ) * 3 + 1 };

			A::Append( vecContainer, value );
			vecReference.push_back( value );
		}
	};

	auto funcLogState = [&]( const char *pszLabel, const BTL::CTimeNS &elapsed = BTL::CTimeNS() )
	{
		sOut.AppendMultiple( BTL::StringView_t( pszCaseLabel ), ": ", BTL::StringView_t( pszLabel ) );
		CheckContainerMatches< A >( vecContainer, vecReference, sOut );

		if ( elapsed.IsValid() )
			sOut.AppendMultiple( " - elapsed ", elapsed.AsMillisF(), " ms\n" );
		else
			sOut += "\n";
	};

	BALL_PROF_BEGIN( AppendRange );
	funcAppendRange( 0, 10'000'000 );

	auto nsAppendRange = BALL_PROF_END( AppendRange );

	funcLogState( "Initial fill", nsAppendRange );

	if ( A::Count( vecContainer ) >= 2 )
	{
		BALL_PROF_BEGIN( RemoveHead );
		{
			A::Remove( vecContainer, A::FirstIndex( vecContainer ) );
		}
		funcLogState( "Removed head", BALL_PROF_END( RemoveHead ) );

		vecReference.erase( vecReference.begin() );

		if ( A::Count( vecContainer ) > 0 )
		{
			const size_t tailIndex = A::Count( vecContainer ) - 1;

			BALL_PROF_BEGIN( RemoveTail );
			{
				A::Remove( vecContainer, tailIndex );
			}
			funcLogState( "Removed tail", BALL_PROF_END( RemoveTail ) );

			vecReference.erase( vecReference.begin() + tailIndex );
		}
	}

	const pair_t middle{ 0xDEAD, 0xBEEF };

	{
		const size_t middleIndex = A::Count( vecContainer ) / 2;

		BALL_PROF_BEGIN( InsertMiddle );
		{
			A::Insert( vecContainer, middleIndex, middle );
		}
		funcLogState( "Inserted middle element", BALL_PROF_END( InsertMiddle ) );

		vecReference.insert( vecReference.begin() + middleIndex, middle );
	}

	const pair_t head{ 0xC0DE, 0xFEED };

	{
		BALL_PROF_BEGIN( InsertHead );
		{
			A::Insert( vecContainer, A::FirstIndex( vecContainer ), head );
		}
		funcLogState( "Inserted at head", BALL_PROF_END( InsertHead ) );
		vecReference.insert( vecReference.begin(), head );
	}

	const pair_t tail{ 0x1234, 0x5678 };

	{
		BALL_PROF_BEGIN( InsertTail );
		{
			A::Insert( vecContainer, A::Count( vecContainer ), tail );
		}
		funcLogState( "Inserted at tail", BALL_PROF_END( InsertTail ) );
		vecReference.push_back( tail );
	}

	const pair_t searchValue = middle;

	BALL_PROF_BEGIN( Find );

	const size_t iFound = A::Find( vecContainer, searchValue );
	const bool bContainerFound = A::IsValidIndex( vecContainer, iFound );

	auto nsFind = BALL_PROF_END( Find );

	const auto itReference = std::find( vecReference.begin(), vecReference.end(), searchValue );
	const bool bReferenceFound = itReference != vecReference.end();
	const size_t iReference = bReferenceFound ? static_cast< size_t >( itReference - vecReference.begin() ) : A::INVALID_INDEX;

	sOut.AppendMultiple( BTL::StringView_t( pszCaseLabel ), ": " );

	if ( bContainerFound )
		sOut.AppendMultiple( "Find: found element at index ", iFound, " " );
	else
		sOut.AppendMultiple( "Find: element not found " );

	if ( bReferenceFound )
		sOut.AppendMultiple( "(ref index ", iReference, ")\n" );
	else
		sOut.AppendMultiple( "(reference missing)\n" );

	if ( bContainerFound != bReferenceFound || ( bContainerFound && bReferenceFound && iFound != iReference ) )
		sOut.AppendMultiple( BTL::StringView_t( pszCaseLabel ), " - Find mismatch between container and reference\n" );

	const pair_t missing{ 0xFFFF, 0xFFFF };
	const size_t iMissing = A::Find( vecContainer, missing );

	sOut.AppendMultiple( BTL::StringView_t( pszCaseLabel ), ": " );

	if ( A::IsValidIndex( vecContainer, iMissing ) )
		sOut.AppendMultiple( "Missing element was unexpectedly found at ", iMissing );
	else
		sOut.AppendMultiple( "Missing element not found as expected" );

	if ( nsFind.IsValid() )
		sOut.AppendMultiple( " - elapsed ", nsFind.AsMillisF(), " ms\n" );
	else
		sOut += "\n";

	sOut += "---\n";
}

void Case01_STLVector( TestsOutput_t &sOut );
void Case02_Vector( TestsOutput_t &sOut );
void Case03_MultiVector( TestsOutput_t &sOut );
void Case04_MultiVector_Fixed( TestsOutput_t &sOut );

#endif // !defined( _SRC_BALL_TYPES_TESTS_COMMON_HPP_ )
