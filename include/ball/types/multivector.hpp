#ifndef _INCLUDE_BALL_TYPES_MULTIVECTOR_HPP_
#	define _INCLUDE_BALL_TYPES_MULTIVECTOR_HPP_

#	pragma once

#	include "base/arch.h"
#	include "c/assert.h"
#	include "meta/fixed.hpp"
#	include "meta/get.hpp"
#	include "allocator.hpp"
#	include "bits.hpp"
#	include "elements.hpp"

#	include "viewbase.hpp"

/// @file multivector.hpp
/// @brief Growable SoA multi-column container (`CMultiVectorBase`) with shared row count.
///        Provides typed column access (`At<T>`, `Front<T>`, `Back<T>`),
///        insertion/removal operations, and `CMultiVector`/`CBufferMultiVector` aliases.

///----------------------------------------------------------------------------
/// @brief Growable SoA container on top of CViewBase<I, N, TI, Ts...>.
///        - Shared logical count for all Ts...
///        - Per-type storage: fixed inline + heap overflow
///----------------------------------------------------------------------------
template < typename I, I N, typename TI, typename... Ts >
class CMultiVectorBase : public CViewBase< I, N, TI, Ts... >, public CAllocatorBase
{
public:
	using Base_t = CViewBase< I, N, TI, Ts... >;
	using BaseAllocator_t = CAllocatorBase;
	using Index_t = I;
	using TypeIndex_t = TI;
	using Fixed_t = MFixed< Index_t >;
	using Pack_t = typename Base_t::Pack_t;
	using View_t = Base_t;
	using ConstView_t = typename Base_t::Const_t;

	using Base_t::Base_t;

	using Base_t::FIRST_INDEX;
	using Base_t::INVALID_INDEX;
	static constexpr I COMMON_FIXED_COUNT = Pack_t::COMMON_FIXED_COUNT;
	static constexpr const bool IS_GROWABLE = COMMON_FIXED_COUNT > 0;
	template < typename T > static constexpr size_t ALIGNED_SIZE = alignof( T );
	template < typename T > static constexpr bool IS_PACKED_STORAGE = Base_t::template TYPE_HAS_PACKED_BITS< T >;
	template < typename T > static constexpr size_t STORAGE_ALIGNMENT = IS_PACKED_STORAGE< T > ? alignof( uchar_t ) : alignof( RemoveCV_t< T > );

	using Base_t::Count;

	/// @brief Releases the shared overflow block, if any, and resets the view storage pointers.
	~CMultiVectorBase() noexcept
	{
		Free();
		Base_t::Set( I( 0 ), ( static_cast< Ts * >( nullptr ) )... );
	}

	// ---------------------------
	// Capacity model
	// ---------------------------
	/// @brief Returns the current geometric capacity derived from the logical row count.
	constexpr I Capacity() const noexcept { return BitCeil< I >( Count() ); }

	///-----------------------------------------------------------------------------
	/// @brief Find first row equal to (args...) starting from @p iFrom.
	/// @return Row index or INVALID_INDEX.
	///-----------------------------------------------------------------------------
	template < typename... Us, EnableIf_t< ( 1 < sizeof...( Ts ) ) && ( sizeof...( Us ) == sizeof...( Ts ) ), int > = 0 >
	constexpr I FindFrom( I iFrom, const Us &...args ) const noexcept
	{
		const I nCount = Count();

		if ( iFrom >= nCount )
			return INVALID_INDEX;

		for ( I i = iFrom; i < nCount; ++i )
		{
			if ( RowEquals< Ts... >( i, args... ) )
				return i;
		}

		return INVALID_INDEX;
	}

	///-----------------------------------------------------------------------------
	/// @brief Find first row equal to (args...) starting from head.
	/// @return Row index or INVALID_INDEX.
	///-----------------------------------------------------------------------------
	template < typename... Us, EnableIf_t< ( 1 < sizeof...( Ts ) ) && ( sizeof...( Us ) == sizeof...( Ts ) ), int > = 0 >
	constexpr I Find( const Us &...args ) const noexcept
	{
		return FindFrom( FIRST_INDEX, args... );
	}

