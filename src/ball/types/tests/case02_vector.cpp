module;

#include <ball/types/c/assert/static.h>
#include <ball/time/macros.h>

#include <algorithm>
#include <vector>

module Ball.Types;

import Ball.New;
import Ball.Time;
import :Core;
import :Elements;
import :ElementsPack;
import :Fixed;
import :Math;
import :Pair;
import :String;
import :Tests.Case02;
import :Vector;

using TestsOutput_t = BTL::BufferString_t< 4096 >;

using TestPair_t = BTL::Pair_t< size_t, size_t >;

#include "vector_case.hpp"

struct CAdapter_Vector
{
	using C = BTL::BufferVector_t< 16, TestPair_t >;
	static constexpr size_t INVALID_INDEX = C::INVALID_INDEX;

	static TestPair_t *Base( C &vec ) { return vec.Base(); }
	static void Reserve( C &, size_t ) {}
	static void Append( C &vec, const TestPair_t &value ) { vec.AddToTail( value ); }
	static size_t Count( const C &vec ) { return vec.Count(); }
	static void Remove( C &vec, size_t index ) { vec.Remove( index ); }
	static void Insert( C &vec, size_t index, const TestPair_t &value ) { vec.Insert( index, value ); }
	static size_t Find( const C &vec, const TestPair_t &value ) { return vec.Find( value ); }
	static bool IsValidIndex( const C &vec, size_t index ) { return vec.IsValidIndex( index ); }
	static size_t FirstIndex( const C &vec ) { return vec.FIRST_INDEX; }
	static const TestPair_t &At( const C &vec, size_t index ) { return vec[ index ]; }
};

namespace
{
	using Pack_t = BTL::CElementsPack< size_t, 2, BTL::uint8_t, int, long >;
	using Packed_Pack_t = BTL::CElementsPack< size_t, 2, BTL::uint8_t, BTL::FixedUnsiged3_t >;

	constexpr bool TestElementsPackValues()
	{
		const int nCopy = 17;
		const long nTailCopy = 29;
		Pack_t copyValues( nCopy, nTailCopy );

		int nMove = 31;
		long nTailMove = 43;
		Pack_t moveValues( BTL::Move( nMove ), BTL::Move( nTailMove ) );
		Pack_t copyConstructed( copyValues );
		Pack_t moveConstructed( BTL::Move( moveValues ) );
		Pack_t copyAssigned;
		Pack_t moveAssigned;
		Packed_Pack_t packedValues( BTL::FixedUnsiged3_t( 5 ) );
		Packed_Pack_t packedAssigned;

		copyAssigned = copyConstructed;
		moveAssigned = BTL::Move( moveConstructed );
		packedAssigned = packedValues;

		return copyValues.template FixedBy< 0, int >()[ 0 ] == 17
			&& copyValues.template FixedBy< 1, long >()[ 0 ] == 29
			&& copyAssigned.template FixedBy< int >()[ 0 ] == 17
			&& copyAssigned.template FixedBy< long >()[ 0 ] == 29
			&& moveAssigned.template FixedBy< int >()[ 0 ] == 31
			&& moveAssigned.template FixedBy< long >()[ 0 ] == 43
			&& ( packedAssigned.template Packed_FixedBy< BTL::FixedUnsiged3_t >()[ 0 ] & 7 ) == 5
		;
	}

	bool Packed_TestVectorIterator()
	{
		BTL::BufferVector_t< 4, BTL::FixedUnsiged3_t > values;
		values.AddToTail( BTL::FixedUnsiged3_t( 3 ) );
		values.AddToTail( BTL::FixedUnsiged3_t( 5 ) );

		auto it = values.begin();
		*it = BTL::FixedUnsiged3_t( 6 );

		const auto &constValues = values;
		return static_cast< BTL::uint8_t >( *constValues.begin() ) == 6
			&& static_cast< BTL::uint8_t >( constValues.begin()[ 1 ] ) == 5
			&& values.end() - values.begin() == values.Count();
	}

	bool TestVectorInteractions()
	{
		BTL::BufferVector_t< 4, TestPair_t > inserted;

		inserted += TestPair_t{ 7, 8 };
		inserted += TestPair_t{ 9, 10 };
		inserted.Insert( 1, inserted[ 0 ] );
		inserted.Remove( 0 );

		if ( inserted.Count() != 2 || inserted[ 0 ] != TestPair_t{ 7, 8 } || inserted[ 1 ] != TestPair_t{ 9, 10 } )
			return false;

		BTL::BufferVector_t< 4, TestPair_t > vecBuffered;

		vecBuffered += TestPair_t{ 1, 2 };

		BTL::Vector_t< TestPair_t > vecDynamic;

		vecDynamic.MoveFrom( BTL::Move( vecBuffered ) );

		if ( !vecBuffered.Empty() || vecDynamic.Count() != 1 )
			return false;

		BTL::BufferVector_t< 4, TestPair_t > moved;

		moved = BTL::Move( vecDynamic );

		BTL::Vector_t< TestPair_t > tail;

		tail += TestPair_t{ 3, 4 };
		moved += tail;

		return vecDynamic.Empty() && moved.Count() == 2 && moved[ 0 ] == TestPair_t{ 1, 2 } && moved[ 1 ] == TestPair_t{ 3, 4 };
	}

	bool TestBitCeil()
	{
		return BTL::BitWidth< size_t >( 0 ) == 0
			&& BTL::BitWidth< size_t >( 1 ) == 0
			&& BTL::BitCeil< size_t >( 0 ) == 1
			&& BTL::BitCeil< size_t >( 1 ) == 1
			&& BTL::BitCeil< size_t >( 16 ) == 16
			&& BTL::BitCeil< size_t >( 17 ) == 32;
	}

	bool TestCopyElementsOverlap()
	{
		int arrValues[] = { 1, 2, 3, 4, 5, 6 };

		BTL::CopyElements( 4, &arrValues[ 0 ], &arrValues[ 1 ] );

		return arrValues[ 0 ] == 2
			&& arrValues[ 1 ] == 3
			&& arrValues[ 2 ] == 4
			&& arrValues[ 3 ] == 5
			&& arrValues[ 4 ] == 5
			&& arrValues[ 5 ] == 6;
	}

	BALL_STATIC_ASSERT( TestElementsPackValues(), "CElementsPack value operations must be constant expressions" );
}

void Case02_Vector( TestsOutput_t &sOut )
{
	sOut += "BTL::CElementsPack values: ";

	if ( TestElementsPackValues() )
		sOut += "ok\n";
	else
		sOut += "mismatch\n";

	if ( Packed_TestVectorIterator() )
		sOut += "BTL::CVector packed iterator: ok\n";
	else
		sOut += "BTL::CVector packed iterator: mismatch\n";

	if ( TestVectorInteractions() )
		sOut += "BTL::CVector interactions: ok\n";
	else
		sOut += "BTL::CVector interactions: mismatch\n";

	if ( TestBitCeil() )
		sOut += "BTL::BitCeil: ok\n";
	else
		sOut += "BTL::BitCeil: mismatch\n";

	if ( TestCopyElementsOverlap() )
		sOut += "BTL::CopyElements overlap: ok\n";
	else
		sOut += "BTL::CopyElements overlap: mismatch\n";

	RunVectorCase< CAdapter_Vector >( sOut, "BTL::Vector_t" );
}
