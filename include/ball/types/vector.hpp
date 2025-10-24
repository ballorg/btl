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
	using Base_t =      B;
	using Index_t =     I;
	using Element_t =   T;
	using Allocator_t = A;
	using Number_t =    MNumber< Index_t >;
	using Unsigned_t =  typename Number_t::U;
	using View_t =      Base_t;
	using ConstView_t = typename Base_t::Const_t;

	using Base_t::Base_t;
	using Base_t::Count;
	using Base_t::Data;

	static constexpr bool IS_GROWABLE = false;
	static constexpr size_t ALIGNED_SIZE = NextPowerOfTwo_Const( 8 * sizeof( Element_t ) );
	static constexpr I INVALID_INDEX = Number_t::INVALID;

	/// @brief Default / external ctor. Does not assume ownership semantics beyond this instance.
	constexpr ~CVectorBase() noexcept
	{
		T *pElements = Data();

		if ( pElements )
		{
			Allocator_t::Free( pElements );
		}
	}

	// ---------------------------
	// Basic queries / accessors.
	// ---------------------------

	/// @warning This returns next power-of-two of Count(), not a tracked internal capacity.
	constexpr I Capacity() const noexcept { return NextPowerOfTwo( Count() ); }
	constexpr size_t CapacitySize() const noexcept { return static_cast< size_t >( Capacity() ) * sizeof( Element_t ); }

protected:
	using Base_t::Set;

	///-----------------------------------------------------------------------------
	/// @brief Ensure the backing heap storage can hold at least @p nRequestCapacity
	///        elements (rounded up to a power of two).
	///
	/// Behavior overview:
	///   - Rounds @p nRequestCapacity to the next power of two via NextPowerOfTwo().
	///   - Grows only (never shrinks heap capacity). If the rounded capacity is
	///     <= current derived capacity (see below), it is a no-op.
	///   - Uses Allocator_t::Realloc() when storage already exists; otherwise
	///     Allocator_t::Alloc() to create a new heap block.
	///
	/// Important notes and invariants:
	///   - Capacity() here is *derived from Count()* (next power of two) rather
	///     than being tracked as a dedicated member. This class assumes callers
	///     will not rely on a stored capacity field.
	///   - NUM_ALIGNED/ALIGNED_SIZE act as allocator hints for bucket/alignment.
	///     The allocator is expected to understand ALIGNED_SIZE as a preferred
	///     alignment/size class for amortized growth.
	///   - On overflow of power-of-two computation, NextPowerOfTwo() yields
	///     Number_t::INVALID; we assert and bail defensively.
	///   - No element moves/copies occur here; this function is *purely about
	///     reserving heap memory*. The caller is responsible for updating
	///     the vector’s internal state (e.g., Set) and for constructing
	///     or moving elements if required elsewhere.
	///   - Not thread-safe. External synchronization is required for concurrent use.
	///
	/// Complexity:
	///   - O(1) when no allocation occurs.
	///   - O(1) expected for Alloc/Realloc (allocator-dependent); element
	///     relocation, if any, is an allocator concern and not performed here.
	///
	/// Safety:
	///   - At call sites is preserved by policy; we assert
	///     on overflow. If Alloc/Realloc returns nullptr, callers must check the
	///     returned pointer before use (this function does not throw).
	///
	/// @param nRequestCapacity Minimum required number of elements before rounding.
	/// @return Data pointer to storage (may be unchanged or reallocated).
	///-----------------------------------------------------------------------------
	constexpr T *EnsureCapacity( I nRequestCapacity )
	{
		// Normalize request: round up to next power of two.
		nRequestCapacity = NextPowerOfTwo( nRequestCapacity );

		// Current heap pointer (may be null if nothing allocated yet).
		T *pElements = Data();

		// Guard against overflow in power-of-two computation.
		BALL_ASSERT_MESSAGE( nRequestCapacity != Number_t::INVALID, "Capacity overflow!" );

		// If overflow detected or the requested (rounded) capacity is not greater
		// than the current derived capacity, nothing to do.
		if ( nRequestCapacity == Number_t::INVALID || nRequestCapacity < Capacity() )
			return pElements;

		// Grow path:
		// - If storage exists, try to reallocate in place (or move by allocator).
		// - Otherwise, allocate a fresh block.
		if ( pElements )
		{
			// Re-bucket to the new power-of-two capacity. ALIGNED_SIZE is a hint.
			pElements = Allocator_t::Realloc( pElements, nRequestCapacity, ALIGNED_SIZE );
			BALL_ASSERT_MESSAGE( pElements != nullptr, "Failed to reallocate elements" );
		}
		else
		{
			// First-time allocation to the requested power-of-two capacity.
			pElements = Allocator_t::Alloc( nRequestCapacity, ALIGNED_SIZE );
			BALL_ASSERT_MESSAGE( pElements != nullptr, "Failed to allocate elements" );
		}

		// Note: We intentionally do not call Set() here; the caller controls
		// when to commit the new pointer/size relationship to Base_t.
		return pElements;
	}

	/// @brief Copy contents from another CVectorBase.
	CVectorBase &CopyFrom( const CMemoryView< I, T > &other )
	{
		const I nNewCount = other.Count();

		T *pElements = EnsureCapacity( nNewCount );

		if ( nNewCount > 0 )
		{
			CopyElements( other.Count(), pElements, other.Data() );
		}

		Set( nNewCount, pElements );

		return *this;
	}

	using Base_t::MoveFrom;
};

