#include <ball/types/base/arch.h>
#include <ball/types/base/fixed.h>
#include <ball/types/c/assert.h>
#include <ball/types/c/macros.h>
#include <ball/types/c/memory.h>
#include <ball/types/c/math.h>

#define BALL_MAGIC 0x42414C4C // "BALL" (without null-terminated)
#define BALL_DEFAULT_PAGE_SIZE 4096u

#ifndef BALL_WIN
#	include <ball/types/c/mmap.h>
#else
#	define BALL_WINAPI_REALLOC_COMMIT_CHUNK ( 8u * BALL_DEFAULT_PAGE_SIZE )
#	define BALL_WINAPI_REALLOC_DECOMMIT_THRESHOLD ( 32u * BALL_DEFAULT_PAGE_SIZE )

#	define BALL_WINAPI_MEM_COMMIT 0x00001000u
#	define BALL_WINAPI_MEM_RESERVE 0x00002000u
#	define BALL_WINAPI_MEM_RELEASE 0x00008000u
#	define BALL_WINAPI_PAGE_READWRITE 0x00000004u
#	define BALL_WINAPI_MEM_DECOMMIT 0x00004000u

typedef struct Ball_SystemInfo_t
{
	DWORD nOemId;
	DWORD nPageSize;
	LPVOID pMinApplicationAddress;
	LPVOID pMaxApplicationAddress;
	DWORD_PTR nActiveProcessorMask;
	DWORD nProcessorCount;
	DWORD nProcessorType;
	DWORD nAllocationGranularity;
	WORD nProcessorLevel;
	WORD nProcessorRevision;
} Ball_SystemInfo_t;

BALL_DLL_IMPORT void BALL_WINAPI GetSystemInfo( Ball_SystemInfo_t *pSystemInfo );
BALL_DLL_IMPORT LPVOID BALL_WINAPI VirtualAlloc( LPVOID pAddress, SIZE_T nSize, DWORD nAllocationType, DWORD nProtect );
BALL_DLL_IMPORT BOOL BALL_WINAPI VirtualFree( LPVOID pAddress, SIZE_T nSize, DWORD nFreeType );
#endif

///-----------------------------------------------------------------------------
/// @brief Header placed immediately before the user pointer inside the same VMA.
/// @note  Lives in the same mapping as the user memory (no separate allocation).
///-----------------------------------------------------------------------------
struct Ball_AlignedHeader_t
{
	ptr_t    pRaw;          ///< Mapping base address (after head trim).
	size_t   nSize;         ///< Logical size requested by the user.
	size_t   nLength;       ///< Full platform allocation length for release/resize logic.
	uint32_t nSignature;    ///< Signature to validate that the pointer is ours.
}; // struct Ball_AlignedHeader_t

///-----------------------------------------------------------------------------
/// @brief Process-wide memory subsystem metadata.
/// @note  Stores cached OS page size used by allocation and remap routines.
///-----------------------------------------------------------------------------
struct Ball_MemoryMetadata_t
{
	size_t nPageSize; ///< System page size in bytes.
} g_Memory;

///-----------------------------------------------------------------------------
/// @brief Returns system page size (falls back to 4096 on failure).
///-----------------------------------------------------------------------------
static inline size_t Ball_GetPageSize( void )
{
#if defined( BALL_WIN )
	Ball_SystemInfo_t info;

	GetSystemInfo( &info );

	return ( info.nPageSize > 0u ) ? ( size_t )info.nPageSize : BALL_DEFAULT_PAGE_SIZE;
#else // !defined( BALL_WIN )
	long_t nPageSize = sysconf( BALL_SC_PAGESIZE );

	return ( nPageSize > 0 ) ? ( size_t )nPageSize : BALL_DEFAULT_PAGE_SIZE;
#endif // defined( BALL_WIN )
}

static inline size_t Ball_PageSize_Cached( void )
{
	if ( g_Memory.nPageSize == 0u )
		g_Memory.nPageSize = Ball_GetPageSize();

	return g_Memory.nPageSize;
}

static inline size_t Ball_PageSize( void )
{
	return g_Memory.nPageSize;
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

	if ( h->nSignature != BALL_MAGIC )
		return BALL_NULL;

	return h;
}

///-----------------------------------------------------------------------------
/// @brief Initializes global memory metadata during module load.
/// @note  Must run before any allocator path that depends on page size.
///-----------------------------------------------------------------------------
BALL_DLL_CONSTRUCTOR void Ball_InitMemory()
{
	g_Memory.nPageSize = Ball_GetPageSize();
}

