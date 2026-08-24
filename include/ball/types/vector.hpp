#ifndef _INCLUDE_BALL_TYPES_VECTOR_HPP_
#	define _INCLUDE_BALL_TYPES_VECTOR_HPP_

#	pragma once

#	include "base/arch.h"
#	include "c/assert.h"
#	include "fixed.hpp"
#	include "meta/fixed.hpp"
#	include "meta/first.hpp"
#	include "meta/get.hpp"
#	include "meta/indexsequence.hpp"
#	include "meta/isintegral.hpp"
#	include "meta/conditional.hpp"
#	include "meta/typeinfo.hpp"
#	include "meta/vectorallocatortype.hpp"
#	include "meta/vectorviewtypes.hpp"
#	include "meta/xvalue.hpp"
#	include "allocator.hpp"
#	include "bits.hpp"
#	include "elements.hpp"
#	include "memory.h"
#	include "math.hpp"
#	include "view.hpp"
#	include "viewbase.hpp"
#	include "vectoriterator.hpp"

template < class A, class B, typename I, I N, typename TI, typename... Ts >
class CVectorBase : public A, public B
{
public:
	using Allocator_t = A;
	using BaseAllocator_t = typename MVectorAllocatorType< Allocator_t >::Type;
	using Base_t = B;
	using Pack_t = typename Base_t::Pack_t;
	using Index_t = I;
	using TypeIndex_t = TI;
	using Fixed_t = MFixed< Index_t >;
	using Unsigned_t = typename Fixed_t::Unsigned_t;
	using FirstColumn_t = typename MFirst< Ts... >::Type;
	using ViewTypes_t = MVectorViewTypes< Base_t, I, N, FirstColumn_t >;
	using Base_t::TYPE_COUNT;
	using View_t = Conditional_t< TYPE_COUNT == 1, typename ViewTypes_t::View_t, Base_t >;
	using ConstView_t = Conditional_t< TYPE_COUNT == 1, typename ViewTypes_t::ConstView_t, typename Base_t::Const_t >;
	template < I GN > using GrowableView_t = CView< I, typename MFirst< Ts... >::Type, GN >;
	template < I GN > using ConstGrowableView_t = CView< I, const typename MFirst< Ts... >::Type, GN >;

	template < typename T = FirstColumn_t > using Iterator_t = Conditional_t< IS_PACKED_STORAGE_BY< Base_t, T >, CVector_Packed_Iterator< I, T, false >, T * >;
	template < typename T = FirstColumn_t > using ConstIterator_t = Conditional_t< IS_PACKED_STORAGE_BY< Base_t, T >, CVector_Packed_Iterator< I, T, true >, const T * >;
	using iterator = Iterator_t<>;
	using const_iterator = ConstIterator_t<>;

	using Base_t::Base_t;

	using Base_t::FIRST_INDEX;
	using Base_t::INVALID_INDEX;
	using Base_t::FIND_BATCH_COUNT;
	using Base_t::FIND_BATCH_LAST;
	using Base_t::COMMON_FIXED_COUNT;
	using Base_t::IS_GROWABLE;
	using Base_t::ALIGNED_SIZE;
	template < typename T > static constexpr bool IS_PACKED_STORAGE = IS_PACKED_STORAGE_BY< Base_t, T >;
	using Base_t::STORAGE_ALIGNMENT;
	using Base_t::Set;
	using Base_t::Self;
	using Base_t::Count;
	using Base_t::Size;
	using Base_t::FixedCount;
	using Base_t::FixedCapacity;
	using Base_t::FixedSize;
	using Base_t::FixedCapacitySize;
	using Base_t::IsOverflow;
	using Base_t::Packed_IsOverflow;
	using Base_t::FixedData;
	using Base_t::Data;
	using Base_t::Base;
	using Base_t::Packed_Data;
	using Base_t::Packed_FixedData;
	using Base_t::Packed_Base;
	using Base_t::Packed_SizeBy;
	using Base_t::Packed_Set;
	using Base_t::Packed_ShiftRowsLeftBy;
	using Base_t::Packed_ShiftRowsRightBy;
	using Base_t::Packed_ClearRows;
	using Base_t::Find;
	using Base_t::FindFrom;
	using Base_t::RFind;
	using Base_t::RFindFrom;
	using Base_t::Get;
	using Base_t::SetTo;
	using Base_t::operator[];
	using Base_t::BaseBy;
	using Base_t::DataBy;
	using Base_t::FixedDataBy;
	using Base_t::IsOverflowBy;
	using Base_t::Packed_IsOverflowBy;
	using Base_t::Packed_BaseBy;
	using Base_t::Packed_DataBy;
	using Base_t::Packed_FixedDataBy;
	using Base_t::Packed_GetBy;
	template < typename T = FirstColumn_t > constexpr Iterator_t< T > begin() noexcept { return GetBegin< T, Iterator_t< T > >( Self() ); }
	template < typename T = FirstColumn_t > constexpr Iterator_t< T > end() noexcept { return GetEnd< T, Iterator_t< T > >( Self() ); }
	template < typename T = FirstColumn_t > constexpr ConstIterator_t< T > begin() const noexcept { return GetBegin< T, ConstIterator_t< T > >( Self() ); }
	template < typename T = FirstColumn_t > constexpr ConstIterator_t< T > end() const noexcept { return GetEnd< T, ConstIterator_t< T > >( Self() ); }
	template < typename T = FirstColumn_t > constexpr ConstIterator_t< T > cbegin() const noexcept { return begin< T >(); }
	template < typename T = FirstColumn_t > constexpr ConstIterator_t< T > cend() const noexcept { return end< T >(); }

	/// @brief Releases the shared overflow block, if any, and resets the view storage pointers.
	~CVectorBase() noexcept
	{
		Free();
		Set( 0, ( static_cast< Ts * >( nullptr ) )... );
	}

	// ---------------------------
	// Capacity model
	// ---------------------------
	/// @brief Returns the current geometric capacity derived from the logical row count.
	constexpr I Capacity() const noexcept { return BitCeil< I >( Count() ); }

	// ---------------------------
	// Copy / move from views
	// ---------------------------
	/// @brief Rebuilds this container from a const view, reallocating the shared overflow block if required.
	CVectorBase &CopyFrom( const ConstView_t &other )
	{
		if ( this == &other )
			return *this;

		const I nNew = other.Count();
		uint8_t *pData = EnsureCapacity( nNew );

		Set( nNew, StorageBy< Ts >( nNew, pData )... );
		( CopyFromViewBy< Ts >( FIRST_INDEX, other ), ... );

		return *this;
	}

	/// @brief Rebuilds this container from a mutable view.
	CVectorBase &CopyFrom( const View_t &other )
	{
		if ( this == &other )
			return *this;

		return CopyFrom( other.Const() );
	}

	/// @brief Rebuilds this container from another compatible CViewBase specialization.
	template< I LN > constexpr CVectorBase &CopyFrom( const CViewBase< I, LN, TI, Ts... > &other )
	{
		const I nNew = other.Count();
		uint8_t *pData = EnsureCapacity( nNew );

		Set( nNew, StorageBy< Ts >( nNew, pData )... );
		( CopyFromViewBy< Ts >( FIRST_INDEX, other.Const() ), ... );

		return *this;
	}

