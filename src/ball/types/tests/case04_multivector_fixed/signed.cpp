#include "common.hpp"

void Case04_MultiVector_Fixed_Signed( TestsOutput_t &sOut, size_t &nTotal, size_t &nFailed )
{
#define BALL_TEST_RUN_SIGNED( bits ) \
	do \
	{ \
		++nTotal; \
		if ( !RunFixedMultiVectorCase< BTL::FixedSigned##bits##_t, BTL::bits_t( bits ) >( sOut, "FixedSigned" #bits "_t" ) ) \
			++nFailed; \
	} while ( 0 );

	BALL_FIXED_FOR_EACH_BITS( BALL_TEST_RUN_SIGNED );

#undef BALL_TEST_RUN_SIGNED
}
