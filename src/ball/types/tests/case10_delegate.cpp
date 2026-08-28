module;


module Ball.Types;

import Ball.New;
import :Core;
import :Delegate;
import :String;
import :StringView;
import :Tests.Case10;

using TestsOutput_t = BTL::BufferString_t< 4096 >;

static constexpr BTL::StringView_t s_svDelegateName = "BTL::Delegate_t", s_svMulticastDelegateName = "BTL::MulticastDelegate_t";

static void LogDelegateCheck( TestsOutput_t &sOut, BTL::StringView_t svContainerName, BTL::StringView_t svLabel, bool bOk )
{
	sOut.AppendMultiple( svContainerName, ": ", svLabel, ": " );

	if ( bOk )
		sOut += "ok\n";
	else
		sOut += "mismatch\n";
}

struct DelegateTestObject_t
{
	int m_nBase;
	int m_nTotal;

	int Add( int nValue, int nPayload ) { return m_nBase + nValue + nPayload; }
	int AddConst( int nValue, int nPayload ) const { return m_nBase + nValue + nPayload + 100; }
	void AccumulateScaled( int nValue, int nPayload ) { m_nTotal += nValue * 2 + nPayload; }
	void AccumulateMember( int nValue, int nPayload ) { m_nTotal += nValue + nPayload; }
};

static int DelegateStaticAdd( int nValue, int nPayload )
{
	return 1000 + nValue + nPayload;
}

static void DelegateStaticAccumulate( int nValue, int *pAccumulator, int nPayload )
{
	if ( pAccumulator != nullptr )
		*pAccumulator += nValue + nPayload;
}

void Case10_Delegate( TestsOutput_t &sOut )
{
	using Delegate_t = BTL::Delegate_t< int ( int ) >;
	using MulticastDelegate_t = BTL::MulticastDelegate_t< void ( int ) >;

	{
		Delegate_t dgStatic( &DelegateStaticAdd, 5 );

		bool bOk = dgStatic.IsBound() && dgStatic.Execute( 7 ) == 1012 && dgStatic.ExecuteIfBound( 1 ) == 1006;

		Delegate_t dgCopy( dgStatic );
		Delegate_t dgMove( BTL::Move( dgCopy ) );
		Delegate_t dgAssign;
		Delegate_t dgMoveAssign;

		dgAssign = dgStatic;
		dgMoveAssign = BTL::Move( dgAssign );

		bOk = bOk && dgMove.Execute( 2 ) == 1007 && dgMoveAssign.Execute( 4 ) == 1009;
		LogDelegateCheck( sOut, s_svDelegateName, "Static delegate", bOk );
	}

	{
		DelegateTestObject_t object{ 10, 0 };

		Delegate_t dgMember( &object, &DelegateTestObject_t::Add, 4 );
		LogDelegateCheck( sOut, s_svDelegateName, "Member delegate", dgMember.IsBound() && dgMember.GetOwner() == &object && dgMember.Execute( 7 ) == 21 );
	}

	{
		DelegateTestObject_t object{ 10, 0 };

		Delegate_t dgMemberConst( &object, &DelegateTestObject_t::AddConst, 4 );

		bool bOk = dgMemberConst.IsBound() && dgMemberConst.GetOwner() == &object && dgMemberConst.Execute( 7 ) == 121;

		LogDelegateCheck( sOut, s_svDelegateName, "Member const delegate", bOk );
	}

	{
		int nCapture = 2;

		Delegate_t dgLambda( [ &nCapture ]( int nValue, int nPayload ) { return nCapture + nValue + nPayload; }, 5 );

		bool bOk = dgLambda.IsBound() && dgLambda.Execute( 7 ) == 14;

		nCapture = 4;

		bOk = bOk && dgLambda.Execute( 1 ) == 10;
		LogDelegateCheck( sOut, s_svDelegateName, "Lambda delegate", bOk );
	}

	{
		DelegateTestObject_t memberObjectA{ 0, 0 };
		DelegateTestObject_t memberObjectB{ 0, 0 };

		int nStaticTotal = 0;
		int nLambdaTotal = 0;

		MulticastDelegate_t dgMulti;

		BTL::DelegateHandle_t hStatic = dgMulti.AddStatic( &DelegateStaticAccumulate, &nStaticTotal, 10 );
		BTL::DelegateHandle_t hMemberA = dgMulti.AddMember( &memberObjectA, &DelegateTestObject_t::AccumulateScaled, 20 );
		BTL::DelegateHandle_t hMemberB = dgMulti.AddMember( &memberObjectB, &DelegateTestObject_t::AccumulateMember, 30 );
		BTL::DelegateHandle_t hLambda = dgMulti.AddLambda( [ &nLambdaTotal ]( int nValue, int nPayload ) { nLambdaTotal += nValue + nPayload; }, 40 );

		dgMulti.Broadcast( 5 );

		bool bOk = dgMulti.GetSize() == 4 && hStatic.IsValid() && hMemberA.IsValid() && hMemberB.IsValid() && hLambda.IsValid() && nStaticTotal == 15 && memberObjectA.m_nTotal == 30 && memberObjectB.m_nTotal == 35 && nLambdaTotal == 45;

		bOk = bOk && dgMulti.Remove( hMemberA ) && !hMemberA.IsValid();
		dgMulti.Broadcast( 5 );
		bOk = bOk && nStaticTotal == 30 && memberObjectA.m_nTotal == 30 && memberObjectB.m_nTotal == 70 && nLambdaTotal == 90;

		dgMulti.RemoveObject( &memberObjectB );
		dgMulti.Broadcast( 5 );
		bOk = bOk && nStaticTotal == 45 && memberObjectA.m_nTotal == 30 && memberObjectB.m_nTotal == 70 && nLambdaTotal == 135 && dgMulti.GetSize() == 2;

		dgMulti.RemoveAll();
		bOk = bOk && dgMulti.GetSize() == 0;

		LogDelegateCheck( sOut, s_svMulticastDelegateName, "Multicast delegate", bOk );
	}

	sOut += "---\n";
}
