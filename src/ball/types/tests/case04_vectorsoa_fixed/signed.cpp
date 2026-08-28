module;

#include <ball/types/fixed.h>

#include <vector>

module Ball.Types;

import Ball.New;
import :Core;
import :Fixed;
import :String;
import :StringView;
import :Tests.Case04;
import :Vector;

using TestsOutput_t = BTL::BufferString_t< 4096 >;

#include "common.hpp"

void Case04_VectorSoA_Fixed_Signed( TestsOutput_t &sOut, size_t &nTotal, size_t &nFailed )
{
#define BALL_TEST_RUN_SIGNED( bits ) \
	do \
	{ \
		++nTotal; \
		if ( !RunFixedVectorSoACase< BTL::FixedSigned##bits##_t, BTL::bits_t( bits ) >( sOut, "FixedSigned" #bits "_t" ) ) \
			++nFailed; \
	} while ( 0 );

	BALL_FIXED_FOR_EACH_BITS( BALL_TEST_RUN_SIGNED );

#undef BALL_TEST_RUN_SIGNED
}
