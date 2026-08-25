#ifndef _INCLUDE_BALL_TYPES_C_PREFETCH_H_
#	define _INCLUDE_BALL_TYPES_C_PREFETCH_H_

#	include "macros.h"

#	if defined( BALL_MSVC ) && !defined( BALL_CLANG )
#		if defined( BALL_ARM )
#			if !defined( __INTRIN0_H )
BALL_EXTERN_C void __cdecl __prefetch( const void *pAddress );
#				if defined( BALL_ARM64 )
BALL_EXTERN_C void __cdecl __prefetch2( const void *pAddress, unsigned char nOperation );
#				endif // defined( BALL_ARM64 )
#			endif // !defined( __INTRIN0_H )
#			pragma intrinsic( __prefetch )
#			define BALL_PREFETCH_READ( pAddress ) __prefetch( pAddress )
#			if defined( BALL_ARM64 )
#				pragma intrinsic( __prefetch2 )
#				// PRFM operation PSTL1KEEP: store into L1 with temporal retention.
#				define BALL_PREFETCH_WRITE( pAddress ) __prefetch2( pAddress, 0x10 )
#			else
#				define BALL_PREFETCH_WRITE( pAddress ) ( ( void )0 )
#			endif // defined( BALL_ARM64 )
#		elif defined( BALL_X86 )
#			if !defined( __INTRIN0_H )
BALL_EXTERN_C void __cdecl _mm_prefetch( const char *pAddress, int nHint );
BALL_EXTERN_C void __cdecl _m_prefetchw( const volatile void *pAddress );
#			endif // !defined( __INTRIN0_H )
#			pragma intrinsic( _mm_prefetch )
#			pragma intrinsic( _m_prefetchw )
#			define BALL_PREFETCH_READ( pAddress ) _mm_prefetch( ( const char * )( pAddress ), 1 )
#			define BALL_PREFETCH_WRITE( pAddress ) _m_prefetchw( ( const volatile void * )( pAddress ) )
#		else
#			define BALL_PREFETCH_READ( pAddress ) ( ( void )0 )
#			define BALL_PREFETCH_WRITE( pAddress ) ( ( void )0 )
#		endif
#	elif defined( BALL_GNUC ) || defined( BALL_CLANG )
#		define BALL_PREFETCH_READ( pAddress ) __builtin_prefetch( pAddress, 0, 1 )
#		define BALL_PREFETCH_WRITE( pAddress ) __builtin_prefetch( pAddress, 1, 1 )
#	else
#		define BALL_PREFETCH_READ( pAddress ) ( ( void )0 )
#		define BALL_PREFETCH_WRITE( pAddress ) ( ( void )0 )
#	endif

#endif // !defined( _INCLUDE_BALL_TYPES_C_PREFETCH_H_ )
