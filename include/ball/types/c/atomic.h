#ifndef _INCLUDE_BALL_TYPES_C_ATOMIC_H_
#	define _INCLUDE_BALL_TYPES_C_ATOMIC_H_

#	include "macros.h"

///-----------------------------------------------------------------------------
/// @brief Compiler intrinsics the atomic layer is built on.
///
/// @details GCC and Clang expose the complete `__atomic_*` family as builtins, so
/// nothing has to be declared for them. MSVC has no such family: its atomics are
/// the `_Interlocked*` intrinsics, declared here the way Ball declares every other
/// intrinsic it uses (see `c/bits.h`), so `<intrin.h>` is never included.
///-----------------------------------------------------------------------------
#	if defined( BALL_MSVC ) && !defined( BALL_CLANG )
#		if !defined( __INTRIN0_H )
BALL_EXTERN_C void _ReadWriteBarrier( void );

BALL_EXTERN_C char _InterlockedExchange8( char volatile *pTarget, char nValue );
BALL_EXTERN_C short _InterlockedExchange16( short volatile *pTarget, short nValue );
BALL_EXTERN_C long _InterlockedExchange( long volatile *pTarget, long nValue );
BALL_EXTERN_C __int64 _InterlockedExchange64( __int64 volatile *pTarget, __int64 nValue );

BALL_EXTERN_C char _InterlockedCompareExchange8( char volatile *pTarget, char nExchange, char nComparand );
BALL_EXTERN_C short _InterlockedCompareExchange16( short volatile *pTarget, short nExchange, short nComparand );
BALL_EXTERN_C long _InterlockedCompareExchange( long volatile *pTarget, long nExchange, long nComparand );
BALL_EXTERN_C __int64 _InterlockedCompareExchange64( __int64 volatile *pTarget, __int64 nExchange, __int64 nComparand );

BALL_EXTERN_C char _InterlockedExchangeAdd8( char volatile *pTarget, char nValue );
BALL_EXTERN_C short _InterlockedExchangeAdd16( short volatile *pTarget, short nValue );
BALL_EXTERN_C long _InterlockedExchangeAdd( long volatile *pTarget, long nValue );
BALL_EXTERN_C __int64 _InterlockedExchangeAdd64( __int64 volatile *pTarget, __int64 nValue );

BALL_EXTERN_C char _InterlockedAnd8( char volatile *pTarget, char nValue );
BALL_EXTERN_C short _InterlockedAnd16( short volatile *pTarget, short nValue );
BALL_EXTERN_C long _InterlockedAnd( long volatile *pTarget, long nValue );
BALL_EXTERN_C __int64 _InterlockedAnd64( __int64 volatile *pTarget, __int64 nValue );

BALL_EXTERN_C char _InterlockedOr8( char volatile *pTarget, char nValue );
BALL_EXTERN_C short _InterlockedOr16( short volatile *pTarget, short nValue );
BALL_EXTERN_C long _InterlockedOr( long volatile *pTarget, long nValue );
BALL_EXTERN_C __int64 _InterlockedOr64( __int64 volatile *pTarget, __int64 nValue );

BALL_EXTERN_C char _InterlockedXor8( char volatile *pTarget, char nValue );
BALL_EXTERN_C short _InterlockedXor16( short volatile *pTarget, short nValue );
BALL_EXTERN_C long _InterlockedXor( long volatile *pTarget, long nValue );
BALL_EXTERN_C __int64 _InterlockedXor64( __int64 volatile *pTarget, __int64 nValue );

#			if defined( BALL_ARM )
BALL_EXTERN_C void __dmb( unsigned int nType );
#			endif // defined( BALL_ARM )
#		endif // !defined( __INTRIN0_H )

#		pragma intrinsic( _ReadWriteBarrier )
#		pragma intrinsic( _InterlockedExchange8, _InterlockedExchange16, _InterlockedExchange, _InterlockedExchange64 )
#		pragma intrinsic( _InterlockedCompareExchange8, _InterlockedCompareExchange16, _InterlockedCompareExchange, _InterlockedCompareExchange64 )
#		pragma intrinsic( _InterlockedExchangeAdd8, _InterlockedExchangeAdd16, _InterlockedExchangeAdd, _InterlockedExchangeAdd64 )
#		pragma intrinsic( _InterlockedAnd8, _InterlockedAnd16, _InterlockedAnd, _InterlockedAnd64 )
#		pragma intrinsic( _InterlockedOr8, _InterlockedOr16, _InterlockedOr, _InterlockedOr64 )
#		pragma intrinsic( _InterlockedXor8, _InterlockedXor16, _InterlockedXor, _InterlockedXor64 )

#		if defined( BALL_ARM )
#			pragma intrinsic( __dmb )
			// DMB ISH: inner-shareable full barrier, the domain a process's threads share.
#			define BALL_ATOMIC_FENCE() __dmb( 0xB )
#		else // !defined( BALL_ARM )
			// x86-TSO reorders only store-load, which a locked RMW already fences; a plain
			// load or store therefore needs the compiler kept in line and nothing more.
#			define BALL_ATOMIC_FENCE() _ReadWriteBarrier()
#		endif // defined( BALL_ARM )
#	endif // defined( BALL_MSVC ) && !defined( BALL_CLANG )

#endif // !defined( _INCLUDE_BALL_TYPES_C_ATOMIC_H_ )
