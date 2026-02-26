#ifndef _INCLUDE_BALL_TYPES_BITS_H_
#	define _INCLUDE_BALL_TYPES_BITS_H_

#	include "macros.h"

#	ifndef __INTRIN0_H
#		if defined( BALL_MSVC )
BALL_EXTERN_C unsigned char _BitScanForward( unsigned long *pIndex, unsigned long nMask );
BALL_EXTERN_C unsigned char _BitScanReverse( unsigned long *pIndex, unsigned long nMask );
#			if defined( BALL_64BITS )
BALL_EXTERN_C unsigned char _BitScanForward64( unsigned long *pIndex, unsigned __int64 nMask );
BALL_EXTERN_C unsigned char _BitScanReverse64( unsigned long *pIndex, unsigned __int64 nMask );
#			endif // defined( BALL_64BITS )
#		endif
#	endif

#endif // !defined( _INCLUDE_BALL_TYPES_BITS_H_ )
