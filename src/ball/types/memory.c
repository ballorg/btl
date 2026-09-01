#include <ball/types/base/arch.h>
#include <ball/types/base/fixed.h>
#include <ball/types/c/assert.h>
#include <ball/types/c/macros.h>
#include <ball/types/c/math.h>
#include <ball/types/memory.h>
#include <ball/types/memoryaligned.h>

#define BALL_DEFAULT_PAGE_SIZE 4096u

#ifndef BALL_WIN
#	include <ball/types/c/mmap.h>
#else
#	define BALL_WINAPI_ALLOCATION_GRANULARITY 65536u // VirtualAlloc always reserves on a 64 KiB boundary.

#	define BALL_WINAPI_MEM_COMMIT 0x00001000u
#	define BALL_WINAPI_MEM_RESERVE 0x00002000u
#	define BALL_WINAPI_MEM_RELEASE 0x00008000u
#	define BALL_WINAPI_PAGE_READWRITE 0x00000004u

BALL_DLL_IMPORT LPVOID BALL_WINAPI VirtualAlloc( LPVOID pAddress, SIZE_T nSize, DWORD nAllocationType, DWORD nProtect );
BALL_DLL_IMPORT BOOL BALL_WINAPI VirtualFree( LPVOID pAddress, SIZE_T nSize, DWORD nFreeType );
#endif

#if defined( BALL_APPLE )
static inline ptr_t Ball_Realloc_MemoryRemap( ptr_t pOldAddress, size_t nOldSize, size_t nNewSize, int nFlags )
{
	if ( ( nFlags & ~BALL_MREMAP_MAYMOVE ) != 0 )
		return BALL_MAP_FAILED;

	if ( pOldAddress == BALL_NULL )
		return BALL_MAP_FAILED;

	if ( nNewSize == 0u )
		return BALL_MAP_FAILED;

	if ( nNewSize == nOldSize )
		return pOldAddress;

	if ( nNewSize < nOldSize )
	{
		const size_t nTailLength = nOldSize - nNewSize;
		uchar_t *pTail = ( uchar_t * )pOldAddress + nNewSize;

		( void )munmap( ( void * )pTail, nTailLength );

		return pOldAddress;
	}

	if ( ( nFlags & BALL_MREMAP_MAYMOVE ) == 0 )
		return BALL_MAP_FAILED;

	{
		const uintptr_t pOldBase = ( uintptr_t )pOldAddress;

		if ( nOldSize <= ( size_t )( ~( uintptr_t )0u - pOldBase ) )
		{
			const size_t nGrowLength = nNewSize - nOldSize;
			ptr_t pTailExpected = ( ptr_t )( pOldBase + nOldSize );
			ptr_t pTailMapped = mmap( pTailExpected, nGrowLength, BALL_PROT_READ | BALL_PROT_WRITE, BALL_MAP_PRIVATE | BALL_MAP_ANONYMOUS, -1, 0 );

			// Fast path on Darwin: if the kernel maps exactly at the tail,
			// the whole region becomes contiguous without moving/copying.
			if ( pTailMapped == pTailExpected )
				return pOldAddress;

			if ( pTailMapped != BALL_MAP_FAILED )
				( void )munmap( pTailMapped, nGrowLength );
		}
	}

	ptr_t pNewAddress = mmap( BALL_NULL, nNewSize, BALL_PROT_READ | BALL_PROT_WRITE, BALL_MAP_PRIVATE | BALL_MAP_ANONYMOUS, -1, 0 );

	if ( pNewAddress == BALL_MAP_FAILED )
		return BALL_MAP_FAILED;

	memcpy( pNewAddress, pOldAddress, nOldSize );

	( void )munmap( pOldAddress, nOldSize );

	return pNewAddress;
}
#elif defined( BALL_UNIX )
static inline ptr_t Ball_Realloc_MemoryRemap( ptr_t pOldAddress, size_t nOldSize, size_t nNewSize, int nFlags )
{
	return mremap( pOldAddress, nOldSize, nNewSize, nFlags );
}
#elif !defined( BALL_WIN )
static inline ptr_t Ball_Realloc_MemoryRemap( ptr_t pOldAddress, size_t nOldSize, size_t nNewSize, int nFlags )
{
	( void )pOldAddress;
	( void )nOldSize;
	( void )nNewSize;
	( void )nFlags;

	return BALL_MAP_FAILED;
}
#endif // defined( BALL_APPLE )

