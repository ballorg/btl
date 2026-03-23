#ifndef _INCLUDE_BALL_TYPES_VECTOR_HPP_
#	define _INCLUDE_BALL_TYPES_VECTOR_HPP_

#	pragma once

#	include "base/arch.h"
#	include "c/assert.h"
#	include "c/memory.h"
#	include "c/memoryaligned.h"
#	include "fixed.hpp"
#	include "meta/fixed.hpp"
#	include "meta/isintegral.hpp"
#	include "meta/xvalue.hpp"
#	include "allocator.hpp"
#	include "bits.hpp"
#	include "elements.hpp"
#	include "math.hpp"
#	include "view.hpp"

// ===============================
// CVectorBase (now derives from CView)
// ===============================
template < class B, typename I, typename T, class A = CAllocator< I, T > >
class CVectorBase : public A, public B
{
public:
	using Base_t      = B;
	using Index_t     = I;
	using Element_t   = T;
	using BaseAllocator_t = A::Base_t;
	using Allocator_t = A;
	using Fixed_t     = MFixed< Index_t >;
	using Unsigned_t  = typename Fixed_t::Unsigned_t;
	using typename Base_t::View_t;
	using typename Base_t::ConstView_t;
	template < I N > using GrowableView_t = typename Base_t::template GrowableView_t< N >;
	template < I N > using ConstGrowableView_t = typename Base_t::template ConstGrowableView_t< N >;

	using Base_t::Base_t;
	using Base_t::FIXED_COUNT;
	using Base_t::IS_PACKED_STORAGE;
	using Base_t::Count;
	using Base_t::IsOverflow;
	using Base_t::IsPackedOverflow;
	using Base_t::FixedData;
	using Base_t::Data;
	using Base_t::Base;
	using Base_t::PackedData;
	using Base_t::PackedBase;
	using Base_t::PackedFixedData;
	using Base_t::PackedBytesForCount;
	using Base_t::Set;

	static constexpr I INVALID_INDEX = Fixed_t::INVALID;
	static constexpr bool IS_GROWABLE = FIXED_COUNT > 0;
	static constexpr size_t ALIGNED_SIZE = alignof( Element_t );

	/// @brief Default / external ctor. Does not assume ownership semantics beyond this instance.
	constexpr ~CVectorBase() noexcept
	{
		if ( IsOverflow() )
		{
			if constexpr ( IS_PACKED_STORAGE )
				BaseAllocator_t::Free( PackedData() );
			else
				Allocator_t::Free( Data() );
		}

		Set( 0, nullptr );
	}

	// ---------------------------
	// Basic queries / accessors.
	// ---------------------------

	constexpr I Capacity() const noexcept { return BitCeil( Count() ); }
	constexpr size_t CapacitySize() const noexcept
	{
		if constexpr ( IS_PACKED_STORAGE )
			return PackedBytesForCount( Capacity() );
		else
			return static_cast< size_t >( Capacity() ) * sizeof( Element_t );
	}

	constexpr I FixedCount() const noexcept
	{
		if constexpr ( IS_PACKED_STORAGE )
			return I( 0 );
		else
			return FIXED_COUNT;
	}
	constexpr size_t FixedSize() const noexcept
	{
		if constexpr ( IS_PACKED_STORAGE )
			return size_t( 0 );
		else
			return FixedCount() * sizeof( Element_t );
	}