template < class B, typename I, typename T, I N, class A = CAllocator< I, T > >
class CVectorBase_Growable : public CVectorBase< B, I, T, A >
{
public:
	using Base_t =      CVectorBase< B, I, T, A >;
	using Index_t =     Base_t::Index_t;
	using Element_t =   Base_t::Element_t;
	using Allocator_t = Base_t::Allocator_t;
	using Number_t =    Base_t::Number_t;
	using Unsigned_t =  Base_t::Unsigned_t;
	using View_t =      Base_t::View_t;
	using ConstView_t = Base_t::ConstView_t;

	using Base_t::Count;
	using Base_t::Size;
	using Base_t::Data;

	static constexpr bool IS_GROWABLE = true;
	static constexpr size_t ALIGNED_SIZE = NextPowerOfTwo_Const( N * sizeof( Element_t ) );
	static constexpr I INVALID_INDEX = Number_t::INVALID;

	constexpr CVectorBase_Growable() noexcept : Base_t( 0, reinterpret_cast< Element_t * >( &m_FixedElements ) ) {}
	constexpr ~CVectorBase_Growable() noexcept
	{
		if ( IsOverflow() )
		{
			Allocator_t::Free( Base_t::Data() );
		}

		Base_t::Set( 0, nullptr );
	}

	/// @warning This returns next power-of-two of Count(), not a tracked internal capacity.
	constexpr I Capacity() const noexcept { return NextPowerOfTwo< I, N >( Count() ); }
	constexpr size_t CapacitySize() const noexcept { return static_cast< size_t >( Capacity() ) * sizeof( Element_t ); }

	constexpr I FixedCount() const noexcept { return static_cast< I >( N ); }
	constexpr size_t FixedSize() const noexcept { return FixedCount() * sizeof( Element_t ); }

	constexpr I FixedCapacity() const noexcept { return NextPowerOfTwo_Unified( FixedCount() ); } 
	constexpr size_t FixedCapacitySize() const noexcept { return FixedCapacity() * sizeof( Element_t ); }