	/// @brief Move-assignment helper; currently implemented as a content copy.
	CVectorBase &MoveFrom( View_t &&other ) noexcept
	{
		if ( this == &other )
			return *this;

		CopyFrom( other );

		return *this;
	}

	CVectorBase &operator=( const ConstView_t &rhs ) { return CopyFrom( rhs ); }
	CVectorBase &operator=( const View_t &rhs ) { return CopyFrom( rhs ); }

protected:
	template < typename T > static constexpr bool Packed_IsStorage() noexcept { return IS_PACKED_STORAGE_BY< Base_t, T >; }
	template < typename T > static constexpr size_t StorageAlignment() noexcept { return STORAGE_ALIGNMENT_BY< Base_t, T >; }

	/// @brief Returns the first per-type overflow pointer and treats it as the base of the shared overflow block.
	template < typename T0, typename... TRest >
	constexpr uint8_t *FirstDataBy() noexcept
	{
		if constexpr ( Packed_IsStorage< T0 >() )
		{
			if ( Packed_IsColumnOverflow< T0 >( Self() ) )
				return reinterpret_cast< uint8_t * >( Packed_GetData< T0 >( Self() ) );
		}
		else if ( IsColumnOverflow< T0 >( Self() ) )
		{
			return reinterpret_cast< uint8_t * >( GetData< T0 >( Self() ) );
		}

		if constexpr ( 0 < sizeof...( TRest ) )
			return FirstDataBy< TRest... >();
		else
			return nullptr;
	}
	template < typename T0, typename... TRest > constexpr const uint8_t *FirstDataBy() const noexcept { return const_cast< CVectorBase * >( this )->template FirstDataBy< T0, TRest... >(); }

	/// @brief Returns the base address of the shared overflow block, or nullptr while using inline storage.
	constexpr uint8_t *FirstData() noexcept
	{
		if constexpr ( 0 < TYPE_COUNT )
			return FirstDataBy< Ts... >();
		else
			return nullptr;
	}
	constexpr const uint8_t *FirstData() const noexcept { return const_cast< CVectorBase * >( this )->FirstData(); }

	/// @brief Returns true when the row count no longer fits into the common inline capacity across all columns.
	static constexpr bool IsDataOverflow( I nCount ) noexcept
	{
		return nCount > COMMON_FIXED_COUNT;
	}

	/// @brief Computes the number of bytes required to store column T for @p nCount rows.
	template < typename T >
	static constexpr size_t StorageSize( I nCount ) noexcept
	{
		if constexpr ( Packed_IsStorage< T >() )
			return CVectorBase::template Packed_Size< T >( nCount );
		else
			return static_cast< size_t >( nCount ) * sizeof( RemoveCV_t< T > );
	}

	/// @brief Returns the alignment requirement of the first column in a block-layout fold.
	template < typename T0 >
	static constexpr size_t BlockAlignmentBy() noexcept
	{
		return StorageAlignment< T0 >();
	}

	/// @brief Returns the maximum alignment required by the participating columns.
	template < typename T0, typename T1, typename... TRest >
	static constexpr size_t BlockAlignmentBy() noexcept
	{
		const size_t nTailAlignment = BlockAlignmentBy< T1, TRest... >();

		return StorageAlignment< T0 >() > nTailAlignment ? StorageAlignment< T0 >() : nTailAlignment;
	}

	/// @brief Returns the alignment required for the complete shared overflow block.
	static constexpr size_t BlockAlignment() noexcept
	{
		if constexpr ( 0 < TYPE_COUNT )
			return BlockAlignmentBy< Ts... >();
		else
			return alignof( uchar_t );
	}

	/// @brief Computes the total byte size of the shared overflow block for @p nCapacity rows.
	template < typename T0, typename... TRest >
	static constexpr size_t BlockSizeBy( I nCapacity, size_t nOffset = 0 ) noexcept
	{
		nOffset = Math_RoundUp( nOffset, StorageAlignment< T0 >() );
		nOffset += StorageSize< T0 >( nCapacity );

		if constexpr ( 0 < sizeof...( TRest ) )
			return BlockSizeBy< TRest... >( nCapacity, nOffset );
		else
			return nOffset;
	}

	/// @brief Returns the total allocation size of the shared overflow block for @p nCapacity rows.
	constexpr size_t BlockSize( I nCount ) const noexcept
	{
		if constexpr ( 0 < TYPE_COUNT )
			return BlockSizeBy< Ts... >( nCount );
		else
			return 0;
	}