	constexpr I FixedCapacity() const noexcept
	{
		if constexpr ( IS_PACKED_STORAGE )
			return I( 0 );
		else
			return NextPowerOfTwo_Unified( FixedCount() );
	}
	constexpr size_t FixedCapacitySize() const noexcept
	{
		if constexpr ( IS_PACKED_STORAGE )
			return size_t( 0 );
		else
			return FixedCapacity() * sizeof( Element_t );
	}

protected:
	///-----------------------------------------------------------------------------
	/// @brief Ensures that heap storage can accommodate at least @p nRequestedCount
	///        elements, growing using NextDoublingCapacity().
	///
	/// Overview:
	///   - Computes growth from current capacity via NextDoublingCapacity(), clamped
	///     to [MIN_CAPACITY_COUNT, Fixed_t::MAX].
	///   - Expands capacity only; never shrinks unless migrating back to fixed storage.
	///   - Uses Allocator_t::Realloc() for existing heap memory, otherwise Allocator_t::Alloc().
	///
	/// Behavior details:
	///   - If @p nRequestedCount exceeds the fixed inline buffer (N), capacity grows to the
	///     next doubled step sufficient for nRequestCapacity.
	///   - If already on heap and new capacity equals the current derived capacity, this is a no-op.
	///   - If still using fixed storage but overflow is detected, a new heap block is allocated,
	///     and data is migrated via MoveToHeap() if IS_GROWABLE is enabled.
	///   - If the new request fits back into the inline buffer while currently on heap,
	///     the data is migrated back via MoveToFixed() (only if IS_GROWABLE).
	///
	/// Invariants and assumptions:
	///   - Capacity() is derived from Count() (rounded to power of two), not stored explicitly.
	///   - NUM_ALIGNED / ALIGNED_SIZE act as allocator hints for alignment and bucket size.
	///   - Overflow is clamped defensively; invalid results are asserted.
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
	/// @param nRequested Minimum required integer of elements before rounding.
	/// @return Pointer to valid element storage (possibly reallocated or migrated).
	///-----------------------------------------------------------------------------
	constexpr T *EnsureCapacity( I nRequested )
	{
		const I nCount = Count();
		const I nCapacity = BitCeil( nCount );
		const I nNewCapacity = BitCeil( nRequested );

		BALL_ASSERT_MESSAGE( nCapacity != Fixed_t::INVALID, "Current capacity overflow!" );
		BALL_ASSERT_MESSAGE( nNewCapacity != Fixed_t::INVALID, "Requested capacity overflow!" );

		if constexpr ( IS_PACKED_STORAGE )
		{
			const bool bWillOverflow = IsPackedOverflow( nRequested );
			const bool bWasOverflow = IsPackedOverflow( nCount );
			uchar_t *pData = PackedBase();
			uchar_t *pCurrent = pData;

			const size_t nOldBytes = PackedBytesForCount( nCapacity );
			const size_t nNewBytes = PackedBytesForCount( nNewCapacity );
			const size_t nLiveBytes = PackedBytesForCount( nCount );

			if ( bWillOverflow )
			{
				if ( bWasOverflow )
				{
					if ( nNewBytes == nOldBytes && pData != nullptr )
						return reinterpret_cast< T * >( pData );

					pData = reinterpret_cast< uchar_t * >( BaseAllocator_t::Realloc( pData, static_cast< I >( nNewBytes ), 8u ) );

					BALL_ASSERT_IF_MESSAGE( pData == nullptr, "Failed to reallocate packed storage" )
						return reinterpret_cast< T * >( pCurrent );

					if ( nOldBytes < nNewBytes )
						memset( pData + nOldBytes, 0, nNewBytes - nOldBytes );
				}
				else
				{
					pData = reinterpret_cast< uchar_t * >( BaseAllocator_t::Alloc( static_cast< I >( nNewBytes ), 8u ) );

					BALL_ASSERT_IF_MESSAGE( pData == nullptr, "Failed to allocate packed storage" )
						return reinterpret_cast< T * >( pCurrent );

					if ( nLiveBytes > size_t( 0 ) )
						memmove( pData, PackedFixedData(), nLiveBytes );

					if ( nLiveBytes < nNewBytes )
						memset( pData + nLiveBytes, 0, nNewBytes - nLiveBytes );
				}

				T *pNew = reinterpret_cast< T * >( pData );

				Set( nCount, pNew );

				return pNew;
			}

			T *pNew = reinterpret_cast< T * >( PackedFixedData() );

			if ( bWasOverflow )
			{
				if ( nLiveBytes > size_t( 0 ) )
					memmove( pNew, pData, nLiveBytes );

				BaseAllocator_t::Free( pData );
			}

			Set( nCount, pNew );

			return pNew;
		}
		else
		{
			T *pElements = Base();

			// Case A: request exceeds inline storage
			if ( IsOverflow( nRequested ) )
			{
				// Subcase A1: already on heap
				if ( IsOverflow() )
				{
					if ( nNewCapacity == nCapacity )
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

				Allocator_t::Free( pElements );

				return FixedData();
			}

			return pElements;
		}
	}

	constexpr void MoveToFixed( const T *pElements )
	{
		if constexpr ( IS_PACKED_STORAGE )
			return;

		T *pFixedElements = FixedData();

		BALL_ASSERT( pFixedElements != nullptr );
		BALL_ASSERT( pElements != nullptr );
		CopyElements( FixedCount(), pFixedElements, pElements );
	}

	constexpr void MoveToHeap( T *pElements )
	{
		if constexpr ( IS_PACKED_STORAGE )
			return;

		const T *pFixedElements = FixedData();

		BALL_ASSERT( pElements != nullptr );
		BALL_ASSERT( pFixedElements != nullptr );
		CopyElements( FixedCount(), pElements, pFixedElements );
	}

	/// @brief Copy contents from another CVectorBase.
	constexpr CVectorBase &CopyFrom( const View_t &other ) { return CopyFrom( other.Const() ); }
	constexpr CVectorBase &CopyFrom( const ConstView_t &other )
	{
		const I nNewCount = other.Count();
		T *pElements = EnsureCapacity( nNewCount );

		if constexpr ( IS_PACKED_STORAGE )
		{
			Set( nNewCount, pElements );

			const size_t nBytes = PackedBytesForCount( nNewCount );

			if ( nBytes > size_t( 0 ) )
				memmove( PackedBase(), other.PackedBase(), nBytes );
		}
		else
		{
			CopyElements( other.Count(), pElements, other.Base() );
			Set( nNewCount, pElements );
		}

		return *this;
	}

	template< I N > constexpr CVectorBase &CopyFrom( const GrowableView_t< N > &other ) { return CopyFrom( other.Const() ); }
	template< I N > constexpr CVectorBase &CopyFrom( const ConstGrowableView_t< N > &other )
	{
		const I nNewCount = other.Count();
		T *pElements = EnsureCapacity( nNewCount );

		if constexpr ( IS_PACKED_STORAGE )
		{
			Set( nNewCount, pElements );
			const size_t nBytes = PackedBytesForCount( nNewCount );

			if ( nBytes > size_t( 0 ) )
				memmove( PackedBase(), other.PackedBase(), nBytes );
		}
		else
		{
			CopyElements( other.Count(), pElements, other.Base() );
			Set( nNewCount, pElements );
		}

		return *this;
	}
	using Base_t::MoveFrom;
};

template < class B, typename I, typename T >
class CVectorImpl : public B
{
public:
	using Base_t = B;
	using typename Base_t::Index_t;
	using typename Base_t::Unsigned_t;
	using typename Base_t::View_t;
	using typename Base_t::ConstView_t;
	template < I GN > using GrowableView_t = typename Base_t::template GrowableView_t< GN >;
	template < I GN > using ConstGrowableView_t = typename Base_t::template ConstGrowableView_t< GN >;

