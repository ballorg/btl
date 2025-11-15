#ifndef _INCLUDE_BALL_TYPES_VECTOR_HPP_
#	define _INCLUDE_BALL_TYPES_VECTOR_HPP_

#	pragma once

#	include "base/arch.h"
#	include "c/assert.h"
#	include "c/memory.h"
#	include "c/memoryaligned.h"
#	include "meta/number.hpp"
#	include "allocator.hpp"
#	include "memoryview.hpp"
#	include "bits.hpp"
#	include "math.hpp"
#	include "xvalue.hpp"

// ===============================
// CVectorBase (now derives from CMemoryView)
// ===============================
template < class B, typename I, typename T, class A = CAllocator< I, T > >
class CVectorBase : public B
{
public:
	using Base_t      = B;
	using Index_t     = I;
	using Element_t   = T;
	using Allocator_t = A;
	using Number_t    = MNumber< Index_t >;
	using Unsigned_t  = typename Number_t::U;
	using View_t      = Base_t::View_t;
	using ConstView_t = Base_t::ConstView_t;

	using Base_t::Base_t;
	using Base_t::FIXED_COUNT;
	using Base_t::Count;
	using Base_t::IsOverflow;
	using Base_t::FixedData;
	using Base_t::Data;
	using Base_t::Base;

	static constexpr bool IS_GROWABLE = FIXED_COUNT > 0;
	static constexpr size_t ALIGNED_SIZE = NextPowerOfTwo_Const( 8 * sizeof( Element_t ) );
	static constexpr I INVALID_INDEX = Number_t::INVALID;

	/// @brief Default / external ctor. Does not assume ownership semantics beyond this instance.
	constexpr ~CVectorBase() noexcept
	{
		if ( IsOverflow() )
		{
			Allocator_t::Free( Data() );
		}

		Base_t::Set( 0, nullptr );
	}

	// ---------------------------
	// Basic queries / accessors.
	// ---------------------------

	/// @warning This returns next power-of-two of Count(), not a tracked internal capacity.
	constexpr I Capacity() const noexcept { return NextPowerOfTwo< I, FIXED_COUNT >( Count() ); }
	constexpr size_t CapacitySize() const noexcept { return static_cast< size_t >( Capacity() ) * sizeof( Element_t ); }

	constexpr I FixedCount() const noexcept { return FIXED_COUNT; }
	constexpr size_t FixedSize() const noexcept { return FixedCount() * sizeof( Element_t ); }