	///-----------------------------------------------------------------------------
	/// @brief Find last row equal to (args...) searching backward from @p iFrom.
	/// @return Row index or INVALID_INDEX.
	///-----------------------------------------------------------------------------
	template < typename... Us, EnableIf_t< ( 1 < sizeof...( Ts ) ) && ( sizeof...( Us ) == sizeof...( Ts ) ), int > = 0 >
	constexpr I RFindFrom( I iFrom, const Us &...args ) const noexcept
	{
		const I nCount = Count();

		if ( nCount == FIRST_INDEX )
			return INVALID_INDEX;

		I iStart = iFrom;

		if ( iStart == INVALID_INDEX || iStart >= nCount )
			iStart = nCount - I( 1 );

		for ( I i = iStart; ; --i )
		{
			if ( RowEquals< Ts... >( i, args... ) )
				return i;

			if ( i == FIRST_INDEX )
				break;
		}

		return INVALID_INDEX;
	}

	///-----------------------------------------------------------------------------
	/// @brief Find last row equal to (args...) searching from tail.
	/// @return Row index or INVALID_INDEX.
	///-----------------------------------------------------------------------------
	template < typename... Us, EnableIf_t< ( 1 < sizeof...( Ts ) ) && ( sizeof...( Us ) == sizeof...( Ts ) ), int > = 0 >
	constexpr I RFind( const Us &...args ) const noexcept
	{
		return RFindFrom( INVALID_INDEX, args... );
	}

	// ---------------------------
	// Copy / move from views
	// ---------------------------
	/// @brief Rebuilds this container from a const view, reallocating the shared overflow block if required.
	CMultiVectorBase &CopyFrom( const ConstView_t &other )
	{
		if ( this == &other )
			return *this;

		const I nNew = other.Count();
		uint8_t *pData = EnsureCapacity( nNew );

		Base_t::Set( nNew, StorageBy< Ts >( nNew, pData )... );
		( CopyFromViewBy< Ts >( FIRST_INDEX, other ), ... );

		return *this;
	}

	/// @brief Rebuilds this container from a mutable view.
	CMultiVectorBase &CopyFrom( const View_t &other )
	{
		if ( this == &other )
			return *this;

		return CopyFrom( other.Const() );
	}

	/// @brief Rebuilds this container from another compatible CViewBase specialization.
	template< I LN > constexpr CMultiVectorBase &CopyFrom( const CViewBase< I, LN, TI, Ts... > &other )
	{
		const I nNew = other.Count();
		uint8_t *pData = EnsureCapacity( nNew );

		Base_t::Set( nNew, StorageBy< Ts >( nNew, pData )... );
		( CopyFromViewBy< Ts >( FIRST_INDEX, other.Const() ), ... );

		return *this;
	}

	/// @brief Move-assignment helper; currently implemented as a content copy.
	CMultiVectorBase &MoveFrom( View_t &&other ) noexcept
	{
		if ( this == &other )
			return *this;

		CopyFrom( other );

		return *this;
	}

	CMultiVectorBase &operator=( const ConstView_t &rhs ) { return CopyFrom( rhs ); }
	CMultiVectorBase &operator=( const View_t &rhs ) { return CopyFrom( rhs ); }

protected:
	/// @brief Returns the first per-type overflow pointer and treats it as the base of the shared overflow block.
	template < typename T0, typename... TRest >
	constexpr uint8_t *FirstDataBy() noexcept
	{
		if constexpr ( IS_PACKED_STORAGE< T0 > )
		{
			if ( Base_t::template IsPackedOverflowBy< T0 >() )
				return reinterpret_cast< uint8_t * >( Base_t::template PackedDataBy< T0 >() );
		}
		else if ( Base_t::template IsOverflowBy< T0 >() )
		{
			return reinterpret_cast< uint8_t * >( Base_t::template DataBy< T0 >() );
		}

		if constexpr ( 0 < sizeof...( TRest ) )
			return FirstDataBy< TRest... >();
		else
			return nullptr;
	}
	template < typename T0, typename... TRest > constexpr const uint8_t *FirstDataBy() const noexcept { return const_cast< CMultiVectorBase * >( this )->template FirstDataBy< T0, TRest... >(); }

	/// @brief Returns the base address of the shared overflow block, or nullptr while using inline storage.
	constexpr uint8_t *FirstData() noexcept
	{
		if constexpr ( 0 < sizeof...( Ts ) )
			return FirstDataBy< Ts... >();
		else
			return nullptr;
	}
	constexpr const uint8_t *FirstData() const noexcept { return const_cast< CMultiVectorBase * >( this )->FirstData(); }