	using Base_t::INVALID_INDEX;
	using Base_t::IS_PACKED_STORAGE;
	using Base_t::Count;
	using Base_t::Base;
	using Base_t::Find;
	using Base_t::CopyFrom;
	using Base_t::MoveFrom;
	using Base_t::PackedBase;
	using Base_t::PackedBytesForCount;
	using Base_t::PackedSetValue;
	using Base_t::PackedShiftRowsLeft;
	using Base_t::PackedShiftRowsRight;
	using Base_t::PackedClearRows;

	constexpr CVectorImpl() noexcept : Base_t() {}
	constexpr CVectorImpl( const View_t &other ) noexcept { CopyFrom( other ); }
	constexpr CVectorImpl( const ConstView_t &other ) noexcept { CopyFrom( other ); }
	template < I N > constexpr CVectorImpl( const GrowableView_t< N > &other ) noexcept { CopyFrom( other ); }
	template < I N > constexpr CVectorImpl( const ConstGrowableView_t< N > &other ) noexcept { CopyFrom( other ); }
	constexpr CVectorImpl( View_t &&other ) noexcept { MoveFrom( Move( other ) ); }
	template < I N > constexpr CVectorImpl( const T ( &elements )[ N ] ) noexcept : Base_t() { AddToTail( I( N ), elements ); }
	template < I N > constexpr CVectorImpl( T ( &&elements )[ N ] ) noexcept : Base_t() { AddToTail( I( N ), Move( elements ) ); }
	constexpr CVectorImpl( T &element ) noexcept : Base_t() { AddToTail( element ); }
	constexpr ~CVectorImpl() noexcept { T *pData = Base(); DestructElements( pData, pData + Count() ); }