	constexpr I FixedCapacity() const noexcept { return NextPowerOfTwo_Unified( FixedCount() ); } 
	constexpr size_t FixedCapacitySize() const noexcept { return FixedCapacity() * sizeof( Element_t ); }

protected:
	///-----------------------------------------------------------------------------
	/// @brief Ensures that heap storage can accommodate at least @p nRequestCapacity
	///        elements, rounding up to the next power of two.
	///
	/// Overview:
	///   - Rounds @p nRequestCapacity up to the next power of two via NextPowerOfTwo().
	///   - Expands capacity only; never shrinks unless migrating back to fixed storage.
	///   - Uses Allocator_t::Realloc() for existing heap memory, otherwise Allocator_t::Alloc().
	///
	/// Behavior details:
	///   - If @p nRequestCapacity exceeds the fixed inline buffer (N), capacity grows to the
	///     next power of two >= nRequestCapacity.
	///   - If already on heap and new capacity equals the current derived capacity, this is a no-op.
	///   - If still using fixed storage but overflow is detected, a new heap block is allocated,
	///     and data is migrated via MoveToHeap() if IS_GROWABLE is enabled.
	///   - If the new request fits back into the inline buffer while currently on heap,
	///     the data is migrated back via MoveToFixed() (only if IS_GROWABLE).
	///
	/// Invariants and assumptions:
	///   - Capacity() is derived from Count() (rounded to power of two), not stored explicitly.
	///   - NUM_ALIGNED / ALIGNED_SIZE act as allocator hints for alignment and bucket size.
	///   - Overflow in NextPowerOfTwo() yields Number_t::INVALID — this is asserted defensively.
	///   - The function does *not* construct, move, or destroy elements — it only manages memory.
	///   - Callers are responsible for element placement and internal state consistency.
	///   - Not thread-safe; external synchronization is required for concurrent access.
	///
	/// Complexity:
	///   - O(1) when no reallocation or migration occurs.
	///   - O(1) expected for allocator operations; O(n) for migration (MoveToHeap/MoveToFixed).
	///
	/// Safety and diagnostics:
	///   - Defensive assertions guard against capacity overflow and null allocator results.
	///   - On allocation failure, nullptr may be returned; the caller must validate the pointer.
	///   - Function never throws exceptions; caller must enforce safety policy.
	///
	/// @param nRequestCapacity  Minimum required number of elements before rounding.
	/// @return Pointer to valid element storage (possibly reallocated or migrated).
	///-----------------------------------------------------------------------------
	constexpr T *EnsureCapacity( I nRequestCapacity )
	{
		T *pElements = Base();

		// Case A: request exceeds inline storage
		if ( IsOverflow( nRequestCapacity ) )
		{
			const I nCapacity = Capacity();
			const I nNewCapacity = NextPowerOfTwo< I, FIXED_COUNT >( nRequestCapacity );

			BALL_ASSERT_MESSAGE( nNewCapacity != Number_t::INVALID, "Capacity overflow!" );

			if ( nNewCapacity == Number_t::INVALID || nNewCapacity < nCapacity )
				return pElements;

			// Subcase A1: already on heap
			if ( IsOverflow() )
			{
				if ( pElements && nNewCapacity == nCapacity )
					return pElements;

				pElements = Allocator_t::Realloc( pElements, nNewCapacity, ALIGNED_SIZE );
				BALL_ASSERT_MESSAGE( pElements != nullptr, "Failed to reallocate elements" );
			}
			else // Subcase A2: currently inline — migrate to heap
			{
				pElements = Allocator_t::Alloc( nNewCapacity, ALIGNED_SIZE );
				BALL_ASSERT_MESSAGE( pElements != nullptr, "Failed to allocate elements" );

				if constexpr ( IS_GROWABLE )
					MoveToHeap( pElements );
			}
		}
		// Case B: fits into inline storage, but currently on heap — migrate back
		else if ( IsOverflow() )
		{
			if constexpr ( IS_GROWABLE )
				MoveToFixed( pElements );
		}

		return pElements;
	}

	constexpr void MoveToFixed( const T *pElements )
	{
		T *pFixedElements = FixedData();

		BALL_ASSERT( pFixedElements != nullptr );
		BALL_ASSERT( pElements != nullptr );
		CopyElements( FixedCount(), pFixedElements, pElements );
	}

	constexpr void MoveToHeap( T *pElements )
	{
		const T *pFixedElements = FixedData();

		BALL_ASSERT( pElements != nullptr );
		BALL_ASSERT( pFixedElements != nullptr );
		CopyElements( FixedCount(), pElements, pFixedElements );
	}

	/// @brief Copy contents from another CVectorBase.
	constexpr CVectorBase &CopyFrom( const ConstView_t &other )
	{
		const I nNewCount = other.Count();

		T *pElements = EnsureCapacity( nNewCount );

		CopyElements( other.Count(), pElements, other.Base() );
		Base_t::Set( nNewCount, pElements );

		return *this;
	}

	template< I LN > constexpr CVectorBase &CopyFrom( const CMemoryView< I, T, LN > &other )
	{
		const I nNewCount = other.Count();

		T *pElements = EnsureCapacity( nNewCount );

		CopyElements( other.Count(), pElements, other.Base() );
		Base_t::Set( nNewCount, pElements );

		return *this;
	}

	using Base_t::MoveFrom;
};

template < class B, typename I, typename T >
class CVectorImpl : public B
{
public:
	using Base_t =      B;
	using Index_t =     typename Base_t::Index_t;
	using Unsigned_t =  typename Base_t::Unsigned_t;
	using View_t =      Base_t::View_t;
	using ConstView_t = Base_t::ConstView_t;

	using Base_t::Base_t;
	using Base_t::INVALID_INDEX;
	using Base_t::Count;
	using Base_t::Base;
	using Base_t::Find;

	constexpr CVectorImpl( const View_t &copyFrom ) noexcept { Base_t::CopyFrom( copyFrom ); }
	constexpr CVectorImpl( const ConstView_t &copyFrom ) noexcept { Base_t::CopyFrom( copyFrom ); }
	constexpr CVectorImpl( View_t &&moveFrom ) noexcept { Base_t::MoveFrom( Move( moveFrom ) ); }
	constexpr ~CVectorImpl() noexcept { Purge(); }