	constexpr bool IsOverflow( I nCount ) const noexcept { return nCount > I( FixedCount() ); }
	constexpr bool IsOverflow() const noexcept { return IsOverflow( Count() ); }

protected:
	///-----------------------------------------------------------------------------
	/// @brief Ensure underlying storage can hold at least @p nRequestCapacity elements.
	///
	/// This grows or shrinks the backing storage depending on two conditions:
	///   1) Whether the requested capacity exceeds the fixed inline buffer (N).
	///   2) Whether the vector is currently "overflowed" (i.e., backed by heap memory).
	///
	/// Growth policy:
	///   - When @p nRequestCapacity > FixedCount() the capacity is rounded up to the
	///     next power of two ( >= N ) via NextPowerOfTwo< I, N >(). This ensures amortized
	///     O(1) append behavior and reduces the number of reallocations.
	///
	/// Shrink policy:
	///   - This function never shrinks heap capacity to a smaller heap capacity.
	///     However, if the request fits into the fixed inline storage and the vector
	///     is currently overflowed, it migrates back to the fixed buffer and frees
	///     the heap block (MoveToFixed()).
	///
	/// Invariants and notes:
	///   - Capacity() here is computed from Count() (next power of two); there is no
	///     separately tracked "capacity" member. We rely on the allocator's returned
	///     block being >= the computed next power-of-two when growing.
	///   - NUM_ALIGNED/ALIGNED_SIZE act as allocator hints (alignment/bucket size).
	///     For heap allocations/reallocations we pass ALIGNED_SIZE to let the allocator
	///     choose a bucket aligned to a power-of-two number of elements.
	///   - On overflow of the capacity computation, NextPowerOfTwo returns Number_t::INVALID.
	///     We assert on that condition and bail out defensively.
	///   - Not thread-safe; external synchronization is required if used concurrently.
	///   - Strong no-throw guarantee for client code paths (this function itself does not
	///     throw, but will assert on allocation failure in debug builds).
	///
	/// Complexity:
	///   - O(1) when no migration occurs.
	///   - O(n) element moves when switching between fixed and heap storage (via MoveToHeap /
	///     MoveToFixed) or when the allocator has to reallocate.
	///
	/// @param nRequestCapacity Minimum number of elements the storage must be able to hold.
	///                         May be >= Count() when growing; can be < Count() if the caller
	///                         only wants to ensure it still fits in fixed storage (no removal
	///                         of elements is performed here).
	/// @return New base pointer to elements (may be different from previous Data()).
	///-----------------------------------------------------------------------------
	constexpr T *EnsureCapacity( const I nRequestCapacity )
	{
		// Current base pointer; may point to the fixed inline buffer or to heap.
		T *pElements = Data();

		// Case A: The request exceeds the fixed inline storage (N).
		if ( IsOverflow( nRequestCapacity ) )
		{
			// Compute the next power-of-two capacity (>= nRequestCapacity and >= N).
			const I nNewCapacity = NextPowerOfTwo< I, N >( nRequestCapacity );

			// If the power-of-two computation overflowed, abort in debug builds.
			BALL_ASSERT_MESSAGE( nNewCapacity != Number_t::INVALID, "Capacity overflow!" );

			// Defensive bail-out: if computation failed or the proposed capacity would be
			// smaller than our current (derived) capacity, do nothing.
			// Note: We never shrink heap capacity here; shrinking is only done by migrating
			// back to fixed storage (see Case B below).
			if ( nNewCapacity == Number_t::INVALID || nNewCapacity < Capacity() )
				return pElements;

			// Subcase A1: Already on heap — grow in place if possible.
			if ( IsOverflow() )
			{
				// Try to re-bucket the allocation to the new power-of-two capacity.
				// ALIGNED_SIZE is used as an allocator hint (alignment/bucket size).
				pElements = Allocator_t::Realloc( pElements, nNewCapacity, ALIGNED_SIZE );

				// If allocation fails, pElements would be nullptr; we assert in debug.
				// In release builds, downstream code must not dereference nullptr.
				BALL_ASSERT_MESSAGE( pElements != nullptr, "Failed to reallocate elements (growable)" );
			}
			// Subcase A2: Currently using the fixed inline buffer — migrate to heap.
			else
			{
				// Allocate a new heap block with the requested power-of-two capacity.
				pElements = Allocator_t::Alloc( nNewCapacity, ALIGNED_SIZE );
				BALL_ASSERT_MESSAGE( pElements != nullptr, "Failed to allocate elements (growable)" );

				// Move/copy existing elements from the fixed buffer into the new heap block
				// and update the base pointer inside Base_t. This is O(n).
				MoveToHeap( pElements );
			}
		}
		// Case B: The request fits into the fixed inline storage.
		else if ( IsOverflow() )
		{
			// We are currently on heap but the required capacity fits inline.
			// Migrate elements back to the fixed buffer and free the heap block.
			// This reduces memory footprint; complexity is O(n).
			MoveToFixed( pElements );
			// After MoveToFixed, Data() now points to the fixed buffer; keep returning
			// the (updated) pElements for convenience/consistency with callers.
		}

		// Return the (possibly updated) base pointer for the caller to continue working with.
		return pElements;
	}

	using Base_t::Set;

	constexpr void MoveToFixed( const T *pElements )
	{
		BALL_ASSERT( pElements != nullptr );
		CopyElements( FixedCount(), reinterpret_cast< T * >( &m_FixedElements ), pElements );
	}

	constexpr void MoveToHeap( T *pElements )
	{
		BALL_ASSERT( pElements != nullptr );
		CopyElements( FixedCount(), pElements, reinterpret_cast< const T * >( &m_FixedElements ) );
	}

	constexpr void Swap( CVectorBase_Growable &other ) noexcept
	{
		Base_t::Swap( other.View() );

		if ( !IsOverflow() )
		{
			Math_Swap( m_FixedElements, other.m_FixedElements );
		}
	}