	constexpr CVectorImpl &operator=( const View_t &other ) { return CopyFrom( other ); }
	constexpr CVectorImpl &operator=( const ConstView_t &other ) { return CopyFrom( other ); }
	template < I N > constexpr CVectorImpl &operator=( const GrowableView_t< N > &other ) noexcept { return CopyFrom( other ); }
	template < I N > constexpr CVectorImpl &operator=( const ConstGrowableView_t< N > &other ) noexcept { return CopyFrom( other ); }
	constexpr CVectorImpl &operator=( CVectorImpl &&other ) noexcept { return MoveFrom( Move( other ) ); }

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
	constexpr I Insert( const I nIndex, const ConstView_t &v )
	{
		BALL_ASSERT( !v.Empty() );

		const I nViewCount = v.Count();

		T *pData = EnsureInsert( nIndex, nViewCount );

		if constexpr ( IS_PACKED_STORAGE )
		{
			for ( I n = 0; n < nViewCount; ++n )
				PackedSetValue( nIndex + n, static_cast< T >( v.GetValue( n ) ) );
		}
		else
		{
			CopyElements_Unified( nViewCount, pData, v.Base() );
		}

		return nIndex + nViewCount;
	}
	constexpr I Insert( const I nIndex, const View_t &v ) { return Insert( nIndex, v.Const() ); }

	template < I N >
	constexpr I Insert( const I nIndex, const ConstGrowableView_t< N > &v )
	{
		BALL_ASSERT( !v.Empty() );

		const I nViewCount = v.Count();

		T *pData = EnsureInsert( nIndex, nViewCount );

		if constexpr ( IS_PACKED_STORAGE )
		{
			for ( I n = 0; n < nViewCount; ++n )
				PackedSetValue( nIndex + n, static_cast< T >( v.GetValue( n ) ) );
		}
		else
		{
			CopyElements_Unified( nViewCount, pData, v.Base() );
		}

		return nIndex + nViewCount;
	}
	template < I N > constexpr I Insert( const I nIndex, const GrowableView_t< N > &v ) { return Insert( nIndex, v.Const() ); }

	///-----------------------------------------------------------------------------
	/// @brief Inserts R elements from a C-array at position @p index (copy).
	/// @return The index where the first element was inserted.
	///-----------------------------------------------------------------------------
	template < I N >
	constexpr I InsertArray( const I nIndex, const I nCount, const T ( &arrElements )[ N ] )
	{
		static_assert( 0 < N, "InsertElements( I, const I, const & ) requires at least one element" );
		BALL_ASSERT( 0 < nCount );
		BALL_ASSERT( nCount <= N );

		T *pData = EnsureInsert( nIndex, nCount );

		if constexpr ( IS_PACKED_STORAGE )
		{
			for ( I n = 0; n < nCount; ++n )
				PackedSetValue( nIndex + n, arrElements[ n ] );
		}
		else
		{
			// Construct each new element
			for ( I n = 0; n < nCount; ++n )
			{
				ConstructElement( &pData[ n ], arrElements[ n ] );
			}
		}

		return nIndex + nCount;
	}
	template < I N > constexpr I InsertArray( const I nIndex, const T ( &arrElements )[ N ] ) { return InsertArray( nIndex, N, arrElements ); }