///-----------------------------------------------------------------------------
/// @brief  Allocate page-backed memory with explicit alignment.
/// @param  nSize  Logical size requested by the user (bytes).
/// @param  nAlign Alignment (power of two, >= sizeof( ptr_t )).
/// @return Aligned user pointer or BALL_NULL on failure.
/// @note
///   * Unix: overmaps and trims head/tail pages via munmap.
///   * WinAPI: reserves/commits one region via VirtualAlloc (no partial trim).
///   * Header is placed immediately before the user pointer (same VMA).
///   * No guard pages are used; caller can add them explicitly if needed.
///-----------------------------------------------------------------------------
ptr_t Ball_AllocAlign( size_t nSize, size_t nAlign )
{
	if ( !nSize )
		return BALL_NULL;

	BALL_ASSERT_IF_MESSAGE( !nAlign || !BALL_IS_POW2( nAlign ), "Incorrect align" )
		return BALL_NULL;

	if ( nSize > ( size_t )( ~( size_t )0u ) - nAlign - sizeof( struct Ball_AlignedHeader_t ) )
		return BALL_NULL;

	const size_t nNeed = nSize + nAlign + sizeof( struct Ball_AlignedHeader_t );

#if defined( BALL_WIN )
	const size_t nPage = Ball_PageSize_Cached(), nInitialLength = BALL_ROUND_UP( nNeed, nPage );
	size_t nReserveLength = nPage;

	while ( nReserveLength < nInitialLength && nReserveLength <= ( ( size_t )~0u >> 1 ) )
		nReserveLength <<= 1;

	if ( nReserveLength < nInitialLength )
		nReserveLength = nInitialLength;

	ptr_t pRaw = ( ptr_t )VirtualAlloc( BALL_NULL, nReserveLength, BALL_WINAPI_MEM_RESERVE, BALL_WINAPI_PAGE_READWRITE );

	BALL_ASSERT_IF_MESSAGE( pRaw == BALL_NULL, "VirtualAlloc failed" )
		return BALL_NULL;

	if ( VirtualAlloc( pRaw, nInitialLength, BALL_WINAPI_MEM_COMMIT, BALL_WINAPI_PAGE_READWRITE ) != pRaw )
	{
		( void )VirtualFree( pRaw, 0u, BALL_WINAPI_MEM_RELEASE );

		return BALL_NULL;
	}

	const uintptr_t pData = ( uintptr_t )pRaw;
	const uintptr_t pCandUser = BALL_ROUND_UP( pData + sizeof( struct Ball_AlignedHeader_t ), nAlign );
	ptr_t pUser = ( ptr_t )pCandUser;

	struct Ball_AlignedHeader_t *pHeader = ( ( struct Ball_AlignedHeader_t * )pUser ) - 1;

	pHeader->pRaw = pRaw;
	pHeader->nSize = nSize;
	pHeader->nLength = nReserveLength;
	pHeader->nSignature = BALL_MAGIC;

	return pUser;
#else // !defined( BALL_WIN )
	const size_t nPage = Ball_PageSize(), nInitialLength = BALL_ROUND_UP( nNeed, nPage );

	// Initial overmap: we will trim extra head/tail pages later.
	ptr_t pMapInitial = mmap( BALL_NULL, nInitialLength, BALL_PROT_READ | BALL_PROT_WRITE, BALL_MAP_PRIVATE | BALL_MAP_ANONYMOUS, -1, 0 );

	BALL_ASSERT_IF_MESSAGE( pMapInitial == BALL_MAP_FAILED, "Initial map failed" )
		return BALL_NULL;

	// Compute aligned user pointer with space for the header right before it.
	const uintptr_t pData = ( uintptr_t )pMapInitial;
	const uintptr_t pCandUser = BALL_ROUND_UP( pData + sizeof( struct Ball_AlignedHeader_t ), nAlign );
	const uintptr_t pHdrStart = pCandUser - sizeof( struct Ball_AlignedHeader_t );

	// Keep [keepStart, keepEnd) and trim everything else.
	const uintptr_t pKeepStart = BALL_ROUND_DOWN( pHdrStart, nPage );         // ensure header is kept
	const uintptr_t pKeepEnd = BALL_ROUND_UP( pCandUser + nSize, nPage );   // ensure full user range
	const uintptr_t pMapEnd = pData + nInitialLength;

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

	ptr_t pRaw = ( ptr_t )pKeepStart;
	size_t nLength = ( size_t )( pKeepEnd - pKeepStart );
	ptr_t pUser = ( ptr_t )pCandUser;

	// Initialize header located right before the aligned user pointer.
	struct Ball_AlignedHeader_t *pHeader = ( ( struct Ball_AlignedHeader_t * )pUser ) - 1;

	pHeader->pRaw = pRaw;
	pHeader->nSize = nSize;
	pHeader->nLength = nLength;
	pHeader->nSignature = BALL_MAGIC;

	return pUser;
#endif // defined( BALL_WIN )
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

	pHeader->nSignature = 0; // Poison signature to minimize accidental reuse.
#if defined( BALL_WIN )
	( void )VirtualFree( pHeader->pRaw, 0u, BALL_WINAPI_MEM_RELEASE );
#else
	( void )munmap( pHeader->pRaw, pHeader->nLength );
#endif
}

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

	ptr_t pNewAddress = mmap(
		BALL_NULL,
		nNewSize,
		BALL_PROT_READ | BALL_PROT_WRITE,
		BALL_MAP_PRIVATE | BALL_MAP_ANONYMOUS,
		-1,
		0
	);

	if ( pNewAddress == BALL_MAP_FAILED )
		return BALL_MAP_FAILED;

	{
		uchar_t *pDst = ( uchar_t * )pNewAddress;
		const uchar_t *pSrc = ( const uchar_t * )pOldAddress;

		for ( size_t nIndex = 0u ; nIndex < nOldSize; ++nIndex )
			pDst[ nIndex ] = pSrc[ nIndex ];
	}

	( void )munmap( pOldAddress, nOldSize );

	return pNewAddress;
}
#elif defined( BALL_UNIX )
static inline ptr_t Ball_Realloc_MemoryRemap( ptr_t pOldAddress, size_t nOldSize, size_t nNewSize, int nFlags )
{
	return mremap( pOldAddress, nOldSize, nNewSize, nFlags );
}
#else // !defined( BALL_APPLE ) && !defined( BALL_UNIX )
static inline ptr_t Ball_Realloc_MemoryRemap( ptr_t pOldAddress, size_t nOldSize, size_t nNewSize, int nFlags )
{
	return BALL_MAP_FAILED;
}
#endif // defined( BALL_APPLE )