	/// @brief Returns true when the row count no longer fits into the common inline capacity across all columns.
	static constexpr bool IsDataOverflow( I nCount ) noexcept
	{
		return nCount > COMMON_FIXED_COUNT;
	}

	/// @brief Computes the number of bytes required to store column T for @p nCount rows.
	template < typename T >
	static constexpr size_t StorageSize( I nCount ) noexcept
	{
		if constexpr ( IS_PACKED_STORAGE< T > )
			return Base_t::template PackedSizeBy< T >( nCount );
		else
			return static_cast< size_t >( nCount ) * sizeof( RemoveCV_t< T > );
	}

	/// @brief Returns the alignment requirement of the first column in a block-layout fold.
	template < typename T0 >
	static constexpr size_t BlockAlignmentBy() noexcept
	{
		return STORAGE_ALIGNMENT< T0 >;
	}

	/// @brief Returns the maximum alignment required by the participating columns.
	template < typename T0, typename T1, typename... TRest >
	static constexpr size_t BlockAlignmentBy() noexcept
	{
		const size_t nTailAlignment = BlockAlignmentBy< T1, TRest... >();

		return STORAGE_ALIGNMENT< T0 > > nTailAlignment ? STORAGE_ALIGNMENT< T0 > : nTailAlignment;
	}

	/// @brief Returns the alignment required for the complete shared overflow block.
	static constexpr size_t BlockAlignment() noexcept
	{
		if constexpr ( 0 < sizeof...( Ts ) )
			return BlockAlignmentBy< Ts... >();
		else
			return alignof( uchar_t );
	}

	/// @brief Computes the total byte size of the shared overflow block for @p nCapacity rows.
	template < typename T0, typename... TRest >
	static constexpr size_t BlockSizeBy( I nCapacity, size_t nOffset = size_t( 0 ) ) noexcept
	{
		nOffset = Math_RoundUp( nOffset, STORAGE_ALIGNMENT< T0 > );
		nOffset += StorageSize< T0 >( nCapacity );

		if constexpr ( 0 < sizeof...( TRest ) )
			return BlockSizeBy< TRest... >( nCapacity, nOffset );
		else
			return nOffset;
	}

	/// @brief Returns the total allocation size of the shared overflow block for @p nCapacity rows.
	constexpr size_t BlockSize( I nCount ) const noexcept
	{
		if constexpr ( 0 < sizeof...( Ts ) )
			return BlockSizeBy< Ts... >( nCount );
		else
			return size_t( 0 );
	}

	/// @brief Computes the byte offset of column @p TTarget within the shared overflow block.
	template < typename T, typename T0, typename... TRest >
	static constexpr size_t BlockOffsetBy( I nCapacity, size_t nOffset = size_t( 0 ) ) noexcept
	{
		nOffset = Math_RoundUp( nOffset, STORAGE_ALIGNMENT< T0 > );

		if constexpr ( IS_SAME< RemoveCV_t< T >, RemoveCV_t< T0 > > )
			return nOffset;
		else
			return BlockOffsetBy< T, TRest... >( nCapacity, nOffset + StorageSize< T0 >( nCapacity ) );
	}

	/// @brief Converts the shared overflow block base plus the column offset into a typed storage pointer.
	template < typename T >
	static constexpr T *OverflowBaseBy( uint8_t *pData, I nCapacity ) noexcept
	{
		BALL_ASSERT( pData != nullptr );

		return reinterpret_cast< T * >( pData + BlockOffsetBy< T, Ts... >( nCapacity ) );
	}

	/// @brief Returns the active storage pointer for column T, using either inline or shared overflow storage.
	template < typename T >
	constexpr T *StorageBy( I nCount, uint8_t *pData = nullptr ) noexcept
	{
		if ( IsDataOverflow( nCount ) )
			return OverflowBaseBy< T >( pData != nullptr ? pData : FirstData(), BitCeil( nCount ) );
		else if constexpr ( IS_PACKED_STORAGE< T > )
			return reinterpret_cast< T * >( Base_t::template PackedFixedDataBy< T >() );
		else
			return Base_t::template FixedDataBy< T >();
	}
	template < typename T > constexpr const T *StorageBy( I nCount, const uint8_t *pData = nullptr ) const noexcept { return const_cast< CMultiVectorBase * >( this )->template StorageBy< T >( nCount, pData ); }

