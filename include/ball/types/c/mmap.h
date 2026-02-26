#ifndef _INCLUDE_BALL_TYPES_C_MMAP_H_
#	define _INCLUDE_BALL_TYPES_C_MMAP_H_

#	include "macros.h"

#	if defined( BALL_APPLE )
#		define BALL_SC_PAGESIZE 29
#	else // !defined( BALL_APPLE )
#		define BALL_SC_PAGESIZE 30
#	endif // defined( BALL_APPLE )

#	define BALL_PROT_READ 0x1
#	define BALL_PROT_WRITE 0x2
#	define BALL_PROT_EXEC 0x4

#	define BALL_MAP_PRIVATE 0x02

#	if defined( BALL_APPLE )
#		define BALL_MAP_ANONYMOUS 0x1000
#	elif defined( BALL_MCST )
#		define BALL_MAP_ANONYMOUS 0x10
#	else // !defined( BALL_APPLE ) && !defined( BALL_MCST )
#		define BALL_MAP_ANONYMOUS 0x20
#	endif // defined( BALL_APPLE ) || defined( __MCST__ )

#	define BALL_MREMAP_MAYMOVE 1

#	define BALL_MAP_FAILED ( ( void * )-1 )

BALL_DLL_IMPORT_C void *mmap( void *pMem, unsigned long long nLength, int nProt, int nFlags, int nFD, long nOffset );
BALL_DLL_IMPORT_C int munmap( void *pMem, unsigned long long nLength );
BALL_DLL_IMPORT_C void *mremap( void *pOldAddress, unsigned long long nOldSize, unsigned long long nNewSize, int nFlags, ... );
BALL_DLL_IMPORT_C long sysconf( int nName );

#endif // !defined( _INCLUDE_BALL_TYPES_C_MMAP_H_ )
