#ifdef BALL_ENABLE_MODULES
// import Ball.New;
import Ball.Types;
#else // !defined( BALL_ENABLE_MODULES )
// #	include <ball/new.hpp>
#	include <ball/time.hpp>
#	include <ball/types.hpp>
#endif // defined( BALL_ENABLE_MODULES )

#include <stdio.h>
#include <algorithm>
#include <vector>

struct pair_t
{
	size_t first;
	size_t second;

	bool operator==( const pair_t &diff ) const { return first == diff.first && second == diff.second; }
};

template < typename It, class S >
void AppendPairSequence( It itFirst, It itLast, S &sOut )
{
	for ( It it = itFirst; it != itLast; ++it )
	{
		sOut.AppendMultiple( "{", it->first, ", ", it->second, "} " );
	}
}

struct CAdapter_Vector
{
	using C = BTL::BufferVector_t< pair_t, 16 >;
	static constexpr size_t INVALID_INDEX = C::INVALID_INDEX;

	static void Reserve( C &, size_t ) {}
	static void Append( C &vec, const pair_t &value ) { vec.AddToTail( value ); }
	static size_t Count( const C &vec ) { return vec.Count(); }
	static void Remove( C &vec, size_t index ) { vec.Remove( index ); }
	static void Insert( C &vec, size_t index, const pair_t &value ) { vec.Insert( index, value ); }
	static size_t Find( const C &vec, const pair_t &value ) { return vec.Find( value ); }
	static bool IsValidIndex( const C &vec, size_t index ) { return vec.IsValidIndex( index ); }
	static size_t FirstIndex( const C &vec ) { return vec.FIRST_INDEX; }
	static const pair_t &At( const C &vec, size_t index ) { return vec[ index ]; }
};

struct CAdapter_StdVector
{
	using C = std::vector< pair_t >;
	static constexpr size_t INVALID_INDEX = static_cast< size_t >( -1 );

	static void Reserve( C &vec, size_t capacity ) { vec.reserve( capacity ); }
	static void Append( C &vec, const pair_t &value ) { vec.emplace_back( value ); }
	static size_t Count( const C &vec ) { return vec.size(); }
	static void Remove( C &vec, size_t index ) { vec.erase( vec.begin() + index ); }
	static void Insert( C &vec, size_t index, const pair_t &value ) { vec.insert( vec.begin() + index, value ); }
	static size_t Find( const C &vec, const pair_t &value )
	{
		const auto it = std::find( vec.begin(), vec.end(), value );
		return it == vec.end() ? INVALID_INDEX : static_cast< size_t >( it - vec.begin() );
	}
	static bool IsValidIndex( const C &vec, size_t index ) { return index < vec.size(); }
	static size_t FirstIndex( const C & ) { return 0; }
	static const pair_t &At( const C &vec, size_t index ) { return vec[ index ]; }
};

template < class A, class S >
bool CheckContainerMatches( const typename A::C &container, const std::vector< pair_t > &expected, S &sOut )
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
	{
		sOut.AppendMultiple( "ok (", nContainerCount, " elements)\n" );
	}
	else
	{
		sOut.AppendMultiple( "mismatch (C=", nContainerCount, ", Ref=", nExpectedCount, ")\n\n" );
	}

	return bMatches;
}

template < class A, class S >
void RunVectorCase( S &sOut, const char *pszCaseLabel )
{
	using C = typename A::C;

	C vecContainer;
	std::vector< pair_t > vecReference;

	A::Reserve( vecContainer, 16 );

	auto AppendRange = [&]( size_t start, size_t count )
	{
		for ( size_t i = 0; i < count; ++i )
		{
			const pair_t value{ start + i, ( start + i ) * 3 + 1 };

			A::Append( vecContainer, value );
			vecReference.push_back( value );
		}
	};

	auto funcLogState = [&]( const char *pszLabel )
	{
		sOut.AppendMultiple( BTL::StringView_t( pszCaseLabel ), ": ", BTL::StringView_t( pszLabel ), ": " );
		CheckContainerMatches< A >( vecContainer, vecReference, sOut );
	};

	AppendRange( 0, 10'000'000 );
	funcLogState( "Initial fill" );

	if ( A::Count( vecContainer ) >= 2 )
	{
		A::Remove( vecContainer, A::FirstIndex( vecContainer ) );
		vecReference.erase( vecReference.begin() );
		funcLogState( "Removed head" );

		if ( A::Count( vecContainer ) > 0 )
		{
			const size_t tailIndex = A::Count( vecContainer ) - 1;

			A::Remove( vecContainer, tailIndex );
			vecReference.erase( vecReference.begin() + tailIndex );
			funcLogState( "Removed tail" );
		}
	}

	const pair_t middle{ 0xDEAD, 0xBEEF };

	{
		const size_t middleIndex = A::Count( vecContainer ) / 2;

		A::Insert( vecContainer, middleIndex, middle );
		vecReference.insert( vecReference.begin() + middleIndex, middle );
		funcLogState( "Inserted middle element" );
	}

	const pair_t head{ 0xC0DE, 0xFEED };

	{
		A::Insert( vecContainer, A::FirstIndex( vecContainer ), head );
		vecReference.insert( vecReference.begin(), head );
		funcLogState( "Inserted at head" );
	}

	const pair_t tail{ 0x1234, 0x5678 };

	{
		A::Insert( vecContainer, A::Count( vecContainer ), tail );
		vecReference.push_back( tail );
		funcLogState( "Inserted at tail" );
	}

	const pair_t searchValue = middle;
	const size_t iFound = A::Find( vecContainer, searchValue );
	const bool bContainerFound = A::IsValidIndex( vecContainer, iFound );
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
	{
		sOut.AppendMultiple( BTL::StringView_t( pszCaseLabel ), " - Find mismatch between container and reference\n" );
	}

	const pair_t missing{ 0xFFFF, 0xFFFF };
	const size_t iMissing = A::Find( vecContainer, missing );

	sOut.AppendMultiple( BTL::StringView_t( pszCaseLabel ), ": " );

	if ( A::IsValidIndex( vecContainer, iMissing ) )
		sOut.AppendMultiple( "Missing element was unexpectedly found at ", iMissing, "\n" );
	else
		sOut.AppendMultiple( "Missing element not found as expected\n" );

	sOut += "---\n";
}

template < class S >
void Case01_Vector( S &sOut )
{
	RunVectorCase< CAdapter_Vector >( sOut, "BTL::Vector_t" );
}

template < class S >
void Case02_STLVector( S &sOut )
{
	RunVectorCase< CAdapter_StdVector >( sOut, "std::vector" );
}

// Entry point section.
int main()
{
	BTL::BufferString_t< 2000 > str;

	BALL_PROF_BEGIN( Case01 );

	Case01_Vector( str );

	auto nsCase01 = BALL_PROF_END( Case01 );

	BALL_PROF_BEGIN( Case02 );

	Case02_STLVector( str );

	auto nsCase02 = BALL_PROF_END( Case02 );

	str.AppendMultiple( "BTL::Vector_t", ": ", "Done in ", nsCase01.AsMillisF(), " milliseconds\n" );
	str.AppendMultiple( "std::vector", ": ", "Done in ", nsCase02.AsMillisF(), " milliseconds\0" );

	puts( str.String() );

	return 0;
}
