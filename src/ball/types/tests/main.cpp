#include "common.hpp"

int main()
{
	TestsOutput_t str;

	BALL_PROF_BEGIN( Case01 );
	Case01_STLVector( str );
	auto nsCase01 = BALL_PROF_END( Case01 );

	BALL_PROF_BEGIN( Case02 );
	Case02_Vector( str );
	auto nsCase02 = BALL_PROF_END( Case02 );

	BALL_PROF_BEGIN( Case03 );
	Case03_MultiVector( str );
	auto nsCase03 = BALL_PROF_END( Case03 );

	BALL_PROF_BEGIN( Case04 );
	Case04_MultiVector_Fixed( str );
	auto nsCase04 = BALL_PROF_END( Case04 );

	BALL_PROF_BEGIN( Case05 );
	Case05_RBTree( str );
	auto nsCase05 = BALL_PROF_END( Case05 );

	BALL_PROF_BEGIN( Case06 );
	Case06_Reflection( str );
	auto nsCase06 = BALL_PROF_END( Case06 );

	BALL_PROF_BEGIN( Case07 );
	Case07_Hash( str );
	auto nsCase07 = BALL_PROF_END( Case07 );

	BALL_PROF_BEGIN( Case08 );
	Case08_HashMap( str );
	auto nsCase08 = BALL_PROF_END( Case08 );

	BALL_PROF_BEGIN( Case10 );
	Case10_Delegate( str );
	auto nsCase10 = BALL_PROF_END( Case10 );

	BALL_PROF_BEGIN( Case11 );
	Case11_MapBenchmark( str );
	auto nsCase11 = BALL_PROF_END( Case11 );

	str.AppendMultiple( "std::vector", ": ", "Done in ", nsCase01.AsMillisF(), " milliseconds\n" );
	str.AppendMultiple( "BTL::Vector_t", ": ", "Done in ", nsCase02.AsMillisF(), " milliseconds\n" );
	str += "---\n";
	str.AppendMultiple( "BTL::MultiVector_t", ": ", "Done in ", nsCase03.AsMillisF(), " milliseconds\n" );
	str.AppendMultiple( "BTL::MultiVector_t fixed", ": ", "Done in ", nsCase04.AsMillisF(), " milliseconds\n" );
	str.AppendMultiple( "BTL::RBTree_t", ": ", "Done in ", nsCase05.AsMillisF(), " milliseconds\n" );
	str.AppendMultiple( "BTL::Reflection_t", ": ", "Done in ", nsCase06.AsMillisF(), " milliseconds\n" );
	str.AppendMultiple( "BTL::Hash_t", ": ", "Done in ", nsCase07.AsMillisF(), " milliseconds\n" );
	str.AppendMultiple( "BTL::HashMap_t", ": ", "Done in ", nsCase08.AsMillisF(), " milliseconds\n" );
	str.AppendMultiple( "BTL::Delegate_t", ": ", "Done in ", nsCase10.AsMillisF(), " milliseconds\n" );
	str.AppendMultiple( "BTL::MapBenchmark_t", ": ", "Done in ", nsCase11.AsMillisF(), " milliseconds\0" );

	puts( str.String() );
	return 0;
}
