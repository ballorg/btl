#ifndef _INCLUDE_BALL_TYPES_C_PREFETCH_H_
#	define _INCLUDE_BALL_TYPES_C_PREFETCH_H_

#	include "macros.h"

#	if defined( BALL_MSVC ) && !defined( BALL_CLANG )
#		if defined( BALL_ARM )
#			if !defined( __INTRIN0_H )
BALL_EXTERN_C void __cdecl __prefetch( const void *pAddress );
#			endif // !defined( __INTRIN0_H )
#			pragma intrinsic( __prefetch )
#			define BALL_PREFETCH_READ( pAddress ) __prefetch( pAddress )
#		elif defined( BALL_X86 )
#			if !defined( __INTRIN0_H )
BALL_EXTERN_C void __cdecl _mm_prefetch( const char *pAddress, int nHint );
#			endif // !defined( __INTRIN0_H )
#			pragma intrinsic( _mm_prefetch )
#			define BALL_PREFETCH_READ( pAddress ) _mm_prefetch( ( const char * )( pAddress ), 1 )
#		else
#			define BALL_PREFETCH_READ( pAddress ) ( ( void )0 )
#		endif
#	elif defined( BALL_GNUC ) || defined( BALL_CLANG )
#		define BALL_PREFETCH_READ( pAddress ) __builtin_prefetch( pAddress, 0, 1 )
#	else
#		define BALL_PREFETCH_READ( pAddress ) ( ( void )0 )
#	endif

#endif // !defined( _INCLUDE_BALL_TYPES_C_PREFETCH_H_ )
