#include "common.hpp"

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

void Case02_Vector( TestsOutput_t &sOut )
{
	RunVectorCase< CAdapter_Vector >( sOut, "BTL::Vector_t" );
}