	///-----------------------------------------------------------------------------
	/// @brief Inserts R elements from an rvalue C-array at position @p index (move).
	/// @return The index where the first element was inserted.
	///-----------------------------------------------------------------------------
	template < I N >
	constexpr I InsertArray( const I nIndex, const I nCount, T ( &&arrElements )[ N ] )
	{
		static_assert( 0 < N, "InsertElements( I, const I, && ) requires at least one element" );
		BALL_ASSERT( 0 < nCount );
		BALL_ASSERT( nCount <= N );

		T *pData = EnsureInsert( nIndex, nCount );

		if constexpr ( IS_PACKED_STORAGE )
		{
			for ( I n = 0; n < nCount; ++n )
				PackedSetValue( nIndex + n, Move( arrElements[ n ] ) );
		}
		else
		{
			// Construct each new element
			for ( I n = 0; n < nCount; ++n )
			{
				ConstructElement( &pData[ n ], Move( arrElements[ n ] ) );
			}
		}

		return nIndex + nCount;
	}
	template < I N > constexpr I InsertArray( const I nIndex, T ( &&arrElements )[ N ] ) { return InsertArray( nIndex, N, Move( arrElements ) ); }

	///-----------------------------------------------------------------------------
	/// @brief Insert a single element at position @p nIndex (copy).
	/// @return The index where the element was inserted.
	///-----------------------------------------------------------------------------
	constexpr I Insert( const I nIndex, const T &element )
	{
		return InsertArray( nIndex, 1, { element } );
	}

	///-----------------------------------------------------------------------------
	/// @brief Insert a single element at position @p nIndex (move).
	/// @return The index where the element was inserted.
	///-----------------------------------------------------------------------------
	constexpr I Insert( const I nIndex, T &&element )
	{
		return InsertArray( nIndex, 1, { Move( element ) } );
	}

	///-----------------------------------------------------------------------------
	/// @brief Insert multiple elements at position @p nIndex (copying/moving arguments).
	/// @return The index just past the last inserted element.
	///-----------------------------------------------------------------------------
	template < typename ...Ts >
	constexpr I InsertMultiple( const I nIndex, Ts &&...args )
	{
		constexpr size_t COUNT = sizeof...( Ts );

		static_assert( COUNT > 0, "InsertMultiple requires at least one element" );

		const I nCount = I( COUNT );
		I nOffset = 0;

		T *pData = EnsureInsert( nIndex, nCount );

		if constexpr ( IS_PACKED_STORAGE )
		{
			( PackedSetValue( nIndex + nOffset++, static_cast< T >( Forward< Ts >( args ) ) ), ... );
		}
		else
		{
			( ConstructElement( &pData[ nOffset++ ], Forward< Ts >( args ) ), ... );
		}

		return nIndex + nCount;
	}

	constexpr I AddToHead( const T &element ) { return Insert( 0, element ); }
	constexpr I AddToHead( T &&element ) { return Insert( 0, Move( element ) ); }
	constexpr I AddToHead( ConstView_t v ) { return Insert( 0, v ); }
	template < I N > constexpr I AddToHead( const I nCount, const T ( &arrElements )[ N ] ) { return Insert( 0, ConstGrowableView_t< N >( nCount, arrElements ) ); }
	template < I N > constexpr I AddToHead( const I nCount, T ( &&arrElements )[ N ] ) { return Insert( 0, ConstGrowableView_t< N >( nCount, Move( arrElements ) ) ); }
	// template < I N > constexpr I AddToHead( const T ( &arrElements )[ N ] ) { return Insert( 0, arrElements ); }
	// template < I N > constexpr I AddToHead( T ( &&arrElements )[ N ] ) { return Insert( 0, Move( arrElements ) ); }
	template < typename ...Ts > constexpr I AddMultipleToHead( Ts &&...args ) { return InsertMultiple( 0, Forward< Ts >( args )... ); }