	/// @brief Copies the current contents of column T into a newly allocated shared overflow block.
	template < typename T >
	constexpr void CopyToDataBy( uint8_t *pData, I nCapacity, I nCount ) const noexcept
	{
		if constexpr ( IS_PACKED_STORAGE< T > )
		{
			uchar_t *pDest = reinterpret_cast< uchar_t * >( pData + BlockOffsetBy< T, Ts... >( nCapacity ) );
			const uchar_t *pSrc = Base_t::template PackedBaseBy< T >();
			const size_t nSize = Base_t::template PackedSizeBy< T >( nCount );
			const size_t nCapacitySize = Base_t::template PackedSizeBy< T >( nCapacity );

			if ( nSize > size_t( 0 ) )
				memcpy( pDest, pSrc, nSize );

			if ( nSize < nCapacitySize )
				memset( pDest + nSize, 0, nCapacitySize - nSize );
		}
		else
		{
			T *pDest = reinterpret_cast< T * >( pData + BlockOffsetBy< T, Ts... >( nCapacity ) );
			const T *pSrc = Base_t::template BaseBy< T >();

			if ( nCount > I( 0 ) )
				CopyElements( nCount, pDest, pSrc );
		}
	}

	/// @brief Copies the current contents of column T back into its inline storage.
	template < typename T >
	constexpr void CopyToFixedBy( I nCount ) noexcept
	{
		if constexpr ( IS_PACKED_STORAGE< T > )
		{
			const size_t nBytes = Base_t::template PackedSizeBy< T >( nCount );

			if ( nBytes > size_t( 0 ) )
				memcpy( Base_t::template PackedFixedDataBy< T >(), Base_t::template PackedBaseBy< T >(), nBytes );
		}
		else if ( nCount > I( 0 ) )
		{
			CopyElements( nCount, Base_t::template FixedDataBy< T >(), Base_t::template BaseBy< T >() );
		}
	}

	/// @brief Ensures storage for @p nRequest rows and returns the base address of the active overflow block, if any.
	constexpr uint8_t *EnsureCapacity( I nRequest )
	{
		const I nCount = Count();
		const I nCopyCount = nCount < nRequest ? nCount : nRequest;
		uint8_t *pOldData = FirstData();
		const bool bWasOverflow = pOldData != nullptr;
		const bool bWillOverflow = IsDataOverflow( nRequest );

		if ( !bWillOverflow )
		{
			if ( bWasOverflow )
			{
				( CopyToFixedBy< Ts >( nCopyCount ), ... );
				BaseAllocator_t::Free( pOldData );
			}

			return nullptr;
		}

		const I nNewCapacity = BitCeil( nRequest );

		if ( bWasOverflow && nNewCapacity == Capacity() )
			return pOldData;

		uint8_t *pNewData = reinterpret_cast< uint8_t * >( BaseAllocator_t::Alloc( BlockSize( nNewCapacity ), BlockAlignment() ) );

		BALL_ASSERT_MESSAGE( pNewData != nullptr, "Failed to allocate multivector storage" );
		( CopyToDataBy< Ts >( pNewData, nNewCapacity, nCopyCount ), ... );

		if ( bWasOverflow )
			BaseAllocator_t::Free( pOldData );

		return pNewData;
	}

	/// @brief Shifts the tail of column T to the right to open an insertion gap.
	template < typename T >
	void ShiftRightBaseBy( I i, I nAdd, I nCount )
	{
		if constexpr ( IS_PACKED_STORAGE< T > )
		{
			const I nRows = static_cast< I >( nCount - i );

			if ( nRows > I( 0 ) )
				Base_t::template PackedShiftRowsRightBy< T >( i, nRows, nAdd );
		}
		else
		{
			T *p = Base_t::template BaseBy< T >();

			if ( i < nCount )
				ShiftElementsRight( &p[ i + nAdd ], &p[ i ], &p[ nCount ] );
		}
	}

	/// @brief Shifts the tail of column T to the left after row removal.
	template < typename T >
	void ShiftLeftBaseBy( I i, I nRemove, I nOld )
	{
		if constexpr ( IS_PACKED_STORAGE< T > )
		{
			const I nTailBegin = i + nRemove;

			if ( nTailBegin < nOld )
				Base_t::template PackedShiftRowsLeftBy< T >( i, static_cast< I >( nOld - nTailBegin ), nRemove );
		}
		else
		{
			T *p = Base_t::template BaseBy< T >();

			const I nTailBegin = i + nRemove;

			if ( nTailBegin < nOld )
				ShiftElementsLeft( &p[ i ], &p[ nTailBegin ], &p[ nOld ] );
		}
	}