///-----------------------------------------------------------------------------
/// @brief  Allocate page-backed memory without any header.
/// @param  nSize Size requested by the user (bytes).
/// @return Pointer to allocated memory, or BALL_NULL on failure or zero size.
/// @note
///   * Unix: backed by a private anonymous mmap().
///   * WinAPI: reserved and committed in one VirtualAlloc() call.
///   * The mapping is handed out as is: nothing precedes the returned pointer,
///     so the caller must keep @p nSize and pass it to Ball_Free/Ball_Realloc.
///-----------------------------------------------------------------------------
ptr_t Ball_Alloc( size_t nSize )
{
	if ( !nSize )
		return BALL_NULL;

#if defined( BALL_WIN )
	ptr_t pRaw = ( ptr_t )VirtualAlloc( BALL_NULL, nSize, BALL_WINAPI_MEM_RESERVE | BALL_WINAPI_MEM_COMMIT, BALL_WINAPI_PAGE_READWRITE );

	BALL_ASSERT_IF_MESSAGE( pRaw == BALL_NULL, "VirtualAlloc failed" )
		return BALL_NULL;
#else // !defined( BALL_WIN )
	ptr_t pRaw = mmap( BALL_NULL, nSize, BALL_PROT_READ | BALL_PROT_WRITE, BALL_MAP_PRIVATE | BALL_MAP_ANONYMOUS, -1, 0 );

	BALL_ASSERT_IF_MESSAGE( pRaw == BALL_MAP_FAILED, "mmap failed" )
		return BALL_NULL;
#endif // defined( BALL_WIN )

	return pRaw;
}

///-----------------------------------------------------------------------------
/// @brief  Free memory allocated by Ball_Alloc or Ball_Realloc.
/// @param  pMem  Pointer previously returned by Ball_Alloc/Ball_Realloc, or BALL_NULL.
/// @param  nSize Size the block was last allocated/reallocated with.
/// @note   Safe to call with BALL_NULL: no-op.
///-----------------------------------------------------------------------------
void Ball_Free( ptr_t pMem, size_t nSize )
{
	if ( !pMem )
		return;

#if defined( BALL_WIN )
	( void )nSize;
	( void )VirtualFree( pMem, 0u, BALL_WINAPI_MEM_RELEASE );
#else
	( void )munmap( pMem, nSize );
#endif
}

///-----------------------------------------------------------------------------
/// @brief  Reallocate memory allocated by Ball_Alloc or Ball_Realloc.
/// @param  pMem     Old pointer previously returned by Ball_Alloc/Ball_Realloc, or BALL_NULL.
/// @param  nOldSize Size the block was last allocated/reallocated with.
/// @param  nNewSize New size in bytes.
/// @return New pointer (possibly moved), or BALL_NULL on failure or zero size.
/// @note
///   * BALL_NULL pMem behaves like Ball_Alloc; a zero nNewSize behaves like Ball_Free.
///   * Unix: tries mremap( MREMAP_MAYMOVE ) for in-place/move resize first.
///   * Fallback (WinAPI, or mremap failure): allocate a new block, copy
///     min( old, new ) bytes, free the old block.
///-----------------------------------------------------------------------------
ptr_t Ball_Realloc( ptr_t pMem, size_t nOldSize, size_t nNewSize )
{
	if ( !pMem )
		return Ball_Alloc( nNewSize );

	if ( !nNewSize )
	{
		Ball_Free( pMem, nOldSize );

		return BALL_NULL;
	}

#if !defined( BALL_WIN )
	{
		ptr_t pRemapped = Ball_Realloc_MemoryRemap( pMem, nOldSize, nNewSize, BALL_MREMAP_MAYMOVE );

		if ( pRemapped != BALL_MAP_FAILED )
			return pRemapped;
	}
#endif // !defined( BALL_WIN )

	ptr_t pNew = Ball_Alloc( nNewSize );

	BALL_ASSERT_IF_MESSAGE( !pNew, "Failed to allocate new memory during reallocation" )
		return BALL_NULL;

	const size_t nToCopy = ( nOldSize < nNewSize ) ? nOldSize : nNewSize;

	if ( nToCopy )
		memcpy( pNew, pMem, nToCopy );

	Ball_Free( pMem, nOldSize );

	return pNew;
}

///-----------------------------------------------------------------------------
/// @brief  Allocation granularity every Ball_Alloc pointer is already a multiple of.
/// @note   Unix: the system page size. WinAPI: VirtualAlloc reserves on a fixed
///         64 KiB boundary, so no system query is needed.
///-----------------------------------------------------------------------------
static inline size_t Ball_AllocGranularity( void )
{
#if defined( BALL_WIN )
	return BALL_WINAPI_ALLOCATION_GRANULARITY;
#else // !defined( BALL_WIN )
	const long_t nPageSize = sysconf( BALL_SC_PAGESIZE );

	return nPageSize > 0 ? ( size_t )nPageSize : BALL_DEFAULT_PAGE_SIZE;
#endif // defined( BALL_WIN )
}

///-----------------------------------------------------------------------------
/// @brief  Raw block size behind an aligned block, recomputed instead of stored.
/// @note   Up to the allocation granularity the raw block is handed out as is;
///         above it Ball_AllocAlign trims the mapping to a whole number of
///         @p nAlign spans, so both cases stay derivable from ( size, align ).
///-----------------------------------------------------------------------------
static inline size_t Ball_AlignedLength( size_t nSize, size_t nAlign )
{
	return nAlign <= Ball_AllocGranularity() ? nSize : BALL_ROUND_UP( nSize, nAlign );
}