	constexpr I AddToTail( const T &element ) { return Insert( Count(), element ); }
	constexpr I AddToTail( T &&element ) { return Insert( Count(), Move( element ) ); }
	constexpr I AddToTail( ConstView_t v ) { return Insert( Count(), v ); }
	template < I N > constexpr I AddToTail( GrowableView_t< N > v ) { return Insert( Count(), v ); }
	template < I N > constexpr I AddToTail( ConstGrowableView_t< N > v ) { return Insert( Count(), v ); }
	template < I N > constexpr I AddToTail( const I nCount, const T ( &arrElements )[ N ] ) { return Insert( Count(), ConstGrowableView_t< N >( nCount, arrElements ) ); }
	template < I N > constexpr I AddToTail( const I nCount, T ( &&arrElements )[ N ] ) { return Insert( Count(), ConstGrowableView_t< N >( nCount, Move( arrElements ) ) ); }
	// template < I N > constexpr I AddToTail( const T ( &arrElements )[ N ] ) { return Insert( Count(), arrElements ); }
	// template < I N > constexpr I AddToTail( T ( &&arrElements )[ N ] ) { return Insert( Count(), Move( arrElements ) ); }
	template < typename ...Ts > constexpr I AddMultipleToTail( Ts &&...args ) { return InsertMultiple( Count(), Forward< Ts >( args )... ); }

	constexpr I Remove( const I nIndex, const I n = 1 )
	{
		I nCount = Count();

		BALL_ASSERT( 0 < nCount );
		BALL_ASSERT( 0 <= nIndex && nIndex + n <= nCount );

		if constexpr ( IS_PACKED_STORAGE )
		{
			const I nTailCount = nCount - ( nIndex + n );

			if ( nTailCount > I( 0 ) )
				PackedShiftRowsLeft( nIndex, nTailCount, n );
			else
				PackedClearRows( nIndex, n );

			SetCount( nCount - n );
		}
		else
		{
			T *pData = Base();
			T *pIt = &pData[ nIndex ], *pItEnd = pIt + n;

			DestructElements( pIt, pItEnd );
			ShiftElements( pIt, pItEnd, &pData[ nCount ] );
			Set( nCount - n, pData );
		}

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
		if constexpr ( IS_PACKED_STORAGE )
		{
			const I nWith = svRepl.Count();
			Remove( i, nRemove );

			if ( nWith > I( 0 ) )
				Insert( i, svRepl );

			return ( nWith == I( 0 ) )
				? ( ( i == I( 0 ) ) ? INVALID_INDEX : ( i - I( 1 ) ) )
				: ( i + nWith - I( 1 ) );
		}

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

			ShiftElementsLeft( &pData[ i + nWith ], &pData[ nSuffixBegin ], &pData[ nOldCount ] );

			// 2.3 Commit new logical size.
			Set( nCount - nShrink, pData );

			return ( nWith == I( 0 ) ) ? ( ( i == I( 0 ) ) ? INVALID_INDEX : i )
			                           : ( i + nWith );
		}

		// Case 3: grow (nWith > nRemove) -> ensure capacity, shift suffix right, then copy.
		{
			const I nGrow = nDelta; // amount to push right
			const I nNew  = nCount + nGrow;

			// 3.1 Ensure capacity; pointer may change.
			pData = EnsureCapacity( nNew );

			// 3.2 Shift suffix right by nGrow: [i + nRemove .. n) -> starts at [i + nWith .. )
			const I nSuffixBegin  = i + nRemove;
			const I nOldCount     = nCount;

			ShiftElementsRight( &pData[ i + nWith ], &pData[ nSuffixBegin ], &pData[ nOldCount ] );

			// 3.3 Commit new logical size before filling the gap.
			Set( nNew, pData );

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
		const I nCount = Count();

		I n = 0;

		for ( I k = 0; k < nCount; ++k )
		{
			const T nValue = Base_t::GetValue( k );

			if ( nValue == from )
			{
				Base_t::SetValue( k, to );
				++n;
			}
		}

		return n;
	}