	/// @brief Releases the shared overflow allocation if the container currently owns one.
	void Free()
	{
		uint8_t *pData = FirstData();

		if ( pData )
			BaseAllocator_t::Free( pData );
	}

	/// @brief Compares one logical row against a heterogeneous tuple of probe values.
	template < typename T0, typename... TRest, typename U0, typename... URest >
	constexpr bool RowEquals( I i, const U0 &u0, const URest &...urest ) const noexcept
	{
		if constexpr ( IS_PACKED_STORAGE< T0 > )
		{
			if ( !( Base_t::template PackedGetValueBy< T0 >( i ) == static_cast< T0 >( u0 ) ) )
				return false;
		}
		else if ( !( Base_t::template BaseBy< T0 >()[ i ] == u0 ) )
		{
			return false;
		}

		if constexpr ( sizeof...( TRest ) > 0 )
			return RowEquals< TRest... >( i, urest... );
		else
			return true;
	}
};

template < class B, typename I, typename TI, typename... Ts >
class CMultiVectorImpl : public B
{
public:
	using Base_t = B;
	using Index_t = I;
	using TypeIndex_t = TI;
	using View_t = typename Base_t::View_t;
	using ConstView_t = typename Base_t::ConstView_t;

	using Base_t::Base_t;
	template < typename T > static constexpr bool IS_PACKED_STORAGE = Base_t::template IS_PACKED_STORAGE< T >;
	using Base_t::Count;
	using Base_t::Set;

	constexpr CMultiVectorImpl() noexcept : Base_t() {}
	constexpr CMultiVectorImpl( const View_t &copyFrom ) noexcept { Base_t::CopyFrom( copyFrom ); }
	constexpr CMultiVectorImpl( const ConstView_t &copyFrom ) noexcept { Base_t::CopyFrom( copyFrom ); }
	constexpr CMultiVectorImpl( View_t &&moveFrom ) noexcept { Base_t::MoveFrom( Move( moveFrom ) ); }
	~CMultiVectorImpl() noexcept { ( DestructAllBy< Ts >(), ... ); }

	CMultiVectorImpl &operator=( const View_t &copyFrom ) { Base_t::CopyFrom( copyFrom ); return *this; }
	CMultiVectorImpl &operator=( const ConstView_t &copyFrom ) { Base_t::CopyFrom( copyFrom ); return *this; }
	CMultiVectorImpl &operator=( CMultiVectorImpl &&moveFrom ) { Base_t::MoveFrom( Move( moveFrom ) ); return *this; }

	// ---------------------------
	// Size management
	// ---------------------------
	constexpr void SetCount( I nNew )
	{
		const I nOld = Count();
		uint8_t *pData = Base_t::EnsureCapacity( nNew );

		Set( nNew, Base_t::template StorageBy< Ts >( nNew, pData )... );
		( ResizeRangeBy< Ts >( nOld, nNew ), ... );
	}

	constexpr void Grow( I delta )
	{
		const I nNew = Count() + delta;

		BALL_ASSERT( nNew >= I( 0 ) );
		SetCount( nNew );
	}

	constexpr void RemoveAll()
	{
		SetCount( I( 0 ) );
	}

	constexpr void Purge()
	{
		RemoveAll();
	}

	// ---------------------------
	// Insert / remove
	// ---------------------------
	///-----------------------------------------------------------------------------
	/// @brief Insert a multi-row view at position @p i.
	/// @return Index right after the inserted block.
	///-----------------------------------------------------------------------------
	constexpr I Insert( I i, const ConstView_t &v )
	{
		const I nAdd = v.Count();

		BALL_ASSERT( nAdd > I( 0 ) );
		EnsureInsert( i, nAdd );
		( CopyFromViewBy< Ts >( i, v ), ... );

		return i + nAdd;
	}

	constexpr I Insert( I i, const View_t &v )
	{
		return Insert( i, v.Const() );
	}

