#include "common.hpp"

void Case04_MultiVector_Fixed_Unsiged( TestsOutput_t &sOut, size_t &nTotal, size_t &nFailed )
{
#define BALL_TEST_RUN_UNSIGNED( bits ) \
	do \
	{ \
		++nTotal; \
		if ( !RunFixedMultiVectorCase< BTL::FixedUnsiged##bits##_t, BTL::bits_t( bits ) >( sOut, "FixedUnsiged" #bits "_t" ) ) \
			++nFailed; \
	} while ( 0 );

	BALL_FIXED_FOR_EACH_BITS( BALL_TEST_RUN_UNSIGNED );

#undef BALL_TEST_RUN_UNSIGNED
}
