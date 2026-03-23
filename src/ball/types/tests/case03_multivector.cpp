#include "common.hpp"

void Case03_MultiVector( TestsOutput_t &sOut )
{
	using C = BTL::MultiVector_t< size_t, uint32_t >;

	C vec;
	std::vector< TestPair_t > ref;

	auto funcCheck = [&]( const char *pszLabel )
	{
		const size_t nVec = vec.Count();
		const size_t nRef = ref.size();
		bool bOk = ( nVec == nRef );

		if ( bOk )
		{
			for ( size_t i = 0; i < nVec; ++i )
			{
				if ( vec.template At< size_t >( i ) != ref[ i ].First() )
				{
					bOk = false;
					break;
				}

				if ( vec.template At< uint32_t >( i ) != static_cast< uint32_t >( ref[ i ].Second() ) )
				{
					bOk = false;
					break;
				}
			}
		}

		sOut.AppendMultiple( "BTL::MultiVector_t: ", BTL::StringView_t( pszLabel ), ": " );

		if ( bOk )
			sOut.AppendMultiple( "ok (", nVec, " elements)\n" );
		else
			sOut.AppendMultiple( "mismatch (C=", nVec, ", Ref=", nRef, ")\n" );
	};

	auto funcAppendRange = [&]( size_t nStart, size_t nCount )
	{
		for ( size_t i = 0; i < nCount; ++i )
		{
			const size_t nFirst = nStart + i;
			const uint32_t nSecond = static_cast< uint32_t >( nFirst * 3 + 1 );

			vec.AddToTail( nFirst, nSecond );
			ref.push_back( { nFirst, nSecond } );
		}
	};

	funcAppendRange( 0, 10'000'000 );
	funcCheck( "Initial fill" );

	vec.AddToTail( size_t( 1 ), uint32_t( 11 ) );
	ref.push_back( { 1, 11 } );
	vec.AddToTail( size_t( 2 ), uint32_t( 22 ) );
	ref.push_back( { 2, 22 } );
	funcCheck( "AddToTail" );

	vec.AddToHead( size_t( 0 ), uint32_t( 0 ) );
	ref.insert( ref.begin(), { 0, 0 } );
	funcCheck( "AddToHead" );

	vec.Insert( 1, size_t( 9 ), uint32_t( 99 ) );
	ref.insert( ref.begin() + 1, { 9, 99 } );
	funcCheck( "Insert single row" );

	vec.InsertMultiple( 2, 2, size_t( 5 ), uint32_t( 50 ) );
	ref.insert( ref.begin() + 2, 2, TestPair_t{ 5, 50 } );
	funcCheck( "InsertMultiple" );

	vec.AddMultipleToHead( 2, size_t( 8 ), uint32_t( 80 ) );
	ref.insert( ref.begin(), 2, TestPair_t{ 8, 80 } );
	funcCheck( "AddMultipleToHead" );

	vec.AddMultipleToTail( 3, size_t( 7 ), uint32_t( 70 ) );
	ref.insert( ref.end(), 3, TestPair_t{ 7, 70 } );
	funcCheck( "AddMultipleToTail" );

	vec.Remove( 1 );
	ref.erase( ref.begin() + 1 );
	funcCheck( "Remove one" );

	vec.Remove( 2, 3 );
	ref.erase( ref.begin() + 2, ref.begin() + 5 );
	funcCheck( "Remove range" );

	if ( !ref.empty() )
	{
		vec.template At< uint32_t >( 0 ) += uint32_t( 1 );
		ref[ 0 ].Second() += 1;

		const bool bFrontOk = vec.template Front< size_t >() == ref.front().First() && vec.template Front< uint32_t >() == static_cast< uint32_t >( ref.front().First() );
		const bool bBackOk = vec.template Back< size_t >() == ref.back().First() && vec.template Back< uint32_t >() == static_cast< uint32_t >( ref.back().Second() );

		sOut.AppendMultiple( "BTL::MultiVector_t: typed access: " );

		if ( bFrontOk && bBackOk )
			sOut += "ok\n";
		else
			sOut += "mismatch\n";
	}

	auto funcFindRow = [&]( size_t nFirst, uint32_t nSecond ) -> size_t
	{
		for ( size_t i = 0; i < vec.Count(); ++i )
		{
			if ( vec.template At< size_t >( i ) == nFirst
				&& vec.template At< uint32_t >( i ) == nSecond )
			{
				return i;
			}
		}

		return C::INVALID_INDEX;
	};

	if ( !ref.empty() )
	{
		const size_t iProbe = ref.size() / 2;
		const size_t nProbeFirst = ref[ iProbe ].First();
		const uint32_t nProbeSecond = static_cast< uint32_t >( ref[ iProbe ].Second() );

		const size_t iFindRow = vec.Find( nProbeFirst, nProbeSecond );
		const size_t iRefFindRow = funcFindRow( nProbeFirst, nProbeSecond );

		sOut.AppendMultiple( "BTL::MultiVector_t: Find row: " );

		if ( iFindRow == iRefFindRow )
			sOut.AppendMultiple( "ok (", iFindRow, ")\n" );
		else
			sOut.AppendMultiple( "mismatch (C=", iFindRow, ", Ref=", iRefFindRow, ")\n" );

		size_t iRefRFindRow = C::INVALID_INDEX;

		for ( size_t i = ref.size(); i > 0; --i )
		{
			if ( ref[ i - 1 ].First() == nProbeFirst && ref[ i - 1 ].Second() == static_cast< size_t >( nProbeSecond ) )
			{
				iRefRFindRow = i - 1;
				break;
			}
		}

		const size_t iRFindRow = vec.RFind( nProbeFirst, nProbeSecond );

		sOut.AppendMultiple( "BTL::MultiVector_t: RFind row: " );

		if ( iRFindRow == iRefRFindRow )
			sOut.AppendMultiple( "ok (", iRFindRow, ")\n" );
		else
			sOut.AppendMultiple( "mismatch (C=", iRFindRow, ", Ref=", iRefRFindRow, ")\n" );

		size_t iRefFindFirstCol = C::INVALID_INDEX;
		size_t iRefRFindFirstCol = C::INVALID_INDEX;

		for ( size_t i = 0; i < ref.size(); ++i )
		{
			if ( ref[ i ].First() == nProbeFirst )
			{
				iRefFindFirstCol = i;
				break;
			}
		}

		for ( size_t i = ref.size(); i > 0; --i )
		{
			if ( ref[ i - 1 ].First() == nProbeFirst )
			{
				iRefRFindFirstCol = i - 1;
				break;
			}
		}

		const size_t iFindFirstCol = vec.template FindBy< size_t >( nProbeFirst );
		const size_t iRFindFirstCol = vec.template RFindBy< size_t >( nProbeFirst );

		sOut.AppendMultiple( "BTL::MultiVector_t: FindBy<size_t>: " );

		if ( iFindFirstCol == iRefFindFirstCol && iRFindFirstCol == iRefRFindFirstCol )
			sOut.AppendMultiple( "ok (first=", iFindFirstCol, ", last=", iRFindFirstCol, ")\n" );
		else
			sOut.AppendMultiple( "mismatch (Cfirst=", iFindFirstCol, ", Clast=", iRFindFirstCol, ", Rfirst=", iRefFindFirstCol, ", Rlast=", iRefRFindFirstCol, ")\n" );

		const size_t iFindSecondCol = vec.template FindBy< uint32_t >( nProbeSecond );
		size_t iRefFindSecondCol = C::INVALID_INDEX;

		for ( size_t i = 0; i < ref.size(); ++i )
		{
			if ( static_cast< uint32_t >( ref[ i ].Second() ) == nProbeSecond )
			{
				iRefFindSecondCol = i;
				break;
			}
		}

		sOut.AppendMultiple( "BTL::MultiVector_t: FindBy<uint32_t>: " );

		if ( iFindSecondCol == iRefFindSecondCol )
			sOut.AppendMultiple( "ok (", iFindSecondCol, ")\n" );
		else
			sOut.AppendMultiple( "mismatch (C=", iFindSecondCol, ", Ref=", iRefFindSecondCol, ")\n" );
	}

	const size_t nMissingFirst = ~size_t( 0 );
	const uint32_t nMissingSecond = ~uint32_t( 0 );
	const size_t iMissing = vec.Find( nMissingFirst, nMissingSecond );
	const size_t iMissingByFirst = vec.template FindBy< size_t >( nMissingFirst );
	const size_t iMissingBySecond = vec.template FindBy< uint32_t >( nMissingSecond );

	sOut.AppendMultiple( "BTL::MultiVector_t: " );

	if ( vec.IsValidIndex( iMissing ) || vec.IsValidIndex( iMissingByFirst ) || vec.IsValidIndex( iMissingBySecond ) )
		sOut.AppendMultiple( "Missing element was unexpectedly found (row=", iMissing, ", first=", iMissingByFirst, ", second=", iMissingBySecond, ")\n" );
	else
		sOut.AppendMultiple( "Missing element not found as expected\n" );

	sOut += "---\n";
}
