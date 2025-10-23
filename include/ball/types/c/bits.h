#ifndef _INCLUDE_BALL_TYPES_BITS_H_
#	define _INCLUDE_BALL_TYPES_BITS_H_

#	include "macros.h"

#	ifndef __INTRIN0_H
#		if defined(_MSC_VER)
BALL_EXTERN_C unsigned char _BitScanForward( unsigned long *pIndex, unsigned long nMask );
BALL_EXTERN_C unsigned char _BitScanReverse( unsigned long *pIndex, unsigned long nMask );
#			if defined(__x86_64__) || defined(__arm__) || defined(__aarch64__)
BALL_EXTERN_C unsigned char _BitScanForward64( unsigned long *pIndex, unsigned __int64 nMask );
BALL_EXTERN_C unsigned char _BitScanReverse64( unsigned long *pIndex, unsigned __int64 nMask );
#			endif
#		endif
#	endif

#endif // !defined( _INCLUDE_BALL_TYPES_BITS_H_ )