///-----------------------------------------------------------------------------
/// @brief  Allocate memory with explicit alignment, on top of Ball_Alloc.
/// @param  nSize  Size requested by the user (bytes).
/// @param  nAlign Alignment (power of two).
/// @return Aligned pointer or BALL_NULL on failure.
/// @note
///   * No header at all: every Ball_Alloc mapping already starts on an
///     allocation-granularity boundary, which satisfies any @p nAlign up to it.
///   * Above the granularity (Unix only): over-maps by @p nAlign and trims the
///     head and tail spans back through Ball_Free. Both trim bounds are
///     multiples of @p nAlign, hence of the granularity, so the mapping left
///     behind is exactly BALL_ROUND_UP( nSize, nAlign ) bytes long.
///   * WinAPI cannot trim a reservation ( VirtualFree releases it whole ), so
///     an alignment above the granularity is rejected there.
///-----------------------------------------------------------------------------
ptr_t Ball_AllocAlign( size_t nSize, size_t nAlign )
{
	if ( !nSize )
		return BALL_NULL;

	BALL_ASSERT_IF_MESSAGE( !nAlign || !BALL_IS_POW2( nAlign ), "Incorrect align" )
		return BALL_NULL;

	if ( nAlign <= Ball_AllocGranularity() )
		return Ball_Alloc( nSize );

#if defined( BALL_WIN )
	BALL_ASSERT_MESSAGE( 0, "Alignment above the allocation granularity is unsupported" );

	return BALL_NULL;
#else // !defined( BALL_WIN )
	const size_t nLength = BALL_ROUND_UP( nSize, nAlign );

	if ( nLength < nSize || nLength > ( size_t )( ~( size_t )0u ) - nAlign )
		return BALL_NULL;

	ptr_t pRaw = Ball_Alloc( nLength + nAlign );

	if ( !pRaw )
		return BALL_NULL;

	const uintptr_t pBase = ( uintptr_t )pRaw;
	const uintptr_t pUser = BALL_ROUND_UP( pBase, nAlign );
	const size_t nHead = ( size_t )( pUser - pBase );
	const size_t nTail = nAlign - nHead;

	if ( nHead )
		Ball_Free( pRaw, nHead );

	if ( nTail )
		Ball_Free( ( ptr_t )( pUser + nLength ), nTail );

	return ( ptr_t )pUser;
#endif // defined( BALL_WIN )
}

///-----------------------------------------------------------------------------
/// @brief  Free memory allocated by Ball_AllocAlign or Ball_ReallocAlign.
/// @param  pMem   Pointer previously returned by them, or BALL_NULL.
/// @param  nSize  Size the block was last allocated/reallocated with.
/// @param  nAlign Alignment the block was allocated with.
/// @note   Safe to call with BALL_NULL: no-op.
///-----------------------------------------------------------------------------
void Ball_FreeAlign( ptr_t pMem, size_t nSize, size_t nAlign )
{
	if ( !pMem )
		return;

	Ball_Free( pMem, Ball_AlignedLength( nSize, nAlign ) );
}

///-----------------------------------------------------------------------------
/// @brief  Reallocate aligned memory, preserving alignment.
/// @param  pMem     Old pointer (or BALL_NULL).
/// @param  nOldSize Size the block was last allocated/reallocated with.
/// @param  nNewSize New size.
/// @param  nAlign   Required alignment (same constraints as in alloc).
/// @return New pointer (possibly moved) or BALL_NULL on failure.
/// @note
///   * Up to the allocation granularity every resized mapping keeps satisfying
///     the alignment, so the plain Ball_Realloc path (mremap where available)
///     is used directly.
///   * Above it the resized mapping would land on an arbitrary granularity
///     boundary, so a fresh aligned block is taken, min( old, new ) bytes are
///     copied, and the old block is released.
///-----------------------------------------------------------------------------
ptr_t Ball_ReallocAlign( ptr_t pMem, size_t nOldSize, size_t nNewSize, size_t nAlign )
{
	if ( !pMem )
		return Ball_AllocAlign( nNewSize, nAlign );

	if ( !nNewSize )
	{
		Ball_FreeAlign( pMem, nOldSize, nAlign );

		return BALL_NULL;
	}

	BALL_ASSERT_IF_MESSAGE( !nAlign || !BALL_IS_POW2( nAlign ), "Incorrect align" )
		return BALL_NULL;

	if ( nAlign <= Ball_AllocGranularity() )
		return Ball_Realloc( pMem, nOldSize, nNewSize );

	ptr_t pNew = Ball_AllocAlign( nNewSize, nAlign );

	BALL_ASSERT_IF_MESSAGE( !pNew, "Failed to allocate new memory during reallocation" )
		return BALL_NULL;

	const size_t nToCopy = ( nOldSize < nNewSize ) ? nOldSize : nNewSize;

	if ( nToCopy )
		memcpy( pNew, pMem, nToCopy );

	Ball_FreeAlign( pMem, nOldSize, nAlign );

	return pNew;
}