static ptr_t Ball_ReallocAlign_Internal( ptr_t pMem, size_t nNewSize, size_t nAlign )
{
	struct Ball_AlignedHeader_t *pHeader = Ball_HeaderFromUser( pMem );

	BALL_ASSERT_IF_MESSAGE( !pHeader, "Memory validation failed" )
		return BALL_NULL;

	//-----------------------------------------------------------------------------
	// Fallback: allocate a new aligned block, copy the data, and free the old one.
	//-----------------------------------------------------------------------------
	ptr_t pNew = Ball_AllocAlign( nNewSize, nAlign );

	BALL_ASSERT_IF_MESSAGE( !pNew, "Failed to allocate new memory during reallocation" )
		return BALL_NULL;

	const size_t nToCopy = ( pHeader->nSize < nNewSize ) ? pHeader->nSize : nNewSize;

	if ( nToCopy )
		memcpy( pNew, pMem, nToCopy );

	Ball_FreeAlign( pMem );

	return pNew;
}

///-----------------------------------------------------------------------------
/// @brief  Reallocate aligned memory, preserving alignment.
/// @param  pMem   Old user pointer (or BALL_NULL).
/// @param  nSize  New logical size.
/// @param  nAlign Required alignment (same constraints as in alloc).
/// @return New user pointer (possibly moved) or BALL_NULL on failure.
/// @note
///   * Fast path: if the new logical range fits current mapping, update metadata only.
///   * Unix: additionally tries mremap( MREMAP_MAYMOVE ) for in-place resize.
///   * WinAPI: uses allocate-copy-free fallback when growth exceeds current mapping.
///   * Fallback: allocate a new aligned block, memcpy( min( old, new ) ), free old.
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

	BALL_ASSERT_IF_MESSAGE( !nAlign || !BALL_IS_POW2( nAlign ), "Incorrect align" )
		return BALL_NULL;

	struct Ball_AlignedHeader_t *pHeader = Ball_HeaderFromUser( pMem );

	BALL_ASSERT_IF_MESSAGE( !pHeader, "Memory validation failed" )
		return BALL_NULL;

	const uintptr_t pOldBase = ( uintptr_t )pHeader->pRaw;      // Base address of the current mapping (VMA)
	const uintptr_t pUserPtr = ( uintptr_t )pMem;               // User-visible pointer
	const size_t nOldLenght = pHeader->nLength;                    // Current mapping size in bytes

	//-----------------------------------------------------------------------------
	// Fast path.
	// Unix: new logical range already fits committed mapping.
	// WinAPI: resize in place inside reserved span by commit/decommit.
	//-----------------------------------------------------------------------------
	const uintptr_t pHeaderStart = pUserPtr - sizeof( struct Ball_AlignedHeader_t );

	// Prevent integer overflow when calculating end address.
	if ( nNewSize <= ( size_t )( ~( uintptr_t )( 0u ) - pUserPtr ) )
	{
		const uintptr_t pNeedEnd = pUserPtr + ( uintptr_t )nNewSize;
#if defined( BALL_WIN )
		const size_t nPage = Ball_PageSize_Cached();
		const uintptr_t pOldCommitEnd = BALL_ROUND_UP( pUserPtr + ( uintptr_t )pHeader->nSize, nPage );
		const uintptr_t pNewCommitEnd = BALL_ROUND_UP( pNeedEnd, nPage );
		const uintptr_t pReserveEnd = pOldBase + ( uintptr_t )nOldLenght;

		if ( pHeaderStart >= pOldBase && pNewCommitEnd <= pReserveEnd )
		{
			if ( pNewCommitEnd > pOldCommitEnd )
			{
				const size_t nNeedCommitLength = ( size_t )( pNewCommitEnd - pOldCommitEnd );
				const size_t nMaxCommitLength = ( size_t )( pReserveEnd - pOldCommitEnd );
				size_t nCommitSpan = nNeedCommitLength;

				if ( nCommitSpan < BALL_WINAPI_REALLOC_COMMIT_CHUNK && BALL_WINAPI_REALLOC_COMMIT_CHUNK <= nMaxCommitLength )
					nCommitSpan = BALL_WINAPI_REALLOC_COMMIT_CHUNK;

				nCommitSpan = BALL_ROUND_UP( nCommitSpan, nPage );

				if ( nCommitSpan > nMaxCommitLength )
					nCommitSpan = nMaxCommitLength;

				void *pCommit = VirtualAlloc( ( void * )pOldCommitEnd, nCommitSpan, BALL_WINAPI_MEM_COMMIT, BALL_WINAPI_PAGE_READWRITE );

				if ( pCommit != ( void * )pOldCommitEnd )
					return Ball_ReallocAlign_Internal( pMem, nNewSize, nAlign );
			}
			else if ( pNewCommitEnd < pOldCommitEnd )
			{
				const size_t nDecommitLength = ( size_t )( pOldCommitEnd - pNewCommitEnd );

				if ( nDecommitLength >= BALL_WINAPI_REALLOC_DECOMMIT_THRESHOLD )
					( void )VirtualFree( ( void * )pNewCommitEnd, nDecommitLength, BALL_WINAPI_MEM_DECOMMIT );
			}

			pHeader->nSize = nNewSize;

			return ( ptr_t )pUserPtr;
		}
#else
		const uintptr_t pMapEnd = pOldBase + ( uintptr_t )nOldLenght;

		if ( pHeaderStart >= pOldBase && pNeedEnd <= pMapEnd )
		{
			pHeader->nSize = nNewSize;

			return ( ptr_t )pUserPtr;
		}
#endif
	}

	//-----------------------------------------------------------------------------
	// Attempt to resize the entire mapping in-place on Unix using mremap().
	// Windows falls through to allocate-copy-free path below.
	//-----------------------------------------------------------------------------
