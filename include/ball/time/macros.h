#ifndef _INCLUDE_BALL_TIME_MACROS_H_
#	define _INCLUDE_BALL_TIME_MACROS_H_

// Simple profiling helpers:
//   - BALL_PROF_BEGIN( tag ): capture start CPU time of current thread
//   - BALL_PROF_END( tag ):   create CTimeNS with elapsed CPU time since BEGIN
// Usage (supply a unique tag per measurement):
//   BALL_PROF_BEGIN( Case01 );
//   ... work ...
//   auto elapsed = BALL_PROF_END( Case01 ); // CTimeNS holding elapsed nanoseconds
#	define BALL_PROF_CAT_IMPL( x, y ) x##y
#	define BALL_PROF_CAT( x, y ) BALL_PROF_CAT_IMPL( x, y )
#	define BALL_PROF_BEGIN( tag ) const BTL::timens_t BALL_PROF_CAT( __ballProfBeginNS_, tag ) = BTL::GetTimeNS()
#	define BALL_PROF_END( tag ) BTL::CTimeNS( BTL::GetTimeNS() - BALL_PROF_CAT( __ballProfBeginNS_, tag ) )

#endif // !defined( _INCLUDE_BALL_TIME_MACROS_H_ )