	///-----------------------------------------------------------------------------
	/// @brief Insert a single row at @p i (one value per column type).
	/// @return Index right after the inserted row.
	///-----------------------------------------------------------------------------
	template < typename... Us >
	constexpr I Insert( I i, Us &&...args )
	{
		static_assert( sizeof...( Us ) == sizeof...( Ts ), "Insert requires exactly one value per type" );

		EnsureInsert( i, I( 1 ) );
		AssignRow< Ts... >( i, Forward< Us >( args )... );

		return i + I( 1 );
	}

	///-----------------------------------------------------------------------------
	/// @brief Insert @p nCount identical rows at @p i.
	/// @return Index right after the inserted block.
	///-----------------------------------------------------------------------------
	template < typename... Us >
	constexpr I InsertMultiple( I i, I nCount, const Us &...args )
	{
		static_assert( sizeof...( Us ) == sizeof...( Ts ), "InsertMultiple requires exactly one value per type" );

		BALL_ASSERT( nCount > I( 0 ) );
		EnsureInsert( i, nCount );

		for ( I n = I( 0 ); n < nCount; ++n )
			AssignRow< Ts... >( i + n, args... );

		return i + nCount;
	}

	constexpr I AddToHead( const ConstView_t &v ) { return Insert( I( 0 ), v ); }
	constexpr I AddToHead( const View_t &v ) { return Insert( I( 0 ), v ); }
	template < typename... Us > constexpr I AddToHead( Us &&...args ) { return Insert( I( 0 ), Forward< Us >( args )... ); }
	template < typename... Us > constexpr I AddMultipleToHead( I nCount, const Us &...args ) { return InsertMultiple( I( 0 ), nCount, args... ); }

	constexpr I AddToTail( const ConstView_t &v ) { return Insert( Count(), v ); }
	constexpr I AddToTail( const View_t &v ) { return Insert( Count(), v ); }
	template < typename... Us > constexpr I AddToTail( Us &&...args ) { return Insert( Count(), Forward< Us >( args )... ); }
	template < typename... Us > constexpr I AddMultipleToTail( I nCount, const Us &...args ) { return InsertMultiple( Count(), nCount, args... ); }

	void Remove( I i, I nRemove = I( 1 ) )
	{
		BALL_ASSERT( nRemove > I( 0 ) );
		BALL_ASSERT( I( 0 ) <= i && i + nRemove <= Count() );

		const I nCount = Count();
		const I nNewCount = nCount - nRemove;

		( DestructRangeBy< Ts >( i, nRemove ), ... );
		( Base_t::template ShiftLeftBaseBy< Ts >( i, nRemove, nCount ), ... );

		uint8_t *pData = Base_t::EnsureCapacity( nNewCount );

		Set( nNewCount, Base_t::template StorageBy< Ts >( nNewCount, pData )... );
	}

protected:
	///-----------------------------------------------------------------------------
	/// @brief Reserve and open a gap of @p nAdd rows at index @p i.
	///-----------------------------------------------------------------------------
	void EnsureInsert( I i, I nAdd )
	{
		BALL_ASSERT( nAdd > I( 0 ) );
		BALL_ASSERT( I( 0 ) <= i && i <= Count() );

		const I nOld = Count();
		const I nNew = nOld + nAdd;
		uint8_t *pData = Base_t::EnsureCapacity( nNew );

		Set( nNew, Base_t::template StorageBy< Ts >( nNew, pData )... );

		if ( i < nOld )
			( Base_t::template ShiftRightBaseBy< Ts >( i, nAdd, nOld ), ... );
	}

private:
	template < typename T >
	constexpr void ResizeRangeBy( I nOld, I nNew )
	{
		T *pData = Base_t::template BaseBy< T >();

		if ( !pData )
			return;

		if constexpr ( IS_PACKED_STORAGE< T > )
		{
			if ( nOld < nNew )
				Base_t::template PackedClearRowsBy< T >( nOld, nNew - nOld );
			else if ( nOld > nNew )
				Base_t::template PackedClearRowsBy< T >( nNew, nOld - nNew );

			return;
		}

		if ( nOld < nNew )
			ConstructElements( &pData[ nOld ], &pData[ nNew ] );
		else if ( nOld > nNew )
			DestructElements( &pData[ nNew ], &pData[ nOld ] );
	}

	template < typename T >
	constexpr void DestructRangeBy( I i, I nCount )
	{
		if constexpr ( IS_PACKED_STORAGE< T > )
			return;

		T *pData = Base_t::template BaseBy< T >();

		if ( !pData )
			return;

		DestructElements( &pData[ i ], &pData[ i + nCount ] );
	}