	/// @brief Computes the byte offset of column @p TTarget within the shared overflow block.
	template < typename T, typename T0, typename... TRest >
	static constexpr size_t BlockOffsetBy( I nCapacity, size_t nOffset = 0 ) noexcept
	{
		nOffset = Math_RoundUp( nOffset, StorageAlignment< T0 >() );

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

public:
	/// @brief Returns the active storage pointer for column T from the container's current storage.
	template < typename T >
	constexpr T *StorageBy( I nCount ) noexcept
	{
		if ( IsDataOverflow( nCount ) )
			return OverflowBaseBy< T >( FirstData(), BitCeil( nCount ) );
		else if constexpr ( Packed_IsStorage< T >() )
			return reinterpret_cast< T * >( Packed_GetFixedData< T >( Self() ) );
		else
			return GetFixedData< T >( Self() );
	}

	/// @brief Returns storage for column T using the explicitly supplied overflow block when required.
	template < typename T >
	constexpr T *StorageBy( I nCount, uint8_t *pData ) noexcept
	{
		if ( IsDataOverflow( nCount ) )
			return OverflowBaseBy< T >( pData, BitCeil( nCount ) );

		return StorageBy< T >( nCount );
	}

	template < typename T > constexpr const T *StorageBy( I nCount ) const noexcept { return const_cast< CVectorBase * >( this )->template StorageBy< T >( nCount ); }
	template < typename T > constexpr const T *StorageBy( I nCount, const uint8_t *pData ) const noexcept { return const_cast< CVectorBase * >( this )->template StorageBy< T >( nCount, pData ); }

protected:

	/// @brief Copies the current contents of column T into a newly allocated shared overflow block.
	template < typename T >
	constexpr void CopyToDataBy( uint8_t *pData, I nCapacity, I nCount ) noexcept
	{
		if constexpr ( Packed_IsStorage< T >() )
		{
			uchar_t *pDest = reinterpret_cast< uchar_t * >( pData + BlockOffsetBy< T, Ts... >( nCapacity ) );
			const uchar_t *pSrc = Packed_Get< T >( Self() );
			const size_t nSize = Packed_GetSize< T >( Self(), nCount );
			const size_t nCapacitySize = Packed_GetSize< T >( Self(), nCapacity );

			if ( nSize > 0 )
				memcpy( pDest, pSrc, nSize );

			if ( nSize < nCapacitySize )
				memset( pDest + nSize, 0, nCapacitySize - nSize );
		}
		else
		{
			T *pDest = reinterpret_cast< T * >( pData + BlockOffsetBy< T, Ts... >( nCapacity ) );
			T *pSrc = GetBase< T >( Self() );

			if ( nCount > 0 )
				RelocateColumn( pDest, pSrc, nCount );
		}
	}

	/// @brief Copies the current contents of column T back into its inline storage.
	template < typename T >
	constexpr void CopyToFixedBy( I nCount ) noexcept
	{
		if constexpr ( Packed_IsStorage< T >() )
		{
			const size_t nBytes = Packed_GetSize< T >( Self(), nCount );

			if ( nBytes > 0 )
				memcpy( Packed_GetFixedData< T >( Self() ), Packed_Get< T >( Self() ), nBytes );
		}
		else if ( nCount > 0 )
		{
			RelocateColumn( GetFixedData< T >( Self() ), GetBase< T >( Self() ), nCount );
		}
	}

	///-----------------------------------------------------------------------------
	/// @brief Relocate @p nCount elements of a non-packed column from @p pSrc into
	/// uninitialized storage at @p pDest (disjoint ranges).
	/// @details Trivially-copyable types are byte-copied; otherwise each element is
	/// move-constructed into the destination and the source is destroyed, which is
	/// required for types whose move/copy is not equivalent to a raw byte copy
	/// (e.g. a delegate that re-homes its payload on move).
	///-----------------------------------------------------------------------------
	template < typename T >
	static constexpr void RelocateColumn( T *pDest, T *pSrc, I nCount ) noexcept
	{
		if constexpr ( MTypeInfo< T >::IS_MEMMOVE_SAFE )
		{
			CopyElements( nCount, pDest, pSrc );
		}
		else
		{
			for ( I n = 0; n < nCount; ++n )
			{
				ConstructElement( &pDest[ n ], Move( pSrc[ n ] ) );
				DestructElement( &pSrc[ n ] );
			}
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

		// A byte-relocatable single column keeps the same block layout at every
		// capacity. Let the allocator extend its reserved region in place when
		// possible instead of allocating and copying the complete live range.
		if constexpr ( TYPE_COUNT == 1 && ( Packed_IsStorage< FirstColumn_t >() || IS_MEMMOVE_SAFE< FirstColumn_t > ) )
		{
			if ( bWasOverflow )
			{
				const I nOldCapacity = Capacity();
				uint8_t *pNewData = reinterpret_cast< uint8_t * >( BaseAllocator_t::Realloc( pOldData, BlockSize( nNewCapacity ), BlockAlignment() ) );

				BALL_ASSERT_MESSAGE( pNewData != nullptr, "Failed to reallocate vector storage" );

				if constexpr ( Packed_IsStorage< FirstColumn_t >() )
				{
					const size_t nOldSize = StorageSize< FirstColumn_t >( nOldCapacity );
					const size_t nNewSize = StorageSize< FirstColumn_t >( nNewCapacity );

					if ( nOldSize < nNewSize )
						memset( pNewData + nOldSize, 0, nNewSize - nOldSize );
				}

				return pNewData;
			}
		}

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
		if constexpr ( Packed_IsStorage< T >() )
		{
			const I nRows = static_cast< I >( nCount - i );

			if ( nRows > 0 )
				Packed_ShiftRight< T >( Self(), i, nRows, nAdd );
		}
		else
		{
			T *p = GetBase< T >( Self() );

			if ( i < nCount )
			{
				if constexpr ( MTypeInfo< T >::IS_MEMMOVE_SAFE )
				{
					ShiftElementsRight( &p[ i + nAdd ], &p[ i ], &p[ nCount ] );
				}
				else
				{
					// Relocate the tail backwards into raw slots, leaving the gap
					// [i, i + nAdd) destroyed for the caller to construct into.
					for ( I n = nCount; n-- > i; )
					{
						ConstructElement( &p[ n + nAdd ], Move( p[ n ] ) );
						DestructElement( &p[ n ] );
					}
				}
			}
		}
	}

	/// @brief Shifts the tail of column T to the left after row removal.
	template < typename T >
	void ShiftLeftBaseBy( I i, I nRemove, I nOld )
	{
		if constexpr ( Packed_IsStorage< T >() )
		{
			const I nTailBegin = i + nRemove;

			if ( nTailBegin < nOld )
				Packed_ShiftLeft< T >( Self(), i, static_cast< I >( nOld - nTailBegin ), nRemove );
		}
		else
		{
			T *p = GetBase< T >( Self() );

			const I nTailBegin = i + nRemove;

			if ( nTailBegin < nOld )
			{
				if constexpr ( MTypeInfo< T >::IS_MEMMOVE_SAFE )
				{
					ShiftElementsLeft( &p[ i ], &p[ nTailBegin ], &p[ nOld ] );
				}
				else
				{
					// The removed range [i, nTailBegin) is already destroyed; relocate
					// the surviving tail forwards into those raw slots.
					const I nTail = nOld - nTailBegin;

					for ( I n = 0; n < nTail; ++n )
					{
						ConstructElement( &p[ i + n ], Move( p[ nTailBegin + n ] ) );
						DestructElement( &p[ nTailBegin + n ] );
					}
				}
			}
		}
	}

	/// @brief Releases the shared overflow allocation if the container currently owns one.
	void Free()
	{
		uint8_t *pData = FirstData();

		if ( pData )
			BaseAllocator_t::Free( pData );
	}

};

template < class B, typename I, typename TI, typename... Ts >
class CVectorImpl;

template < class B, typename I, typename TI, typename T >
class CVectorImpl< B, I, TI, T > : public B
{
public:
	using Base_t = B;
	using typename Base_t::Index_t;
	using Unsigned_t = typename MFixed< I >::Unsigned_t;
	using typename Base_t::View_t;
	using typename Base_t::ConstView_t;
	template < I GN > using GrowableView_t = GrowableViewBy_t< Base_t, GN >;
	template < I GN > using ConstGrowableView_t = ConstGrowableViewBy_t< Base_t, GN >;

	using Base_t::INVALID_INDEX;
	using Base_t::TYPE_COUNT;
	static constexpr bool IS_PACKED_STORAGE = IS_PACKED_STORAGE_BY< Base_t, T >;
	using Base_t::Count;
	using Base_t::Size;
	using Base_t::Base;
	using Base_t::Find;
	using Base_t::MoveFrom;
	using Base_t::Packed_Base;
	using Base_t::Packed_SizeBy;
	using Base_t::Packed_Set;
	using Base_t::Packed_ShiftRowsLeftBy;
	using Base_t::Packed_ShiftRowsRightBy;
	using Base_t::Packed_ClearRows;
	using Base_t::begin;
	using Base_t::end;
	using Base_t::cbegin;
	using Base_t::cend;

protected:
	template < class V > constexpr CVectorImpl &CopyFromView( const V &other )
	{
		const I nNewCount = other.Count();
		T *pElements = EnsureCapacity( nNewCount );

		CopyElements( nNewCount, pElements, other.Base() );
		Set( nNewCount, pElements );

		return *this;
	}

public:
	constexpr CVectorImpl &CopyFrom( const ConstView_t &other ) { return CopyFromView( other ); }
	constexpr CVectorImpl &CopyFrom( const View_t &other ) { return CopyFrom( other.Const() ); }
	template < I N > constexpr CVectorImpl &CopyFrom( const GrowableView_t< N > &other ) { return CopyFrom( other.Const() ); }
	template < I N > constexpr CVectorImpl &CopyFrom( const ConstGrowableView_t< N > &other ) { return CopyFromView( other ); }

	constexpr CVectorImpl() noexcept : Base_t() {}
	constexpr CVectorImpl( const View_t &other ) noexcept { CopyFrom( other ); }
	constexpr CVectorImpl( const ConstView_t &other ) noexcept { CopyFrom( other ); }
	template < I N > constexpr CVectorImpl( const GrowableView_t< N > &other ) noexcept { CopyFrom( other ); }
	template < I N > constexpr CVectorImpl( const ConstGrowableView_t< N > &other ) noexcept { CopyFrom( other ); }
	constexpr CVectorImpl( View_t &&other ) noexcept { MoveFrom( Move( other ) ); }
	template < I N > constexpr CVectorImpl( const T ( &elements )[ N ] ) noexcept : Base_t() { AddToTail( I( N ), elements ); }
	template < I N > constexpr CVectorImpl( T ( &&elements )[ N ] ) noexcept : Base_t() { AddToTail( I( N ), Move( elements ) ); }
	constexpr CVectorImpl( T &element ) noexcept : Base_t() { AddToTail( element ); }
	constexpr ~CVectorImpl() noexcept
	{
		T *pData = Base();

		DestructElements( pData, pData + Count() );
	}

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
				Packed_Set( nIndex + n, static_cast< T >( v.Get( n ) ) );
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
				Packed_Set( nIndex + n, static_cast< T >( v.Get( n ) ) );
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
		BALL_STATIC_ASSERT( 0 < N, "InsertElements( I, const I, const & ) requires at least one element" );
		BALL_ASSERT( 0 < nCount );
		BALL_ASSERT( nCount <= N );

		T *pData = EnsureInsert( nIndex, nCount );

		if constexpr ( IS_PACKED_STORAGE )
		{
			for ( I n = 0; n < nCount; ++n )
				Packed_Set( nIndex + n, arrElements[ n ] );
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
		BALL_STATIC_ASSERT( 0 < N, "InsertElements( I, const I, && ) requires at least one element" );
		BALL_ASSERT( 0 < nCount );
		BALL_ASSERT( nCount <= N );

		T *pData = EnsureInsert( nIndex, nCount );

		if constexpr ( IS_PACKED_STORAGE )
		{
			for ( I n = 0; n < nCount; ++n )
				Packed_Set( nIndex + n, Move( arrElements[ n ] ) );
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
	/// @details Byte-relocatable, non-packed elements use a dedicated hot path:
	/// the suffix is moved once with `memmove`, and insertion inside the current
	/// geometric capacity updates only the logical count. Types that require
	/// element-wise relocation and packed columns retain the general array path.
	/// @return The index immediately after the inserted element.
	///-----------------------------------------------------------------------------
	constexpr I Insert( const I nIndex, const T &element )
	{
		// Only byte-relocatable objects can be shifted without invoking individual
		// move constructors and destructors. Packed values require bit-level shifts.
		if constexpr ( !IS_PACKED_STORAGE && IS_MEMMOVE_SAFE< T > )
		{
			// The pointer arithmetic and raw temporary storage below are runtime-only.
			// Keep constant evaluation on the element-wise, constexpr-safe path.
			if ( IsConstantEvaluated() )
				return InsertArray( nIndex, 1, { element } );

			const I nOldCount = Count();
			const I nNewCount = nOldCount + 1;
			T *pData;
			const T *pElement = &element;
			alignas( T ) uchar_t arrCopy[ sizeof( T ) ];

			BALL_ASSERT( nIndex <= nOldCount );

			// `element` may refer to an existing row of this vector. In that case the
			// following suffix shift can overwrite its value, so preserve only aliases
			// in aligned local storage. External values—the common benchmark and API
			// path—avoid an unconditional temporary copy.
			const uintptr_t nElement = reinterpret_cast< uintptr_t >( pElement );
			const uintptr_t nBegin = reinterpret_cast< uintptr_t >( Base() );
			const uintptr_t nEnd = nBegin + static_cast< uintptr_t >( nOldCount ) * sizeof( T );

			if ( nBegin <= nElement && nElement < nEnd )
				pElement = static_cast< const T * >( memcpy( arrCopy, pElement, sizeof( T ) ) );

			// Overflow capacity is a power of two. If the current count is not itself a
			// growth boundary, adding one element cannot change the allocation or active
			// column pointer. Open the gap directly and update only the logical count.
			if ( nOldCount > Base_t::COMMON_FIXED_COUNT && ( static_cast< Unsigned_t >( nOldCount ) & static_cast< Unsigned_t >( nOldCount - 1 ) ) )
			{
				pData = Base_t::Data();

				if ( nIndex < nOldCount )
					ShiftElementsRight( &pData[ nIndex + 1 ], &pData[ nIndex ], &pData[ nOldCount ] );

				Base_t::SetCount( nNewCount );
			}
			else
			{
				// Inline-to-heap transitions and exact power-of-two boundaries may move
				// storage, so use the regular allocation and pointer-commit path.
				pData = EnsureCapacity( nNewCount );

				if ( nIndex < nOldCount )
					ShiftElementsRight( &pData[ nIndex + 1 ], &pData[ nIndex ], &pData[ nOldCount ] );

				Set( nNewCount, pData );
			}

			// The shift leaves an uninitialized gap; start the new object's lifetime in
			// that slot after all potentially invalidating storage operations are done.
			ConstructElement( &pData[ nIndex ], *pElement );

			return nIndex + 1;
		}

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

		BALL_STATIC_ASSERT( COUNT > 0, "InsertMultiple requires at least one element" );

		const I nCount = I( COUNT );
		I nOffset = 0;

		T *pData = EnsureInsert( nIndex, nCount );

		if constexpr ( IS_PACKED_STORAGE )
		{
			( Packed_Set( nIndex + nOffset++, static_cast< T >( Forward< Ts >( args ) ) ), ... );
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

	constexpr I AddToTail( const T &element ) { return Append( element ); }
	constexpr I AddToTail( T &&element ) { return Append( Move( element ) ); }
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

			if ( nTailCount > 0 )
				Packed_ShiftRowsLeftBy( nIndex, nTailCount, n );
			else
				Packed_ClearRows( nIndex, n );

			SetCount( nCount - n );
		}
		else
		{
			const I nNewCount = nCount - n;

			if constexpr ( IS_MEMMOVE_SAFE< T > )
			{
				if ( !IsConstantEvaluated() )
				{
					// Removing a suffix does not touch element storage for a
					// byte-relocatable type; commit the count without resolving Base().
					if ( nIndex == nNewCount )
					{
						Base_t::SetCount( nNewCount );

						return nIndex;
					}

					T *pData = Base();
					T *pIt = &pData[ nIndex ], *pItEnd = pIt + n;

					// Byte-relocatable elements need neither per-element destruction nor
					// pointer recommit when removal stays in the same allocation.
					ShiftElements( pIt, pItEnd, &pData[ nCount ] );

					Base_t::SetCount( nNewCount );

					return nIndex;
				}
			}

			T *pData = Base();
			T *pIt = &pData[ nIndex ], *pItEnd = pIt + n;

			DestructElements( pIt, pItEnd );
			ShiftElements( pIt, pItEnd, &pData[ nCount ] );
			Set( nNewCount, pData );
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

			if ( nWith > 0 )
				Insert( i, svRepl );

			return !nWith ? ( !i ? INVALID_INDEX : ( i - 1 ) ) : ( i + nWith - 1 );
		}

		const I nCount = Count();

		BALL_ASSERT( i <= nCount );
		BALL_ASSERT( nRemove <= ( nCount - i ) );

		const I nWith = svRepl.Count();
		const I nDelta = nWith - nRemove; // can be negative
		T *pData = const_cast< T * >( Base() );

		// Case 1: exact-size replace -> copy over the window and done.
		if ( !nDelta )
		{
			for ( I k = 0; k < nWith; ++k )
				pData[ i + k ] = svRepl[ k ];

			return !nWith ? ( !i ? INVALID_INDEX : ( i - 1 ) ) : ( i + nWith - 1 );
		}

		// Case 2: shrink (nWith < nRemove) -> move suffix left, shrink logical size.
		if ( nDelta < 0 )
		{
			const I nShrink = -nDelta; // amount to pull left
			// 2.1 Copy replacement into its final spot [i .. i + nWith)
			for ( I k = 0; k < nWith; ++k )
				pData[ i + k ] = svRepl[ k ];

			// 2.2 Shift suffix left by nShrink.
			const I nSuffixBegin  = i + nRemove;
			const I nOldCount = nCount;

			ShiftElementsLeft( &pData[ i + nWith ], &pData[ nSuffixBegin ], &pData[ nOldCount ] );

			// 2.3 Commit new logical size.
			Set( nCount - nShrink, pData );

			return !nWith ? ( !i ? INVALID_INDEX : i ) : ( i + nWith );
		}

		// Case 3: grow (nWith > nRemove) -> ensure capacity, shift suffix right, then copy.
		{
			const I nGrow = nDelta; // amount to push right
			const I nNew = nCount + nGrow;

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

		if ( !nWhat )
			return 0;

		I nReplaced = 0;
		I nFrom = 0;

		for ( ; ; )
		{
			const I iFound = Find( svWhat, nFrom );

			if ( iFound == INVALID_INDEX )
				break;

			Replace( iFound, nWhat, svWith );
			++nReplaced;

			// Advance beyond the just-inserted region to avoid re-matching inside replacement.
			const I nWith = svWith.Count();

			nFrom = iFound + ( nWith > 0 ? nWith : 1 ); // ensure forward progress even when nWith == 0

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
			const T nValue = Base_t::Get( k );

			if ( nValue == from )
			{
				Base_t::SetTo( k, to );
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
		const I nNewCount = nCount - nViewCount + nInsertCount;

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
		SetCount( 0 );
	}

	constexpr void Purge()
	{
		RemoveAll();
	}

protected:
	using Base_t::Set;

	/// Append is the hottest vector operation. It does not need the bounds checks
	/// or suffix-shift branch of the general insertion path.
	template < typename U >
	constexpr I Append( U &&element )
	{
		const I nOldCount = Count();
		const I nNewCount = nOldCount + 1;
		T *pData;

		// A geometric allocation only changes immediately after a power of two.
		// Avoid recomputing BitCeil and walking the allocator path for every append.
		if ( nOldCount > Base_t::COMMON_FIXED_COUNT
			&& ( static_cast< Unsigned_t >( nOldCount ) & static_cast< Unsigned_t >( nOldCount - 1 ) ) )
		{
			pData = Base_t::Data();
			Base_t::SetCount( nNewCount );
		}
		else
		{
			pData = EnsureCapacity( nNewCount );
			Set( nNewCount, pData );
		}

		if constexpr ( IS_PACKED_STORAGE )
			Packed_Set( nOldCount, static_cast< T >( Forward< U >( element ) ) );
		else
			ConstructElement( &pData[ nOldCount ], Forward< U >( element ) );

		return nNewCount;
	}

	constexpr T *EnsureCapacity( I nRequested )
	{
		uint8_t *pData = Base_t::EnsureCapacity( nRequested );

		return GetStorage< T >( *this, nRequested, pData );
	}

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
				Packed_ShiftRowsRightBy( nIndex, nOldCount - nIndex, nAddCount );
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

		if ( !pData )
		{
			return;
		}

		if constexpr ( IS_PACKED_STORAGE )
		{
			if ( nOld < nNew )
				Packed_ClearRows( nOld, nNew - nOld );
			else if ( nOld > nNew )
				Packed_ClearRows( nNew, nOld - nNew );
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

template < class B, typename I, typename TI, typename... Ts >
class CVectorImpl : public B
{
public:
	using Base_t = B;
	using Index_t = I;
	using TypeIndex_t = TI;
	using T = typename MFirst< Ts... >::Type;
	using Element_t = T;
	using Fixed_t = MFixed< Index_t >;
	using Unsigned_t = typename Fixed_t::Unsigned_t;
	using Base_t::TYPE_COUNT;
	using View_t = Conditional_t< TYPE_COUNT == 1, CView< I, T, Base_t::FIXED_COUNT >, typename Base_t::View_t >;
	using ConstView_t = Conditional_t< TYPE_COUNT == 1, CView< I, const T, Base_t::FIXED_COUNT >, typename Base_t::ConstView_t >;
	template < I GN > using GrowableView_t = CView< I, T, GN >;
	template < I GN > using ConstGrowableView_t = CView< I, const T, GN >;

	using Base_t::Base_t;
	template < typename U > static constexpr bool IS_PACKED_STORAGE_BY = ::BTL::IS_PACKED_STORAGE_BY< Base_t, U >;
	using Base_t::Count;
	using Base_t::Set;
	using Base_t::INVALID_INDEX;
	using Base_t::Empty;
	using Base_t::Get;
	using Base_t::Find;
	using Base_t::RFind;
	static constexpr bool IS_PACKED_STORAGE = IS_PACKED_STORAGE_BY< T >;
	static constexpr I FIXED_COUNT = Base_t::FIXED_COUNT;
	using Base_t::Size;
	using Base_t::FixedCount;
	using Base_t::FixedSize;
	using Base_t::FixedCapacity;
	using Base_t::FixedCapacitySize;
	using Base_t::IsOverflow;
	using Base_t::Packed_IsOverflow;
	using Base_t::FixedData;
	using Base_t::Data;
	using Base_t::Base;
	using Base_t::Packed_Data;
	using Base_t::Packed_Base;
	using Base_t::Packed_FixedData;
	using Base_t::Packed_SizeBy;
	using Base_t::Packed_Set;
	using Base_t::Packed_ShiftRowsLeftBy;
	using Base_t::Packed_ShiftRowsRightBy;
	using Base_t::Packed_ClearRows;
	using Base_t::StoreFirstElement;
	using Base_t::ShiftLeftBaseBy;
	using Base_t::ShiftRightBaseBy;
	using Base_t::Packed_ClearRowsBy;
	using Base_t::Packed_SetBy;

	constexpr void SetTo( I i, const T &value ) noexcept
	{
		if constexpr ( IS_PACKED_STORAGE )
			Packed_Set( i, value );
		else
			Base()[ i ] = value;
	}
	constexpr decltype( auto ) Get( I i ) noexcept
	{
		if constexpr ( IS_PACKED_STORAGE )
			return GetElement< T >( *this, i );
		else
			return GetElement< T >( *this, i );
	}
	constexpr decltype( auto ) Get( I i ) const noexcept { return GetElement< T >( *this, i ); }
	constexpr decltype( auto ) operator[]( I i ) noexcept { return Get( i ); }
	constexpr decltype( auto ) operator[]( I i ) const noexcept { return Get( i ); }
	constexpr I Find( const T &value, I iFrom = 0 ) const noexcept { return FindBy< T >( *this, value, iFrom ); }
	constexpr I RFind( const T &value, I iFrom = INVALID_INDEX ) const noexcept { return RFindBy< T >( *this, value, iFrom ); }
	constexpr ConstView_t Const() const noexcept { return ConstView_t( Count(), Base() ); }
	constexpr operator ConstView_t() const noexcept { return Const(); }

	constexpr CVectorImpl &CopyFrom( const ConstView_t &other )
	{
		Base_t::CopyFrom( static_cast< const CViewBase< I, Base_t::FIXED_COUNT, size8_t, const T > & >( other ) );

		return *this;
	}
	constexpr CVectorImpl &CopyFrom( const View_t &other ) { return CopyFrom( other.Const() ); }
	template < I GN > constexpr CVectorImpl &CopyFrom( const GrowableView_t< GN > &other ) { return CopyFrom( other.Const() ); }
	template < I GN > constexpr CVectorImpl &CopyFrom( const ConstGrowableView_t< GN > &other )
	{
		Base_t::CopyFrom( static_cast< const CViewBase< I, GN, size8_t, const T > & >( other ) );

		return *this;
	}
	constexpr CVectorImpl &MoveFrom( View_t &&other )
	{
		CopyFrom( other );
		other.Set( 0, nullptr );

		return *this;
	}

	constexpr T *EnsureCapacity( I nRequested )
	{
		uint8_t *pData = Base_t::EnsureCapacity( nRequested );

		return GetStorage< T >( *this, nRequested, pData );
	}

	constexpr CVectorImpl() noexcept : Base_t() {}
	constexpr CVectorImpl( const View_t &copyFrom ) noexcept { Base_t::CopyFrom( copyFrom ); }
	constexpr CVectorImpl( const ConstView_t &copyFrom ) noexcept { Base_t::CopyFrom( copyFrom ); }
	constexpr CVectorImpl( View_t &&moveFrom ) noexcept { Base_t::MoveFrom( Move( moveFrom ) ); }

	// Construct a one-row container in place (one value per column) via InitFirst,
	// so the first row is established as part of construction. Constrained to the
	// exact column arity (and >1 column) to never shadow the single-argument view
	// constructors above.
	template < typename... Us, EnableIf_t< ( TYPE_COUNT > 1 ) && ( TYPE_COUNT == sizeof...( Us ) ), int > = 0 >
	constexpr explicit CVectorImpl( Us &&...args ) noexcept : Base_t() { InitFirst( Forward< Us >( args )... ); }

	~CVectorImpl() noexcept
	{
		if constexpr ( TYPE_COUNT > 1 )
			( DestructAllBy< Ts >(), ... );
	}

	CVectorImpl &operator=( const View_t &copyFrom ) { Base_t::CopyFrom( copyFrom ); return *this; }
	CVectorImpl &operator=( const ConstView_t &copyFrom ) { Base_t::CopyFrom( copyFrom ); return *this; }
	CVectorImpl &operator=( CVectorImpl &&moveFrom ) { Base_t::MoveFrom( Move( moveFrom ) ); return *this; }

	// ---------------------------
	// Size management
	// ---------------------------

	///-----------------------------------------------------------------------------
	/// @brief Like @ref SetCount, but leaves any newly grown rows as raw storage.
	///
	/// @details Reserves through @ref EnsureCapacity directly and repoints the
	/// columns. Shrinking still destructs the dropped rows (as @ref SetCount does),
	/// but growing does NOT construct the new rows: the caller owns the raw memory
	/// in `[old count, nNew)` and must fill it by construction before those rows are
	/// read or destroyed. Rows below the old count keep their live objects. Suited
	/// to bulk builds that move each element into place exactly once.
	///-----------------------------------------------------------------------------
	constexpr void SetCountRaw( I nNew )
	{
		const I nOld = Count();

		// On shrink, destruct the dropped rows [nNew, nOld) in the CURRENT buffer before
		// EnsureCapacity may reallocate to a smaller capacity. EnsureCapacity only copies
		// [0, nNew) into the new buffer, so doing the destruct afterwards would index past
		// the shrunken column (and, for the packed column, clear a bit one byte into the
		// next column). Growth still constructs the new rows after (re)allocation.
		if ( nNew < nOld )
			( ResizeRangeBy< Ts >( nOld, nNew ), ... );

		uint8_t *pData = Base_t::EnsureCapacity( nNew );

		Set( nNew, GetStorage< Ts >( *this, nNew, pData )... );
	}

	///-----------------------------------------------------------------------------
	/// @brief Changes the logical row count while keeping all columns synchronized.
	///-----------------------------------------------------------------------------
	constexpr void SetCount( I nNew )
	{
		const I nOld = Count();

		SetCountRaw( nNew );

		if ( nNew > nOld )
			( ResizeRangeBy< Ts >( nOld, nNew ), ... );
	}

	constexpr void Resize( I nNew )
	{
		SetCount( nNew );
	}

	constexpr void Grow( I delta )
	{
		const I nNew = Count() + delta;

		BALL_ASSERT( nNew >= 0 );
		SetCount( nNew );
	}

	constexpr void RemoveAll()
	{
		SetCount( 0 );
	}

	/// @brief Standard clear alias for `RemoveAll()`.
	constexpr void Clear()
	{
		RemoveAll();
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

		BALL_ASSERT( nAdd > 0 );
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
		BALL_STATIC_ASSERT( TYPE_COUNT == sizeof...( Us ), "Insert requires exactly one value per type" );

		EnsureInsert( i, 1 );
		AssignRow< Ts... >( i, Forward< Us >( args )... );

		return i + 1;
	}

	///-----------------------------------------------------------------------------
	/// @brief Insert @p nCount identical rows at @p i.
	/// @return Index right after the inserted block.
	///-----------------------------------------------------------------------------
	template < typename... Us >
	constexpr I InsertMultiple( I i, I nCount, const Us &...args )
	{
		BALL_STATIC_ASSERT( TYPE_COUNT == sizeof...( Us ), "InsertMultiple requires exactly one value per type" );

		BALL_ASSERT( nCount > 0 );
		EnsureInsert( i, nCount );

		for ( I n = 0; n < nCount; ++n )
			AssignRow< Ts... >( i + n, args... );

		return i + nCount;
	}

	constexpr I AddToHead( const ConstView_t &v ) { return Insert( 0, v ); }
	constexpr I AddToHead( const View_t &v ) { return Insert( 0, v ); }
	template < typename... Us > constexpr I AddToHead( Us &&...args ) { return Insert( 0, Forward< Us >( args )... ); }
	template < typename... Us > constexpr I AddMultipleToHead( I nCount, const Us &...args ) { return InsertMultiple( 0, nCount, args... ); }

	constexpr I AddToTail( const ConstView_t &v ) { return Insert( Count(), v ); }
	constexpr I AddToTail( const View_t &v ) { return Insert( Count(), v ); }
	template < typename... Us > constexpr I AddToTail( Us &&...args )
	{
		BALL_STATIC_ASSERT( TYPE_COUNT == sizeof...( Us ), "AddToTail requires exactly one value per type" );

		const I nOld = Count();
		const I nNew = nOld + 1;
		uint8_t *pData = Base_t::EnsureCapacity( nNew );

		Set( nNew, GetStorage< Ts >( *this, nNew, pData )... );
		AssignRow< Ts... >( nOld, Forward< Us >( args )... );

		return nNew;
	}
	template < typename... Us > constexpr I AddMultipleToTail( I nCount, const Us &...args ) { return InsertMultiple( Count(), nCount, args... ); }

	///-----------------------------------------------------------------------------
	/// @brief One-time initialization of the very first row (index 0) of an empty
	/// container.
	/// @details When the row fits the inline buffer this constructs it in place via
	/// a constant-expression-safe path (no grow/shift machinery), so it can run at
	/// compile time. Otherwise it falls back to the regular append, which is only
	/// reachable at run time (overflow needs heap storage). Requires `Count() == 0`.
	///-----------------------------------------------------------------------------
	template < typename... Us >
	constexpr I InitFirst( Us &&...args )
	{
		BALL_STATIC_ASSERT( TYPE_COUNT == sizeof...( Us ), "InitFirst requires exactly one value per type" );
		BALL_ASSERT_MESSAGE( !Count(), "InitFirst requires an empty container" );

		if ( Base_t::IsDataOverflow( 1 ) )
			return AddToTail( Forward< Us >( args )... );

		StoreFirst< Ts... >( *this, static_cast< Ts >( Forward< Us >( args ) )... );

		return 1;
	}

	void Remove( I i, I nRemove = 1 )
	{
		BALL_ASSERT( nRemove > 0 );
		BALL_ASSERT( 0 <= i && i + nRemove <= Count() );

		const I nCount = Count();
		const I nNewCount = nCount - nRemove;

		( DestructRangeBy< Ts >( i, nRemove ), ... );
		( ShiftStorageLeft< Ts >( *this, i, nRemove, nCount ), ... );

		uint8_t *pData = Base_t::EnsureCapacity( nNewCount );

		Set( nNewCount, GetStorage< Ts >( *this, nNewCount, pData )... );
	}

protected:
	///-----------------------------------------------------------------------------
	/// @brief Reserve and open a gap of @p nAdd rows at index @p i.
	///-----------------------------------------------------------------------------
	void EnsureInsert( I i, I nAdd )
	{
		BALL_ASSERT( nAdd > 0 );
		BALL_ASSERT( 0 <= i && i <= Count() );

		const I nOld = Count();
		const I nNew = nOld + nAdd;
		uint8_t *pData = Base_t::EnsureCapacity( nNew );

		Set( nNew, GetStorage< Ts >( *this, nNew, pData )... );

		if ( i < nOld )
			( ShiftStorageRight< Ts >( *this, i, nAdd, nOld ), ... );
	}

private:
	template < typename T >
	constexpr void ResizeRangeBy( I nOld, I nNew )
	{
		if constexpr ( IS_PACKED_STORAGE_BY< T > )
		{
			if ( nOld < nNew )
				Packed_Clear< T >( *this, nOld, nNew - nOld );
			else if ( nOld > nNew )
				Packed_Clear< T >( *this, nNew, nOld - nNew );

		}
		else
		{
			T *pData = GetBase< T >( *this );

			if ( !pData )
				return;

			if ( nOld < nNew )
				ConstructElements( &pData[ nOld ], &pData[ nNew ] );
			else if ( nOld > nNew )
				DestructElements( &pData[ nNew ], &pData[ nOld ] );
		}
	}

	template < typename T >
	constexpr void DestructRangeBy( I i, I nCount )
	{
		if constexpr ( !IS_PACKED_STORAGE_BY< T > )
		{
			T *pData = GetBase< T >( *this );

			if ( !pData )
				return;

			DestructElements( &pData[ i ], &pData[ i + nCount ] );
		}
	}

	template < typename T >
	constexpr void DestructAllBy()
	{
		if constexpr ( !IS_PACKED_STORAGE_BY< T > )
		{
			T *pData = GetBase< T >( *this );

			DestructElements( pData, pData + Count() );
		}
	}

	template < typename T >
	constexpr void CopyFromViewBy( I i, const ConstView_t &v )
	{
		const I nAdd = v.Count();

		if constexpr ( IS_PACKED_STORAGE_BY< T > )
		{
			for ( I n = 0; n < nAdd; ++n )
				::BTL::Packed_Set< T >( *this, static_cast< I >( i + n ), v.template Packed_GetBy< T >( n ) );
		}
		else
		{
			CopyElements( nAdd, GetBase< T >( *this ) + i, v.template BaseBy< T >() );
		}
	}

	template < typename T0, typename... TRest, typename U0, typename... URest >
	constexpr void AssignRow( I i, U0 &&u0, URest &&...urest )
	{
		// Address the column by positional index, not by type: distinct columns
		// may share a type (e.g. a key and value column of the same type), which
		// would make type-based access ambiguous and clobber the wrong column.
		constexpr TI COLUMN = static_cast< TI >( TYPE_COUNT - 1 - sizeof...( TRest ) );

		if constexpr ( IS_PACKED_STORAGE_BY< T0 > )
			::BTL::Packed_Set< T0 >( *this, i, static_cast< T0 >( Forward< U0 >( u0 ) ) );
		else
			// The slot is freshly opened (uninitialized) gap memory, so construct in
			// place rather than assign: a move/copy assignment would first release the
			// destination's prior contents, which here are garbage (tail growth) or a
			// raw memmove duplicate (middle insert) and not a live object.
			ConstructElement( &GetBaseBy< COLUMN >( *this )[ i ], static_cast< T0 >( Forward< U0 >( u0 ) ) );

		if constexpr ( 0 < sizeof...( TRest ) )
			AssignRow< TRest... >( i, Forward< URest >( urest )... );
	}
};

template < typename I, I N, typename T, typename... Ts > class CBufferVector;
template < typename I, typename T, typename... Ts > class CVector;

template < typename I, I N, typename... Ts >
using VectorImpl_t = CVectorImpl< CVectorBase< CAllocatorBase, CViewBase< I, N, size8_t, Ts... >, I, N, size8_t, Ts... >, I, size8_t, Ts... >;

template < typename I, typename T, typename... Ts >
class CVector : public VectorImpl_t< I, 0, T, Ts... >
{
public:
	using Base_t = VectorImpl_t< I, 0, T, Ts... >;
	using Base_t::TYPE_COUNT;
	using Base_t::Base_t;
	using Base_t::AddToTail;
	using Base_t::CopyFrom;

	constexpr CVector() noexcept = default;
	constexpr CVector( const CVector &other ) { CopyFromAuto( other ); }
	constexpr CVector( CVector &&other ) noexcept { MoveFrom( Move( other ) ); }
	template < I N > constexpr CVector( const CBufferVector< I, N, T, Ts... > &other ) { CopyFromAuto( other ); }
	template < I N > constexpr CVector( CBufferVector< I, N, T, Ts... > &&other ) { MoveFrom( Move( other ) ); }
	constexpr CVector &MoveFrom( CVector &&other ) noexcept
	{
		if ( this != &other )
		{
			CopyFromAuto( other );
			other.RemoveAll();
		}

		return *this;
	}
	template < I N > constexpr CVector &MoveFrom( CBufferVector< I, N, T, Ts... > &&other )
	{
		CopyFromAuto( other );
		other.RemoveAll();

		return *this;
	}
	constexpr CVector &operator=( const CVector &other ) { return CopyFromAuto( other ); }
	constexpr CVector &operator=( CVector &&other ) noexcept { return MoveFrom( Move( other ) ); }
	template < I N > constexpr CVector &operator=( const CBufferVector< I, N, T, Ts... > &other ) { return CopyFromAuto( other ); }
	template < I N > constexpr CVector &operator=( CBufferVector< I, N, T, Ts... > &&other ) { return MoveFrom( Move( other ) ); }
	constexpr CVector &operator+=( const T &element ) requires ( TYPE_COUNT == 1 ) { AddToTail( element ); return *this; }
	constexpr CVector &operator+=( T &&element ) requires ( TYPE_COUNT == 1 ) { AddToTail( Move( element ) ); return *this; }
	constexpr CVector &operator+=( const CVector &other ) requires ( TYPE_COUNT == 1 )
	{
		if ( this == &other )
		{
			CVector copy( other );

			AddToTail( CView< I, const T, 0 >( copy.Count(), copy.Base() ) );
		}
		else
			AddToTail( CView< I, const T, 0 >( other.Count(), other.Base() ) );

		return *this;
	}
	template < I N > constexpr CVector &operator+=( const CBufferVector< I, N, T, Ts... > &other ) requires ( TYPE_COUNT == 1 ) { AddToTail( CView< I, const T, N >( other.Count(), other.Base() ) ); return *this; }

private:
	template < class V > constexpr CVector &CopyFromAuto( const V &other )
	{
		if constexpr ( TYPE_COUNT == 1 )
			Base_t::CopyFrom( typename Base_t::ConstView_t( other.Count(), other.Base() ) );
		else
			Base_t::CopyFrom( typename Base_t::ConstView_t( other.Count(), other.template BaseBy< T >(), other.template BaseBy< Ts >()... ) );

		return *this;
	}
};

template < typename I, I N, typename T, typename... Ts >
class CBufferVector : public VectorImpl_t< I, N, T, Ts... >
{
public:
	using Base_t = VectorImpl_t< I, N, T, Ts... >;
	using Base_t::TYPE_COUNT;
	using Base_t::Base_t;
	using Base_t::AddToTail;
	using Base_t::CopyFrom;

	constexpr CBufferVector() noexcept = default;
	constexpr CBufferVector( const CBufferVector &other ) { CopyFromAuto( other ); }
	constexpr CBufferVector( CBufferVector &&other ) noexcept { MoveFrom( Move( other ) ); }
	constexpr CBufferVector( const CVector< I, T, Ts... > &other ) { CopyFromAuto( other ); }
	constexpr CBufferVector( CVector< I, T, Ts... > &&other ) { MoveFrom( Move( other ) ); }
	constexpr CBufferVector &MoveFrom( CBufferVector &&other ) noexcept
	{
		if ( this == &other )
			return *this;

		CopyFromAuto( other );
		other.RemoveAll();
	}
	constexpr CBufferVector &MoveFrom( CVector< I, T, Ts... > &&other )
	{
		CopyFromAuto( other );
		other.RemoveAll();

		return *this;
	}
	constexpr CBufferVector &operator=( const CBufferVector &other ) { return CopyFromAuto( other ); }
	constexpr CBufferVector &operator=( CBufferVector &&other ) noexcept { return MoveFrom( Move( other ) ); }
	constexpr CBufferVector &operator=( const CVector< I, T, Ts... > &other ) { return CopyFromAuto( other ); }
	constexpr CBufferVector &operator=( CVector< I, T, Ts... > &&other ) { return MoveFrom( Move( other ) ); }
	constexpr CBufferVector &operator+=( const T &element ) requires ( TYPE_COUNT == 1 ) { AddToTail( element ); return *this; }
	constexpr CBufferVector &operator+=( T &&element ) requires ( TYPE_COUNT == 1 ) { AddToTail( Move( element ) ); return *this; }
	constexpr CBufferVector &operator+=( const CBufferVector &other ) requires ( TYPE_COUNT == 1 )
	{
		if ( this == &other )
		{
			CBufferVector copy( other );

			AddToTail( CView< I, const T, N >( copy.Count(), copy.Base() ) );
		}
		else
			AddToTail( CView< I, const T, N >( other.Count(), other.Base() ) );

		return *this;
	}
	constexpr CBufferVector &operator+=( const CVector< I, T, Ts... > &other ) requires ( TYPE_COUNT == 1 ) { AddToTail( CView< I, const T, 0 >( other.Count(), other.Base() ) ); return *this; }

private:
	template < class V > constexpr CBufferVector &CopyFromAuto( const V &other )
	{
		if constexpr ( TYPE_COUNT == 1 )
			Base_t::CopyFrom( typename Base_t::ConstView_t( other.Count(), other.Base() ) );
		else
			Base_t::CopyFrom( typename Base_t::ConstView_t( other.Count(), other.template BaseBy< T >(), other.template BaseBy< Ts >()... ) );

		return *this;
	}
};

template < typename T, typename... Ts > using Vector_t = CVector< size_t, T, Ts... >;
template < typename T, typename... Ts > using Vector8_t = CVector< size8_t, T, Ts... >;
template < typename T, typename... Ts > using Vector16_t = CVector< size16_t, T, Ts... >;
template < typename T, typename... Ts > using Vector32_t = CVector< size32_t, T, Ts... >;
template < typename T, typename... Ts > using Vector64_t = CVector< size64_t, T, Ts... >;

template < size_t N, typename T, typename... Ts > using BufferVector_t = CBufferVector< size_t, N, T, Ts... >;
template < size8_t N, typename T, typename... Ts > using BufferVector8_t = CBufferVector< size8_t, N, T, Ts... >;
template < size16_t N, typename T, typename... Ts > using BufferVector16_t = CBufferVector< size16_t, N, T, Ts... >;
template < size32_t N, typename T, typename... Ts > using BufferVector32_t = CBufferVector< size32_t, N, T, Ts... >;
template < size64_t N, typename T, typename... Ts > using BufferVector64_t = CBufferVector< size64_t, N, T, Ts... >;

#endif // !defined( _INCLUDE_BALL_TYPES_VECTOR_HPP_ )
