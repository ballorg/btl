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

	BALL_PROF_BEGIN( Case10 );
	Case10_Delegate( str );
	auto nsCase10 = BALL_PROF_END( Case10 );

	str.AppendMultiple( "std::vector", ": ", "Done in ", nsCase01.AsMillisF(), " milliseconds\n" );
	str.AppendMultiple( "BTL::Vector_t", ": ", "Done in ", nsCase02.AsMillisF(), " milliseconds\n" );
	str += "---\n";
	str.AppendMultiple( "BTL::MultiVector_t", ": ", "Done in ", nsCase03.AsMillisF(), " milliseconds\n" );
	str.AppendMultiple( "BTL::MultiVector_t fixed", ": ", "Done in ", nsCase04.AsMillisF(), " milliseconds\n" );
	str.AppendMultiple( "BTL::Delegate_t", ": ", "Done in ", nsCase10.AsMillisF(), " milliseconds\0" );

	puts( str.String() );
	return 0;
}