	CVectorImpl &operator=( const View_t &copyFrom ) { return Base_t::CopyFrom( copyFrom ); }
	CVectorImpl &operator=( const ConstView_t &copyFrom ) { return Base_t::CopyFrom( copyFrom ); }
	CVectorImpl &operator=( CVectorImpl &&moveFrom ) { return Base_t::MoveFrom( Move( moveFrom ) ); }

	// Returns new count.
	constexpr I Grow( const I n = 1 )
	{
		I nNewCount = Count() + n;

		SetCount( nNewCount );

		return nNewCount;
	}

	///-----------------------------------------------------------------------------
	/// @brief Inserts nCount elements from a C-array at position @p index (copy).
	/// @return The index where the first element was inserted.
	///-----------------------------------------------------------------------------
	constexpr I Insert( const I nIndex, ConstView_t v )
	{
		BALL_ASSERT( !v.Empty() );

		const T *pViewData = v.Base();
		I nViewCount = v.Count();

		T *pData = EnsureInsert( nIndex, nViewCount );

		// Construct each new element
		for ( I n = 0; n < nViewCount; ++n )
		{
			ConstructElement( &pData[ n ], pViewData[ n ] );
		}

		return nIndex + nViewCount;
	}
	constexpr I Insert( View_t v ) { return Insert( v.Const() ); }

	///-----------------------------------------------------------------------------
	/// @brief Inserts R elements from a C-array at position @p index (copy).
	/// @return The index where the first element was inserted.
	///-----------------------------------------------------------------------------
	template < I N >
	constexpr I Insert( const I nIndex, const I nCount, const T ( &arrElements )[ N ] )
	{
		static_assert( 0 < N, "Insert( I, const I, const & ) requires at least one element" );
		BALL_ASSERT( 0 < nCount );
		BALL_ASSERT( nCount <= N );

		T *pData = EnsureInsert( nIndex, nCount );

		// Construct each new element
		for ( I n = 0; n < nCount; ++n )
		{
			ConstructElement( &pData[ n ], arrElements[ n ] );
		}

		return nIndex + nCount;
	}

	///-----------------------------------------------------------------------------
	/// @brief Inserts R elements from an rvalue C-array at position @p index (move).
	/// @return The index where the first element was inserted.
	///-----------------------------------------------------------------------------
	template < I N >
	constexpr I Insert( const I nIndex, const I nCount, T ( &&arrElements )[ N ] )
	{
		static_assert( 0 < N, "Insert( I, const I, && ) requires at least one element" );
		BALL_ASSERT( 0 < nCount );
		BALL_ASSERT( nCount <= N );

		T *pData = EnsureInsert( nIndex, nCount );

		// Construct each new element
		for ( I n = 0; n < nCount; ++n )
		{
			ConstructElement( &pData[ n ], Move( arrElements[ n ] ) );
		}

		return nIndex + nCount;
	}

	///-----------------------------------------------------------------------------
	/// @brief Insert a single element at position @p nIndex (copy).
	/// @return The index where the element was inserted.
	///-----------------------------------------------------------------------------
	constexpr I Insert( const I nIndex, const T &element )
	{
		return Insert( nIndex, 1, { element } );
	}

	///-----------------------------------------------------------------------------
	/// @brief Insert a single element at position @p nIndex (move).
	/// @return The index where the element was inserted.
	///-----------------------------------------------------------------------------
	constexpr I Insert( const I nIndex, T &&element )
	{
		return Insert( nIndex, 1, { Move( element ) } );
	}

	///-----------------------------------------------------------------------------
	/// @brief Insert multiple elements at position @p index (copying arguments).
	/// @return The index where the element was inserted.
	///-----------------------------------------------------------------------------
	template < typename ...Ts > I constexpr InsertMultiple( I nIndex, Ts &&...args ) { return Insert( nIndex, sizeof...(Ts), { Forward< Ts >( args )... } ); }

	template < I N > constexpr I AddToHead( const I nCount, const T ( &arrElements )[ N ] ) { return Insert( 0, nCount, arrElements ); }
	template < I N > constexpr I AddToHead( const I nCount, T ( &&arrElements )[ N ] ) { return Insert( 0, nCount, arrElements ); }
	constexpr I AddToHead( const T &element ) { return Insert( 0, element ); }
	constexpr I AddToHead( T &&element ) { return Insert( 0, Move( element ) ); }
	constexpr I AddToHead( ConstView_t v ) { return Insert( 0, v ); }
	template < typename ...Ts > constexpr I AddMultipleToHead( Ts &&...args ) { return InsertMultiple( 0, Forward< Ts >( args )... ); }

