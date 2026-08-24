#include "case04_multivector_fixed/common.hpp"

void Case04_MultiVector_Fixed( TestsOutput_t &sOut )
{
	size_t nTotal = 0u;
	size_t nFailed = 0u;

	Case04_MultiVector_Fixed_Signed( sOut, nTotal, nFailed );
	Case04_MultiVector_Fixed_Uncertain( sOut, nTotal, nFailed );
	Case04_MultiVector_Fixed_Unsiged( sOut, nTotal, nFailed );

	sOut.AppendMultiple( "BTL::Vector_t fixed types: " );

	if ( nFailed == 0u )
		sOut.AppendMultiple( "ok (", nTotal, " cases)\n" );
	else
		sOut.AppendMultiple( "mismatch (failed ", nFailed, " of ", nTotal, ")\n" );

	sOut += "---\n";
}