	/// @brief Replace [index, index + len) with src (shifts tail if needed).
	constexpr I ReplaceRange( const I nIndex, ConstView_t src )
	{
		if constexpr ( IS_PACKED_STORAGE )
		{
			return Replace( nIndex, src.Count(), src );
		}

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
			ShiftElements( pElements + nIndex + nInsertCount, pElements + nTailStart, pElements + nCount );
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
		SetCount( I( 0 ) );
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
			if constexpr ( IS_PACKED_STORAGE )
				PackedShiftRowsRight( nIndex, nOldCount - nIndex, nAddCount );
			else
				ShiftElementsRight( &pData[ nIndex + nAddCount ], &pData[ nIndex ], &pData[ nOldCount ] );
		}

		// 3) Commit the new logical size; the gap [nIndex, nIndex + nAddCount)
		//    is now reserved for the caller to fill.
		Set( nNewCount, pData );

		return &pData[ nIndex ];
	}

	constexpr void SetCount( I nNew )
	{
		I nOld = Count();

		T *pData = EnsureCapacity( nNew );

		Set( nNew, pData );

		if constexpr ( IS_PACKED_STORAGE )
		{
			if ( nOld < nNew )
				PackedClearRows( nOld, nNew - nOld );
			else if ( nOld > nNew )
				PackedClearRows( nNew, nOld - nNew );
		}
		else
		{
			if ( nOld < nNew )
				ConstructElements( &pData[ nOld ], &pData[ nNew ] );
			else if ( nOld > nNew )
				DestructElements( &pData[ nNew ], &pData[ nOld ] );
		}
	}
}; // class CVectorImpl

template < typename I, I N, typename T, class A > class CBufferVector;

template < typename I, typename T, class A = CAllocator< I, T > >
class CVector : public CVectorImpl< CVectorBase< CView< I, T >, I, T, A >, I, T >
{
public:
	using Base_t = CVectorImpl< CVectorBase< CView< I, T >, I, T, A >, I, T >;
	using Base_t::Base_t;
	using Base_t::CopyFrom;

	template < I N > constexpr CVector( const CBufferVector< I, N, T, A > &other ) { CopyFrom< N >( other ); }
	template < I N > constexpr CVector &operator=( const CBufferVector< I, N, T, A > &other ) { return CopyFrom< N >( other ); }
};

template < typename I, I N, typename T, class A = CAllocator< I, T > >
class CBufferVector : public CVectorImpl< CVectorBase< CView< I, T, N >, I, T, A >, I, T >
{
public:
	using Base_t = CVectorImpl< CVectorBase< CView< I, T, N >, I, T, A >, I, T >;
	using Base_t::Base_t;
	using Base_t::CopyFrom;

	constexpr CBufferVector( const CVector< I, T, A > &other ) { CopyFrom( other ); }
	constexpr CBufferVector &operator=( const CVector< I, T, A > &other ) { return CopyFrom( other ); }
};

template < typename T > using Vector_t =            CVector< size_t, T >;
template < typename T > using Vector8_t =           CVector< size8_t, T >;
template < typename T > using Vector16_t =          CVector< size16_t, T >;
template < typename T > using Vector32_t =          CVector< size32_t, T >;
template < typename T > using Vector64_t =          CVector< size64_t, T >;

template < typename T, size_t N > using BufferVector_t =            CBufferVector< size_t, N, T >;
template < typename T, size8_t N > using BufferVector8_t =          CBufferVector< size8_t, N, T >;
template < typename T, size16_t N > using BufferVector16_t =        CBufferVector< size16_t, N, T >;
template < typename T, size32_t N > using BufferVector32_t =        CBufferVector< size32_t, N, T >;
template < typename T, size64_t N > using BufferVector64_t =        CBufferVector< size64_t, N, T >;

#endif // !defined( _INCLUDE_BALL_TYPES_VECTOR_HPP_ )