	template < I N > constexpr I AddToTail( const I nCount, const T ( &arrElements )[ N ] ) { return Insert( Count(), nCount, arrElements ); }
	template < I N > constexpr I AddToTail( const I nCount, T ( &&arrElements )[ N ] ) { return Insert( Count(), nCount, Move( arrElements ) ); }
	constexpr I AddToTail( const T &element ) { return Insert( Count(), element ); }
	constexpr I AddToTail( T &&element ) { return Insert( Count(), Move( element ) ); }
	constexpr I AddToTail( ConstView_t v ) { return Insert( Count(), v ); }
	template < typename ...Ts > constexpr I AddMultipleToTail( Ts &&...args ) { return InsertMultiple( Count(), Forward< Ts >( args )... ); }

	constexpr I Remove( const I nIndex, const I n = 1 )
	{
		I nCount = Count();

		BALL_ASSERT( 0 < nCount );
		BALL_ASSERT( 0 <= nIndex && nIndex <= nCount );

		T *pData = Base();

		DestructElement( &pData[ nIndex ] );
		ShiftElements( &pData[ nIndex ], &pData[ nIndex + n ], &pData[ nCount ] );
		Grow( -n );

		return nIndex;
	}

	///-----------------------------------------------------------------------------
	/// @brief Replace a range [index, index + nRemove) with @p svRepl in-place.
	///        No aliasing with temporaries: we shift the suffix explicitly and
	///        copy the replacement into its final position.
	///        Returns index of the last written character of the inserted part
	///        (or prefix end - 1 if svRepl is empty and string shrinks).
	///-----------------------------------------------------------------------------
	constexpr I Replace( I i, I nRemove, ConstView_t svRepl )
	{
		const I nCount = Count();

		BALL_ASSERT( i <= nCount );
		BALL_ASSERT( nRemove <= ( nCount - i ) );

		const I nWith   = svRepl.Count();
		const I nDelta  = nWith - nRemove; // can be negative
		T *pData        = const_cast< T * >( Base() );

		// Case 1: exact-size replace -> copy over the window and done.
		if ( nDelta == I( 0 ) )
		{
			for ( I k = 0; k < nWith; ++k )
				pData[ i + k ] = svRepl[ k ];

			return ( nWith == I( 0 ) ) ? ( ( i == I( 0 ) ) ? INVALID_INDEX : ( i - 1 ) )
			                           : ( i + nWith - 1 );
		}

		// Case 2: shrink (nWith < nRemove) -> move suffix left, shrink logical size.
		if ( nDelta < I( 0 ) )
		{
			const I nShrink = -nDelta; // amount to pull left
			// 2.1 Copy replacement into its final spot [i .. i + nWith)
			for ( I k = 0; k < nWith; ++k )
				pData[ i + k ] = svRepl[ k ];

			// 2.2 Shift suffix left by nShrink.
			const I nSuffixBegin  = i + nRemove;
			const I nOldCount     = nCount;

			ShiftElements( &pData[ i + nWith ], &pData[ nSuffixBegin ], &pData[ nOldCount ] );

			// 2.3 Commit new logical size.
			Base_t::Set( nCount - nShrink, pData );

			return ( nWith == I( 0 ) ) ? ( ( i == I( 0 ) ) ? INVALID_INDEX : i )
			                           : ( i + nWith );
		}

		// Case 3: grow (nWith > nRemove) -> ensure capacity, shift suffix right, then copy.
		{
			const I nGrow = nDelta; // amount to push right
			const I nNew  = nCount + nGrow;

			// 3.1 Ensure capacity; pointer may change.
			pData = Base_t::EnsureCapacity( nNew );

			// 3.2 Shift suffix right by nGrow: [i + nRemove .. n) -> starts at [i + nWith .. )
			const I nSuffixBegin  = i + nRemove;
			const I nOldCount     = nCount;

			ShiftElements( &pData[ i + nWith ], &pData[ nSuffixBegin ], &pData[ nOldCount ] );

			// 3.3 Commit new logical size before filling the gap.
			Base_t::Set( nNew, pData );

			// 3.4 Copy replacement into its final spot.
			for ( I k = 0; k < nWith; ++k )
				pData[ i + k ] = svRepl[ k ];

			return i + nWith - 1;
		}
	}

