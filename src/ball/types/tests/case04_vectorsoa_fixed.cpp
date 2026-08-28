module;

#include <ball/types/fixed.h>

#include <vector>

module Ball.Types;

import Ball.New;
import :String;
import :Tests.Case04;
import :Vector;

using TestsOutput_t = BTL::BufferString_t< 4096 >;

#include "case04_vectorsoa_fixed/common.hpp"

void Case04_VectorSoA_Fixed( TestsOutput_t &sOut )
{
	size_t nTotal = 0u;
	size_t nFailed = 0u;

	Case04_VectorSoA_Fixed_Signed( sOut, nTotal, nFailed );
	Case04_VectorSoA_Fixed_Uncertain( sOut, nTotal, nFailed );
	Case04_VectorSoA_Fixed_Unsiged( sOut, nTotal, nFailed );

	sOut.AppendMultiple( "BTL::Vector_t fixed types: " );

	if ( nFailed == 0u )
		sOut.AppendMultiple( "ok (", nTotal, " cases)\n" );
	else
		sOut.AppendMultiple( "mismatch (failed ", nFailed, " of ", nTotal, ")\n" );

	sOut += "---\n";
}