	template < typename T >
	constexpr void DestructAllBy()
	{
		if constexpr ( IS_PACKED_STORAGE< T > )
			return;

		T *pData = Base_t::template BaseBy< T >();

		DestructElements( pData, pData + Count() );
	}

	template < typename T >
	constexpr void CopyFromViewBy( I i, const ConstView_t &v )
	{
		const I nAdd = v.Count();

		if constexpr ( IS_PACKED_STORAGE< T > )
		{
			for ( I n = I( 0 ); n < nAdd; ++n )
				Base_t::template PackedSetValueBy< T >( static_cast< I >( i + n ), v.template PackedGetValueBy< T >( n ) );
		}
		else
		{
			CopyElements( nAdd, Base_t::template BaseBy< T >() + i, v.template BaseBy< T >() );
		}
	}

	template < typename T0, typename... TRest, typename U0, typename... URest >
	constexpr void AssignRow( I i, U0 &&u0, URest &&...urest )
	{
		if constexpr ( IS_PACKED_STORAGE< T0 > )
			Base_t::template PackedSetValueBy< T0 >( i, static_cast< T0 >( Forward< U0 >( u0 ) ) );
		else
			Base_t::template BaseBy< T0 >()[ i ] = Forward< U0 >( u0 );

		if constexpr ( sizeof...( TRest ) > 0 )
			AssignRow< TRest... >( i, Forward< URest >( urest )... );
	}
};

template < typename I, I N, typename TI, typename... Ts > class CBufferMultiVector;

template < typename I, typename TI, typename... Ts >
class CMultiVector : public CMultiVectorImpl< CMultiVectorBase< I, 0, TI, Ts... >, I, TI, Ts... >
{
public:
	using Base_t = CMultiVectorImpl< CMultiVectorBase< I, 0, TI, Ts... >, I, TI, Ts... >;
	using Base_t::Base_t;
	using Base_t::CopyFrom;

	template < I N > constexpr CMultiVector( const CBufferMultiVector< I, N, TI, Ts... > &other ) { CopyFrom( other ); }
	template < I N > constexpr CMultiVector &operator=( const CBufferMultiVector< I, N, TI, Ts... > &other ) { return CopyFrom( other ); }
};

template < typename I, I N, typename TI, typename... Ts >
class CBufferMultiVector : public CMultiVectorImpl< CMultiVectorBase< I, N, TI, Ts... >, I, TI, Ts... >
{
public:
	using Base_t = CMultiVectorImpl< CMultiVectorBase< I, N, TI, Ts... >, I, TI, Ts... >;
	using Base_t::Base_t;
	using Base_t::CopyFrom;

	constexpr CBufferMultiVector( const CMultiVector< I, TI, Ts... > &other ) { CopyFrom( other ); }
	constexpr CBufferMultiVector &operator=( const CMultiVector< I, TI, Ts... > &other ) { return CopyFrom( other ); }
};

template < typename... Ts > using MultiVector_t =            CMultiVector< size_t, size8_t, Ts... >;
template < typename... Ts > using MultiVector8_t =           CMultiVector< size8_t, size8_t, Ts... >;
template < typename... Ts > using MultiVector16_t =          CMultiVector< size16_t, size8_t, Ts... >;
template < typename... Ts > using MultiVector32_t =          CMultiVector< size32_t, size8_t, Ts... >;
template < typename... Ts > using MultiVector64_t =          CMultiVector< size64_t, size8_t, Ts... >;

template < size_t N, typename... Ts > using BufferMultiVector_t =            CBufferMultiVector< size_t, N, size8_t, Ts... >;
template < size8_t N, typename... Ts > using BufferMultiVector8_t =          CBufferMultiVector< size8_t, N, size8_t, Ts... >;
template < size16_t N, typename... Ts > using BufferMultiVector16_t =        CBufferMultiVector< size16_t, N, size8_t, Ts... >;
template < size32_t N, typename... Ts > using BufferMultiVector32_t =        CBufferMultiVector< size32_t, N, size8_t, Ts... >;
template < size64_t N, typename... Ts > using BufferMultiVector64_t =        CBufferMultiVector< size64_t, N, size8_t, Ts... >;

#endif // !defined( _INCLUDE_BALL_TYPES_MULTIVECTOR_HPP_ )