#if !defined( BALL_WIN )
	{
		if ( nNewSize <= ( size_t )( ~( uintptr_t )( 0u ) - pUserPtr ) )
		{
			const size_t nPage = Ball_PageSize();
			const uintptr_t pDelta = pUserPtr - pOldBase; // Offset of the user pointer within the mapping.

			// Compute the new desired mapping range [pKeepStart .. pKeepEnd),
			// aligned to full pages, that covers both header and user data.
			const uintptr_t pKeepStart = BALL_ROUND_DOWN( pHeaderStart, nPage ), pKeepEnd = BALL_ROUND_UP( pUserPtr + ( uintptr_t )nNewSize, nPage );
			const size_t nNewLength = ( size_t )( pKeepEnd - pKeepStart );

			ptr_t pNewBase = Ball_Realloc_MemoryRemap( ( ptr_t )pOldBase, nOldLenght, nNewLength, BALL_MREMAP_MAYMOVE );

			if ( pNewBase != BALL_MAP_FAILED )
			{
				const uintptr_t pNewBaseU  = ( uintptr_t )pNewBase;
				ptr_t pNewUser = ( ptr_t )( pNewBaseU + pDelta );
				struct Ball_AlignedHeader_t *pNewHeader = ( ( struct Ball_AlignedHeader_t * )pNewUser ) - 1;

				pNewHeader->pRaw = pNewBase;
				pNewHeader->nSize = nNewSize;
				pNewHeader->nLength = nNewLength;
				pNewHeader->nSignature = BALL_MAGIC;

				return pNewUser;
			}
		}
	}
#endif

	return Ball_ReallocAlign_Internal( pMem, nNewSize, nAlign );
}

///-----------------------------------------------------------------------------
/// @brief  Return logical size recorded for a user pointer.
/// @note   Alignment/offset parameters are ignored; kept for API symmetry.
///-----------------------------------------------------------------------------
size_t Ball_SizeAlign( ptr_t pMem, size_t nAlign, size_t nOffset )
{
	( void )nAlign;
	( void )nOffset;

	struct Ball_AlignedHeader_t *pHeader = Ball_HeaderFromUser( pMem );

	return pHeader ? pHeader->nSize : 0u;
}