	///-----------------------------------------------------------------------------
	/// @brief Replace a range [index, index + nRemove) with a C-string (null-terminated).
	///-----------------------------------------------------------------------------
	constexpr I Replace( I i, I nRemove, const T *pRepl )
	{
		// Treat nullptr as empty replacement.
		return Replace( i, nRemove, pRepl ? ConstView_t( pRepl ) : ConstView_t() );
	}

	///-----------------------------------------------------------------------------
	/// @brief Replace first occurrence of @p what with @p with. Returns index of replaced range start or INVALID_INDEX.
	///-----------------------------------------------------------------------------
	constexpr I ReplaceFirst( ConstView_t svWhat, ConstView_t svWith )
	{
		const I iFound = Find( svWhat );

		if ( iFound == INVALID_INDEX )
			return INVALID_INDEX;

		Replace( iFound, svWhat.Count(), svWith );

		return iFound;
	}

	///-----------------------------------------------------------------------------
	/// @brief Replace all non-overlapping occurrences of @p what with @p with.
	///        Returns count of replacements.
	///        NOTE: If @p what is empty, no-op (returns 0) to avoid infinite loop.
	///-----------------------------------------------------------------------------
	constexpr I ReplaceAll( ConstView_t svWhat, ConstView_t svWith )
	{
		const I nWhat = svWhat.Count();

		if ( nWhat == I( 0 ) )
			return I( 0 );

		I nReplaced = 0;
		I nFrom     = I( 0 );

		for ( ; ; )
		{
			const I iFound = Find( svWhat, nFrom );
			if ( iFound == INVALID_INDEX )
				break;

			Replace( iFound, nWhat, svWith );
			++nReplaced;

			// Advance beyond the just-inserted region to avoid re-matching inside replacement.
			const I nWith = svWith.Count();

			nFrom = iFound + ( nWith > 0 ? nWith : I( 1 ) ); // ensure forward progress even when nWith == 0

			// Clamp in case of pathological inputs.
			if ( nFrom > Count() )
				break;
		}

		return nReplaced;
	}

	///-----------------------------------------------------------------------------
	/// @brief Replace all occurrences of single character @p fromCh with @p toCh.
	///        Returns count of replacements.
	///-----------------------------------------------------------------------------
	constexpr I Replace( const T &from, const T &to )
	{
		T *p = const_cast< T * >( Base() );
		const I nCount = Count();

		I n = 0;

		for ( I k = 0; k < nCount; ++k )
		{
			if ( p[ k ] == from )
			{
				p[ k ] = to;
				++n;
			}
		}

		return n;
	}

	/// @brief Replace [index, index + len) with src (shifts tail if needed).
	constexpr I ReplaceRange( const I nIndex, ConstView_t src )
	{
		I nCount = Count();
		I nViewCount = src.Length();

		BALL_ASSERT( 0 <= nIndex && nIndex <= nCount );

		if ( nIndex < 0 ) nIndex = 0;
		if ( nViewCount < 0 ) nViewCount = 0;
		if ( nIndex > nCount ) nIndex = nCount;
		if ( nIndex + nViewCount > nCount ) nViewCount = nCount - nIndex;

		const I nTailStart = nIndex + nViewCount;
		const I nTailCount = nCount - nTailStart;
		const I nInsertCount  = src.Length();
		const I nNewCount  = nCount - nViewCount + nInsertCount;

		T *pElements = EnsureCapacity( nNewCount );

		// Move tail if sizes differ.
		if ( nTailCount > 0 && ( nInsertCount != nViewCount ) )
		{
			CopyElements( nTailCount, pElements + nIndex + nInsertCount, pElements + nTailStart );
		}

		// Copy inserted segment.
		if ( nInsertCount > 0 )
		{
			CopyElements( nInsertCount, pElements + nIndex, src.Base() );
		}

		Set( nNewCount, pElements );

		return nIndex;
	}

	constexpr void RemoveAll()
	{
		for ( auto &it : *this )
			DestructElement( &it );
	}

	constexpr void Purge()
	{
		RemoveAll();
	}

protected:
	using Base_t::EnsureCapacity;
	using Base_t::Set;

