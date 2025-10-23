#include <ball/types/base/arch.h>
#include <ball/types/base/fixed.h>
#include <ball/types/c/mmap.h>
#include <ball/types/c/memory.h>
#include <ball/types/c/math.h>

#define BALL_MAGIC 0x42414C4C // "BALL" (without null-terminated)

///-----------------------------------------------------------------------------
/// @brief Header placed immediately before the user pointer inside the same VMA.
/// @note  Lives in the same mapping as the user memory (no separate allocation).
///-----------------------------------------------------------------------------
struct Ball_AlignedHeader_t
{
	ptr_t    pRaw;          ///< Mapping base address (after head trim).
	size_t   nSize;         ///< Logical size requested by the user.
	size_t   nMapLength;    ///< Full mapping length to pass to munmap/mremap.
	uint32_t nMagic;        ///< Signature to validate that the pointer is ours.
}; // struct Ball_AlignedHeader_t

///-----------------------------------------------------------------------------
/// @brief Returns system page size (falls back to 4096 on failure).
/// @note  Cached page size is not necessary here; sysconf is cheap enough,
///        and this helper is rarely on a hot path.
///-----------------------------------------------------------------------------
static inline size_t Ball_PageSize( void )
{
	long_t nPageSize = sysconf( BALL_SC_PAGESIZE );

	return ( nPageSize > 0 ) ? ( size_t )nPageSize : 4096u;
}

///-----------------------------------------------------------------------------
/// @brief Convert a user pointer back to our header (with validation).
/// @return Pointer to header or BALL_NULL if invalid.
/// @note   Precondition: @p user must be a pointer returned by Ball_AllocAlign.
///-----------------------------------------------------------------------------
static inline struct Ball_AlignedHeader_t *Ball_HeaderFromUser( ptr_t pUser )
{
	if ( !pUser )
		return BALL_NULL;

	struct Ball_AlignedHeader_t *h = ( ( struct Ball_AlignedHeader_t * )pUser ) - 1;

	if ( h->nMagic != BALL_MAGIC )
		return BALL_NULL;

	return h;
}

///-----------------------------------------------------------------------------
/// @brief  Allocate page-backed memory with explicit alignment via mmap.
/// @param  nSize  Logical size requested by the user (bytes).
/// @param  nAlign Alignment (power of two, >= sizeof( ptr_t )).
/// @return Aligned user pointer or BALL_NULL on failure.
/// @note
///   * Overmaps then trims head/tail with munmap to make the aligned user
///     pointer lie at the beginning of a VMA (useful for mprotect/madvise).
///   * Header is placed immediately before the user pointer (same VMA).
///   * No guard pages are used; caller can add them explicitly if needed.
///-----------------------------------------------------------------------------
ptr_t Ball_AllocAlign( size_t nSize, size_t nAlign )
{
	if ( !nSize )
		return BALL_NULL;

	if ( !BALL_IS_POW2( nAlign ) || nAlign < sizeof( ptr_t ) )
		return BALL_NULL;

	const size_t nPage            = Ball_PageSize();
	const size_t nNeed            = nSize + nAlign + sizeof( struct Ball_AlignedHeader_t );
	const size_t nMapLenInitial   = BALL_ROUND_UP( nNeed, nPage );

	// Initial overmap: we will trim extra head/tail pages later.
	ptr_t pMapInitial = mmap( BALL_NULL, nMapLenInitial,
	                          BALL_PROT_READ | BALL_PROT_WRITE,
	                          BALL_MAP_PRIVATE | BALL_MAP_ANONYMOUS, -1, 0 );

	if ( pMapInitial == BALL_MAP_FAILED )
		return BALL_NULL;

	// Compute aligned user pointer with space for the header right before it.
	const uintptr_t pData       = ( uintptr_t )pMapInitial;
	const uintptr_t pCandUser   = BALL_ROUND_UP( pData + sizeof( struct Ball_AlignedHeader_t ), nAlign );
	const uintptr_t pHdrStart   = pCandUser - sizeof( struct Ball_AlignedHeader_t );

	// Keep [keepStart, keepEnd) and trim everything else.
	const uintptr_t pKeepStart  = BALL_ROUND_DOWN( pHdrStart, nPage );         // ensure header is kept
	const uintptr_t pKeepEnd    = BALL_ROUND_UP( pCandUser + nSize, nPage );   // ensure full user range
	const uintptr_t pMapEnd     = pData + nMapLenInitial;

	// Trim head.
	if ( pKeepStart > pData )
	{
		const size_t nHeadLength = ( size_t )( pKeepStart - pData );

		( void )munmap( ( void * )pData, nHeadLength );
	}

	// Trim tail.
	if ( pKeepEnd < pMapEnd )
	{
		const size_t nTailLength = ( size_t )( pMapEnd - pKeepEnd );

		( void )munmap( ( void * )pKeepEnd, nTailLength );
	}

	ptr_t   pRaw        = ( ptr_t )pKeepStart;
	size_t  nMapLength  = ( size_t )( pKeepEnd - pKeepStart );
	ptr_t   pUser       = ( ptr_t )pCandUser;

	// Initialize header located right before the aligned user pointer.
	struct Ball_AlignedHeader_t *pHeader = ( ( struct Ball_AlignedHeader_t * )pUser ) - 1;

	pHeader->pRaw       = pRaw;
	pHeader->nSize      = nSize;
	pHeader->nMapLength = nMapLength;
	pHeader->nMagic     = BALL_MAGIC;

	return pUser;
}

