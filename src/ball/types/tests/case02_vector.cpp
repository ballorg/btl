#include "common.hpp"
#include <ball/types/c/assert/static.h>

struct CAdapter_Vector
{
	using C = BTL::BufferVector_t< TestPair_t, 16 >;
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
	using Pack_t = BTL::CElementsPack< size_t, 2, uint8_t, int, long >;
	using PackedPack_t = BTL::CElementsPack< size_t, 2, uint8_t, BTL::FixedUnsiged3_t >;

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
		PackedPack_t packedValues( BTL::FixedUnsiged3_t( 5 ) );
		PackedPack_t packedAssigned;

		copyAssigned = copyConstructed;
		moveAssigned = BTL::Move( moveConstructed );
		packedAssigned = packedValues;

		return copyValues.template FixedBy< 0, int >()[ 0 ] == 17
			&& copyValues.template FixedBy< 1, long >()[ 0 ] == 29
			&& copyAssigned.template FixedBy< int >()[ 0 ] == 17
			&& copyAssigned.template FixedBy< long >()[ 0 ] == 29
			&& moveAssigned.template FixedBy< int >()[ 0 ] == 31
			&& moveAssigned.template FixedBy< long >()[ 0 ] == 43
			&& ( packedAssigned.template PackedFixedBy< BTL::FixedUnsiged3_t >()[ 0 ] & 7 ) == 5
		;
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

	RunVectorCase< CAdapter_Vector >( sOut, "BTL::Vector_t" );
}
