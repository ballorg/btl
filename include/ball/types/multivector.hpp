#ifndef _INCLUDE_BALL_TYPES_MULTIVECTOR_HPP_
#	define _INCLUDE_BALL_TYPES_MULTIVECTOR_HPP_

#	pragma once

#	include "base/arch.h"
#	include "c/assert.h"
#	include "meta/fixed.hpp"
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
class CMultiVectorBase : public CViewBase< I, N, TI, Ts... >
{
public:
	using Base_t      = CViewBase< I, N, TI, Ts... >;
	using Index_t     = I;
	using TypeIndex_t = TI;
	using Fixed_t     = MFixed< Index_t >;
	using View_t      = Base_t;
	using ConstView_t = typename Base_t::Const_t;

	template < typename T > using Allocator_t = CAllocator< I, T >;

	using Base_t::Base_t;

	using Base_t::FIRST_INDEX;
	using Base_t::FIXED_COUNT;
	using Base_t::INVALID_INDEX;
	static constexpr bool IS_GROWABLE = FIXED_COUNT > 0;

	using Base_t::Count;
	using Base_t::FindBy;
	using Base_t::RFindBy;

	~CMultiVectorBase() noexcept
	{
		FreeAllHeaps();
		Base_t::Set( I( 0 ), ( static_cast< Ts * >( nullptr ) )... );
	}

	// ---------------------------
	// Capacity model
	// ---------------------------
	constexpr I Capacity() const noexcept { return BitCeil< I >( Count() ); }

	// ---------------------------
	// Typed storage access
	// ---------------------------
	template < typename T > constexpr bool IsOverflow( I nCount ) const noexcept { return Base_t::template IsOverflow< T >( nCount ); }
	template < typename T > constexpr bool IsOverflow() const noexcept { return IsOverflow< T >( Count() ); }

	template < typename T > constexpr T       *FixedData() noexcept { return Base_t::template FixedData< T >(); }
	template < typename T > constexpr const T *FixedData() const noexcept { return Base_t::template FixedData< T >(); }
	template < typename T > constexpr T       *Data() noexcept { return Base_t::template Data< T >(); }
	template < typename T > constexpr const T *Data() const noexcept { return Base_t::template Data< T >(); }
	template < typename T > constexpr T       *Base() noexcept { return Base_t::template Base< T >(); }
	template < typename T > constexpr const T *Base() const noexcept { return Base_t::template Base< T >(); }

	// ---------------------------
	// Typed element access (style from CVector/CView)
	// ---------------------------
	constexpr bool IsValidIndex( I i ) const noexcept { return i != INVALID_INDEX; }

	template < typename T >
	constexpr T &At( I i )
	{
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( I( 0 ) <= i && i < Count() );

		return Base< T >()[ i ];
	}

	template < typename T >
	constexpr const T &At( I i ) const
	{
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( I( 0 ) <= i && i < Count() );

		return Base< T >()[ i ];
	}

	template < typename T > constexpr T &operator[]( I i ) { return At< T >( i ); }
	template < typename T > constexpr const T &operator[]( I i ) const { return At< T >( i ); }

	template < typename T >
	constexpr const T &Front() const
	{
		BALL_ASSERT( I( 0 ) < Count() );
		return Base< T >()[ 0 ];
	}

