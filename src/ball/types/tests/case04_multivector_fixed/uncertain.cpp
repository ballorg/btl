#include "common.hpp"

void Case04_MultiVector_Fixed_Uncertain( TestsOutput_t &sOut, size_t &nTotal, size_t &nFailed )
{
#define BALL_TEST_RUN_UNCERTAIN( bits ) \
	do \
	{ \
		++nTotal; \
		if ( !RunFixedMultiVectorCase< BTL::FixedUncertain##bits##_t, BTL::bits_t( bits ) >( sOut, "FixedUncertain" #bits "_t" ) ) \
			++nFailed; \
	} while ( 0 );

	BALL_FIXED_FOR_EACH_BITS( BALL_TEST_RUN_UNCERTAIN );

#undef BALL_TEST_RUN_UNCERTAIN
}
