module;

#include <ball/time/macros.h>

#include <algorithm>
#include <vector>

module Ball.Types;

import Ball.New;
import Ball.Time;
import :Pair;
import :String;
import :Tests.Case01;

using TestsOutput_t = BTL::BufferString_t< 4096 >;

using TestPair_t = BTL::Pair_t< size_t, size_t >;

#include "vector_case.hpp"

struct CAdapter_StdVector
{
	using C = std::vector< TestPair_t >;
	static constexpr size_t INVALID_INDEX = static_cast< size_t >( -1 );

	static TestPair_t *Base( C &vec ) { return vec.data(); }
	static void Reserve( C &vec, size_t capacity ) { vec.reserve( capacity ); }
	static void Append( C &vec, const TestPair_t &value ) { vec.emplace_back( value ); }
	static size_t Count( const C &vec ) { return vec.size(); }
	static void Remove( C &vec, size_t index ) { vec.erase( vec.begin() + index ); }
	static void Insert( C &vec, size_t index, const TestPair_t &value ) { vec.insert( vec.begin() + index, value ); }
	static size_t Find( const C &vec, const TestPair_t &value )
	{
		const auto it = std::find( vec.begin(), vec.end(), value );
		return it == vec.end() ? INVALID_INDEX : static_cast< size_t >( it - vec.begin() );
	}
	static bool IsValidIndex( const C &vec, size_t index ) { return index < vec.size(); }
	static size_t FirstIndex( const C & ) { return 0; }
	static const TestPair_t &At( const C &vec, size_t index ) { return vec[ index ]; }
};

void Case01_STLVector( TestsOutput_t &sOut )
{
	RunVectorCase< CAdapter_StdVector >( sOut, "std::vector" );
}