	template < typename T >
	constexpr const T &Back() const
	{
		BALL_ASSERT( I( 0 ) < Count() );
		return Base< T >()[ Count() - 1 ];
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
	/// @brief Find last row equal to (args...) searching from tail.
	/// @return Row index or INVALID_INDEX.
	///-----------------------------------------------------------------------------
	template < typename... Us, EnableIf_t< ( 1 < sizeof...( Ts ) ) && ( sizeof...( Us ) == sizeof...( Ts ) ), int > = 0 >
	constexpr I RFind( const Us &...args ) const noexcept
	{
		return RFindFrom( INVALID_INDEX, args... );
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

	// ---------------------------
	// Copy / move from views
	// ---------------------------
	CMultiVectorBase &CopyFrom( const ConstView_t &other )
	{
		if ( this == &other )
			return *this;

		const I nNew = other.Count();

		Base_t::Set( nNew, CopyFromBy< Ts >( nNew, other )... );

		return *this;
	}

	CMultiVectorBase &CopyFrom( const View_t &other )
	{
		if ( this == &other )
			return *this;

		return CopyFrom( other.Const() );
	}

	template< I LN > constexpr CMultiVectorBase &CopyFrom( const CViewBase< I, LN, TI, Ts... > &other )
	{
		const I nNew = other.Count();

		Base_t::Set( nNew, CopyFromBy< Ts >( nNew, other )... );

		return *this;
	}

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
	// ---------------------------
	// Raw pointer helpers
	// ---------------------------
	template < typename T > constexpr T *Writable() { return Base< T >(); }
	template < typename T > constexpr const T *Readable() const { return Base< T >(); }

protected:
	template < typename T > static constexpr size_t AlignedSize() { return BitCeil_Const< I >( 8 * sizeof( T ) ); }

	template < typename T >
	T *EnsureCapacityBy( I nRequest )
	{
		const I nOld = Count();
		const bool bWasOverflow = IsOverflow< T >( nOld );
		const bool bWillOverflow = IsOverflow< T >( nRequest );

		T *pElements = Base< T >();

		if ( bWillOverflow )
		{
			const I nCapacity = BitCeil< I >( nOld );
			const I nNewCapacity = BitCeil< I >( nRequest );

			BALL_ASSERT_IF_MESSAGE( nNewCapacity == Fixed_t::INVALID, "Capacity overflow!" )
				return pElements;

			if ( bWasOverflow )
			{
				if ( nNewCapacity == nCapacity )
					return pElements;

				pElements = Allocator_t< T >::Realloc( pElements, nNewCapacity, AlignedSize< T >() );
				BALL_ASSERT_MESSAGE( pElements != nullptr, "Failed to reallocate elements" );
			}
			else
			{
				pElements = Allocator_t< T >::Alloc( nNewCapacity, AlignedSize< T >() );
				BALL_ASSERT_MESSAGE( pElements != nullptr, "Failed to allocate elements" );

				if constexpr ( IS_GROWABLE )
					CopyElements( nOld, pElements, FixedData< T >() );
			}

			return pElements;
		}

		if ( bWasOverflow )
		{
			if constexpr ( IS_GROWABLE )
				CopyElements( nRequest, FixedData< T >(), pElements );

			Allocator_t< T >::Free( pElements );
			return FixedData< T >();
		}

		return FixedData< T >();
	}

	template < typename T >
	T *CopyFromBy( I nCount, const ConstView_t &other )
	{
		T *pDest = EnsureCapacityBy< T >( nCount );
		const T *pSrc = other.template Base< T >();

		if ( nCount > I( 0 ) )
			CopyElements( nCount, pDest, pSrc );

		return pDest;
	}

	template < typename T >
	T *InsertBy( I i, I nAdd, I nOld, I nNew )
	{
		T *p = EnsureCapacityBy< T >( nNew );

		if ( i < nOld )
			ShiftElementsRight( &p[ i + nAdd ], &p[ i ], &p[ nOld ] );

		return p;
	}

	template < typename T >
	void ShiftLeftByType( I i, I nRemove, I nOld )
	{
		T *p = Base< T >();
		const I nTailBegin = i + nRemove;

		if ( nTailBegin < nOld )
			ShiftElementsLeft( &p[ i ], &p[ nTailBegin ], &p[ nOld ] );
	}

	void FreeAllHeaps()
	{
		( FreeByType< Ts >(), ... );
	}

	template < typename T >
	void FreeByType()
	{
		if ( IsOverflow< T >() )
		{
			T *p = Data< T >();

			if ( p )
				Allocator_t< T >::Free( p );
		}
	}

	template < typename T0, typename... TRest, typename U0, typename... URest >
	constexpr bool RowEquals( I i, const U0 &u0, const URest &...urest ) const noexcept
	{
		if ( !( Base< T0 >()[ i ] == u0 ) )
			return false;

		if constexpr ( sizeof...( TRest ) > 0 )
			return RowEquals< TRest... >( i, urest... );

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
	using Base_t::Count;

	constexpr CMultiVectorImpl() noexcept : Base_t() {}
	constexpr CMultiVectorImpl( const View_t &copyFrom ) noexcept { Base_t::CopyFrom( copyFrom ); }
	constexpr CMultiVectorImpl( const ConstView_t &copyFrom ) noexcept { Base_t::CopyFrom( copyFrom ); }
	constexpr CMultiVectorImpl( View_t &&moveFrom ) noexcept { Base_t::MoveFrom( Move( moveFrom ) ); }

	CMultiVectorImpl &operator=( const View_t &copyFrom ) { Base_t::CopyFrom( copyFrom ); return *this; }
	CMultiVectorImpl &operator=( const ConstView_t &copyFrom ) { Base_t::CopyFrom( copyFrom ); return *this; }
	CMultiVectorImpl &operator=( CMultiVectorImpl &&moveFrom ) { Base_t::MoveFrom( Move( moveFrom ) ); return *this; }

	// ---------------------------
	// Size management
	// ---------------------------
	void SetCount( I nNew )
	{
		Base_t::Set( nNew, Base_t::template EnsureCapacityBy< Ts >( nNew )... );
	}

	void Grow( I delta )
	{
		const I nNew = Count() + delta;

		BALL_ASSERT( nNew >= I( 0 ) );
		SetCount( nNew );
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
		( CopyElements( nAdd, Base_t::template Base< Ts >() + i, v.template Base< Ts >() ), ... );

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

		const I nOld = Count();
		const I nNew = nOld - nRemove;

		( Base_t::template ShiftLeftByType< Ts >( i, nRemove, nOld ), ... );
		Base_t::Set( nNew, Base_t::template EnsureCapacityBy< Ts >( nNew )... );
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

		Base_t::Set( nNew, Base_t::template InsertBy< Ts >( i, nAdd, nOld, nNew )... );
	}

private:
	template < typename T0, typename... TRest, typename U0, typename... URest >
	constexpr void AssignRow( I i, U0 &&u0, URest &&...urest )
	{
		Base_t::template Base< T0 >()[ i ] = Forward< U0 >( u0 );

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

template < typename... Ts > using MultiVector_t =            CMultiVector< size_t, uint8_t, Ts... >;
template < typename... Ts > using MultiVector8_t =           CMultiVector< uint8_t, uint8_t, Ts... >;
template < typename... Ts > using MultiVector16_t =          CMultiVector< uint16_t, uint8_t, Ts... >;
template < typename... Ts > using MultiVector32_t =          CMultiVector< uint32_t, uint8_t, Ts... >;
template < typename... Ts > using MultiVector64_t =          CMultiVector< uint64_t, uint8_t, Ts... >;

template < size_t N, typename... Ts > using BufferMultiVector_t =            CBufferMultiVector< size_t, N, uint8_t, Ts... >;
template < uint8_t N, typename... Ts > using BufferMultiVector8_t =          CBufferMultiVector< uint8_t, N, uint8_t, Ts... >;
template < uint16_t N, typename... Ts > using BufferMultiVector16_t =        CBufferMultiVector< uint16_t, N, uint8_t, Ts... >;
template < uint32_t N, typename... Ts > using BufferMultiVector32_t =        CBufferMultiVector< uint32_t, N, uint8_t, Ts... >;
template < uint64_t N, typename... Ts > using BufferMultiVector64_t =        CBufferMultiVector< uint64_t, N, uint8_t, Ts... >;

#endif // !defined( _INCLUDE_BALL_TYPES_MULTIVECTOR_HPP_ )