///-----------------------------------------------------------------------------
/// @brief  Free memory allocated by Ball_AllocAlign.
/// @param  pMem User pointer previously returned by Ball_AllocAlign.
/// @note   Safe to call with invalid/foreign pointer: function will no-op.
///-----------------------------------------------------------------------------
void Ball_FreeAlign( ptr_t pMem )
{
	struct Ball_AlignedHeader_t *pHeader = Ball_HeaderFromUser( pMem );

	if ( !pHeader )
		return;

	pHeader->nMagic = 0; // Poison signature to minimize accidental reuse.
	( void )munmap( pHeader->pRaw, pHeader->nMapLength );
}

///-----------------------------------------------------------------------------
/// @brief  Reallocate aligned memory, preserving alignment.
/// @param  pMem   Old user pointer (or BALL_NULL).
/// @param  nSize  New logical size.
/// @param  nAlign Required alignment (same constraints as in alloc).
/// @return New user pointer (possibly moved) or BALL_NULL on failure.
/// @note
///   * Fast path: try mremap( MREMAP_MAYMOVE ) to resize the *whole* VMA while
///     preserving the user pointer offset (delta) from the VMA base.
///   * Fallback: allocate a new aligned block, memcpy( min( old, new ) ), free old.
///   * On shrink, mremap may return the same base; on grow it may move.
///-----------------------------------------------------------------------------
ptr_t Ball_ReallocAlign( ptr_t pMem, size_t nNewSize, size_t nAlign )
{
	if ( !pMem )
		return Ball_AllocAlign( nNewSize, nAlign );

	if ( !nNewSize )
	{
		Ball_FreeAlign( pMem );
		return BALL_NULL;
	}

	if ( !BALL_IS_POW2( nAlign ) || nAlign < sizeof( ptr_t ) )
		return BALL_NULL;

	struct Ball_AlignedHeader_t *pHeader = Ball_HeaderFromUser( pMem );
	if ( !pHeader )
		return BALL_NULL;

	const uintptr_t pOldBase = ( uintptr_t )pHeader->pRaw;     // Base address of the current mapping (VMA)
	const uintptr_t pUserPtr = ( uintptr_t )pMem;              // User-visible pointer
	const size_t    nOldLen  = pHeader->nMapLength;            // Current mapping size in bytes

	//-----------------------------------------------------------------------------
	// Fast path: the new logical size fully fits into the already allocated region.
	// Check that the range [header_start .. user_ptr + nNewSize) is inside the
	// mapped region [pOldBase .. pOldBase + nOldLen).
	//-----------------------------------------------------------------------------
	const uintptr_t pHeaderStart = pUserPtr - sizeof( struct Ball_AlignedHeader_t );

	// Prevent integer overflow when calculating end address.
	if ( nNewSize <= ( size_t )( ~( uintptr_t )( 0u ) - pUserPtr ) )
	{
		const uintptr_t pNeedEnd = pUserPtr + ( uintptr_t )nNewSize;
		const uintptr_t pMapEnd  = pOldBase + ( uintptr_t )nOldLen;

		if ( pHeaderStart >= pOldBase && pNeedEnd <= pMapEnd )
		{
			// The new data region is still fully inside the existing mapping.
			// Simply update the logical size and return the same pointer.
			pHeader->nSize = nNewSize;
			// Physical map length remains unchanged.
			return ( ptr_t )pUserPtr;
		}
	}

	//-----------------------------------------------------------------------------
	// Attempt to resize the entire mapping in-place using mremap().
	// If expansion fails, mremap() may return BALL_MAP_FAILED.
	//-----------------------------------------------------------------------------
	const size_t    nPage        = Ball_PageSize();
	const uintptr_t pDelta       = pUserPtr - pOldBase; // Offset of the user pointer within the mapping.

	// Compute the new desired mapping range [pKeepStart .. pKeepEnd),
	// aligned to full pages, that covers both header and user data.
	const uintptr_t pKeepStart   = BALL_ROUND_DOWN( pHeaderStart, nPage );
	const uintptr_t pKeepEnd     = BALL_ROUND_UP( pUserPtr + ( uintptr_t )nNewSize, nPage );
	const size_t    nNewLength   = ( size_t )( pKeepEnd - pKeepStart );

	void *pNewBase = mremap( ( void * )pOldBase, nOldLen, nNewLength, BALL_MREMAP_MAYMOVE );

	if ( pNewBase != BALL_MAP_FAILED )
	{
		const uintptr_t pNewBaseU  = ( uintptr_t )pNewBase;
		ptr_t           pNewUser   = ( ptr_t )( pNewBaseU + pDelta );
		struct Ball_AlignedHeader_t *pNewHeader = ( ( struct Ball_AlignedHeader_t * )pNewUser ) - 1;

		pNewHeader->pRaw        = ( ptr_t )pNewBase;
		pNewHeader->nSize       = nNewSize;
		pNewHeader->nMapLength  = nNewLength;
		pNewHeader->nMagic      = BALL_MAGIC;

		return pNewUser;
	}

	//-----------------------------------------------------------------------------
	// Fallback: allocate a new aligned block, copy the data, and free the old one.
	//-----------------------------------------------------------------------------
	ptr_t pNew = Ball_AllocAlign( nNewSize, nAlign );

	if ( !pNew )
		return BALL_NULL;

	const size_t nToCopy = ( pHeader->nSize < nNewSize ) ? pHeader->nSize : nNewSize;

	if ( nToCopy )
		__builtin_memcpy( pNew, pMem, nToCopy );

	Ball_FreeAlign( pMem );

	return pNew;
}

///-----------------------------------------------------------------------------
/// @brief  Return logical size recorded for a user pointer.
/// @note   Alignment/offset parameters are ignored; kept for API symmetry.
///-----------------------------------------------------------------------------
size_t Ball_MemSize( ptr_t pMem, size_t nAlign, size_t nOffset )
{
	( void )nAlign;
	( void )nOffset;

	struct Ball_AlignedHeader_t *pHeader = Ball_HeaderFromUser( pMem );

	return pHeader ? pHeader->nSize : 0u;
}