	constexpr CVectorBase_Growable &CopyFrom( ConstView_t &other ) noexcept
	{
		I nOtherCount = other.Count();

		T *pElements = EnsureCapacity( nOtherCount );

		CopyElements( other.Count(), pElements, other.Data() );
		Set( nOtherCount, pElements );

		return *this;
	}
	constexpr CVectorBase_Growable &CopyFrom( View_t &other ) noexcept { return CopyFrom( other.Const() );  }
	constexpr CVectorBase_Growable &MoveFrom( CVectorBase_Growable &&other ) noexcept
	{
		Swap( static_cast< CVectorBase_Growable & >( other ) );

		return *this;
	}

private:
	T m_FixedElements[ N ];
}; // class CVectorBase_Growable

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
	using Base_t::Count;
	using Base_t::Data;
	using Base_t::EnsureCapacity;

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

		const T *pViewData = v.Data();
		I nViewCount = v.Count();

		T *pData = EnsureInsert( nIndex, nViewCount );

		// Construct each new element
		for ( I n = 0; n < nViewCount; n++ )
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
		for ( I n = 0; n < nCount; n++ )
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
		for ( I n = 0; n < nCount; n++ )
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

		T *pData = Data();

		DestructElement( &pData[ nIndex ] );
		ShiftElements( &pData[ nIndex ], &pData[ nIndex + n ], &pData[ nCount ] );
		Grow( -n );

		return nIndex;
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
			CopyElements( nInsertCount, pElements + nIndex, src.Data() );
		}

		Set( nNewCount, pElements );

		return nIndex;
	}

	void RemoveAll()
	{
		for ( auto &it : *this )
			DestructElement( &it );

		SetCount( 0 );
	}

	void Purge()
	{
		RemoveAll();
	}

protected:
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
	constexpr T *EnsureInsert( I nIndex, I nAddCount ) noexcept
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
		Base_t::Set( nNewCount, pData );

		return &pData[ nIndex ];
	}

	constexpr void SetCount( I nNew ) noexcept
	{
		I nOld = Count();

		Base_t::Set( nNew, Base_t::EnsureCapacity( nNew ) );

		T *pData = Data();

		if ( nOld < nNew )
			ConstructElements( &pData[ nOld ], &pData[ nNew ] );
		else if ( nOld > nNew )
			DestructElements( &pData[ nNew ], &pData[ nOld ] );
	}
}; // class CVectorImpl

template < typename I, typename T, I N, class A > class CBufferVector;

template < typename I, typename T, class A = CAllocator< I, T > >
class CVector : public CVectorImpl< CVectorBase< CMemoryView< I, T >, I, T, A >, I, T >
{
public:
	using Base_t = CVectorImpl< CVectorBase< CMemoryView< I, T >, I, T, A >, I, T >;
	using Base_t::Base_t;

	template < I N >
	CVector( const CBufferVector< I, T, N, A > &other ) :
		Base_t( other.View() )
	{
	}

	template < I N > CVector &operator=( const CBufferVector< I, T, N, A > &other )
	{
		Base_t::operator=( other.View() );

		return *this;
	}
};

template < typename I, typename T, I N, class A = CAllocator< I, T > >
class CBufferVector : public CVectorImpl< CVectorBase_Growable< CMemoryView< I, T >, I, T, N, A >, I, T >
{
public:
	using Base_t = CVectorImpl< CVectorBase_Growable< CMemoryView< I, T >, I, T, N, A >, I, T >;
	using Base_t::Base_t;

	CBufferVector( const CVector< I, T, A > &other ) :
		Base_t( other.View() )
	{
	}

	CBufferVector &operator=( const CVector< I, T, A > &other )
	{
		Base_t::operator=( other.View() );

		return *this;
	}
};

template < typename T > using Vector_t =            CVector< size_t, T >;
template < typename T > using Vector8_t =           CVector< uint8_t, T >;
template < typename T > using Vector16_t =          CVector< uint16_t, T >;
template < typename T > using Vector32_t =          CVector< uint32_t, T >;
template < typename T > using Vector64_t =          CVector< uint64_t, T >;

template < typename T, size_t N > using BufferVector_t =            CBufferVector< size_t, T, N >;
template < typename T, uint8_t N > using BufferVector8_t =          CBufferVector< uint8_t, T, N >;
template < typename T, uint16_t N > using BufferVector16_t =        CBufferVector< uint16_t, T, N >;
template < typename T, uint32_t N > using BufferVector32_t =        CBufferVector< uint32_t, T, N >;
template < typename T, uint64_t N > using BufferVector64_t =        CBufferVector< uint64_t, T, N >;

#endif // !defined( _INCLUDE_BALL_TYPES_VECTOR_HPP_ )