	///-----------------------------------------------------------------------------
	/// @brief Ensure space for inserting @p nAddCount elements at position @p nIndex,
	///        grow storage if needed, shift the tail to the right, and commit size.
	/// @details
	///  - Computes required total size (Count() + nAddCount) and calls EnsureCapacity()
	///    which rounds up to next power-of-two and Alloc/Reallocs if needed.
	///  - If the underlying pointer changes after EnsureCapacity(), commits it via Set()
	///    while preserving the old element count.
	///  - Shifts the existing suffix [nIndex, nOld) right by nAddCount slots.
	///  - Updates the logical element count to @p nOld + @p nAddCount.
	///  - Returns a writable pointer to the first slot where caller should place data.
	/// @note No element construction is performed here; caller must write/construct
	///       the nAddCount elements into the returned gap.
	/// @pre  0 <= nIndex <= Count(), nAddCount > 0.
	///-----------------------------------------------------------------------------
	constexpr T *EnsureInsert( I nIndex, I nAddCount )
	{
		BALL_ASSERT( nAddCount > 0 );
		BALL_ASSERT( nIndex <= Count() );

		const I nOldCount = Count();
		const I nNewCount = nOldCount + nAddCount;

		// 1) Ensure capacity for the final size (rounded to pow2 inside).
		T *pData = EnsureCapacity( nNewCount );

		// 2) Make a hole: shift the suffix [nIndex, nOldCount) to the right.
		if ( nIndex < nOldCount )
		{
			ShiftElements( &pData[ nIndex + nAddCount ], &pData[ nIndex ], &pData[ nOldCount ] );
		}

		// 3) Commit the new logical size; the gap [nIndex, nIndex + nAddCount)
		//    is now reserved for the caller to fill.
		Set( nNewCount, pData );

		return &pData[ nIndex ];
	}

	constexpr void SetCount( I nNew )
	{
		I nOld = Count();

		Set( nNew, EnsureCapacity( nNew ) );

		T *pData = Base();

		if ( nOld < nNew )
			ConstructElements( &pData[ nOld ], &pData[ nNew ] );
		else if ( nOld > nNew )
			DestructElements( &pData[ nNew ], &pData[ nOld ] );
	}
}; // class CVectorImpl

template < typename I, I N, typename T, class A > class CBufferVector;

template < typename I, typename T, class A = CAllocator< I, T > >
class CVector : public CVectorImpl< CVectorBase< CMemoryView< I, T >, I, T, A >, I, T >
{
public:
	using Base_t = CVectorImpl< CVectorBase< CMemoryView< I, T >, I, T, A >, I, T >;
	using Base_t::Base_t;

	template < I N > constexpr CVector( const CBufferVector< I, N, T, A > &other ) : Base_t( other ) {}
	template < I N > constexpr CVector &operator=( const CBufferVector< I, N, T, A > &other ) { Base_t::CopyFrom( other ); return *this; }
};

template < typename I, I N, typename T, class A = CAllocator< I, T > >
class CBufferVector : public CVectorImpl< CVectorBase< CMemoryView< I, T, N >, I, T, A >, I, T >
{
public:
	using Base_t = CVectorImpl< CVectorBase< CMemoryView< I, T, N >, I, T, A >, I, T >;
	using Base_t::Base_t;

	constexpr CBufferVector( const CVector< I, T, A > &other ) : Base_t( other ) {}
	constexpr CBufferVector &operator=( const CVector< I, T, A > &other ) { Base_t::CopyFrom( other ); return *this; }
};

template < typename T > using Vector_t =            CVector< size_t, T >;
template < typename T > using Vector8_t =           CVector< uint8_t, T >;
template < typename T > using Vector16_t =          CVector< uint16_t, T >;
template < typename T > using Vector32_t =          CVector< uint32_t, T >;
template < typename T > using Vector64_t =          CVector< uint64_t, T >;

template < typename T, size_t N > using BufferVector_t =            CBufferVector< size_t, N, T >;
template < typename T, uint8_t N > using BufferVector8_t =          CBufferVector< uint8_t, N, T >;
template < typename T, uint16_t N > using BufferVector16_t =        CBufferVector< uint16_t, N, T >;
template < typename T, uint32_t N > using BufferVector32_t =        CBufferVector< uint32_t, N, T >;
template < typename T, uint64_t N > using BufferVector64_t =        CBufferVector< uint64_t, N, T >;

#endif // !defined( _INCLUDE_BALL_TYPES_VECTOR_HPP_ )
