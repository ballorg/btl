#ifndef _INCLUDE_BALL_TYPES_ARRAY_HPP_
#	define _INCLUDE_BALL_TYPES_ARRAY_HPP_

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "c/assert.h"
#	include "meta/number.hpp"
#	include "math.hpp"

template < typename I, typename T >
class CEmptyArray
{
public:
	constexpr       T *Base()       noexcept    { return nullptr; }
	constexpr const T *Base() const noexcept    { return nullptr; }
	constexpr       T *Data()       noexcept    { return Base(); }
	constexpr const T *Data() const noexcept    { return Base(); }

	static constexpr I      Count() noexcept    { return 0; }
	static constexpr bool   Empty() noexcept    { return true; }
}; // class CEmptyArray

/*
------------------------------------------------------------------------------
CArray<I, TI, T, N> — fixed-size, owning AoS container for a single type T.

Constraints:
- No dynamic counters. Storage is exactly `T m_Elements[ N ]`.
- Constexpr-friendly metadata and address computations (C++20).
- Not derived from CMemoryViewBase; provides View()/ConstView() that return
  compatible non-owning views.

Behavior:
- For N == 0: Base()/Data()/begin()/end() return nullptr; Front()/Back() assert.
- Bounds are checked via BALL_ASSERT in At()/Front()/Back().
- No exceptions; contract-based API.

Notes:
- ValueInit() helper returns an instance with value-initialized elements.
- Fill(), Set(), CopyFrom() avoid std:: algorithms.
-------------------------------------------------------------------------------
*/
template < typename I, typename T, I N >
class CArray
{
public:
	using Index_t       = I;
	using Number_t      = MNumber< Index_t >;

	static constexpr I FIRST_INDEX   =  I( 0 );
	static constexpr I INVALID_INDEX =  Number_t::INVALID;

public:
	// --------- ctors / assignment ----------
	constexpr CArray() noexcept = default;
	constexpr CArray( const CArray & ) noexcept = default;
	constexpr CArray( CArray && ) noexcept = default;
	constexpr CArray &operator=( const CArray & ) noexcept = default;
	constexpr CArray &operator=( CArray && ) noexcept = default;

	/// @brief Construct from an N-sized C array (copies elements).
	constexpr CArray( const T ( &src )[ N ] ) noexcept
		: CArray()
	{
		CopyFrom( src );
	}

	/// @brief Return a value-initialized array (zero for scalars).
	constexpr CArray Init() noexcept
	{
		for ( I i = I( 0 ); i < I( N ); ++i )
			m_Elements[ i ] = T();
	}

	// --------- sizes / byte sizes ----------
	static constexpr size_t Stride() noexcept   { return sizeof( T ); }
	static constexpr size_t Size()   noexcept   { return static_cast< size_t >( N ) * Stride(); }

	static constexpr I     Count() noexcept     { return Index_t( N ); }
	static constexpr bool  Empty() noexcept     { return N == I( 0 ); }

	// --------- raw base pointers ----------
	constexpr       T *Base()       noexcept    { return Empty() ? nullptr : m_Elements; }
	constexpr const T *Base() const noexcept    { return Empty() ? nullptr : m_Elements; }
	constexpr       T *Data()       noexcept    { return Base(); }
	constexpr const T *Data() const noexcept    { return Base(); }

	// --------- element access ----------
	static constexpr bool IsValidIndex( I i ) noexcept
	{
		return i != INVALID_INDEX;
	}

	constexpr T &At( I i )
	{
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( FIRST_INDEX <= i && i < N );

		return m_Elements[ i ];
	}

	constexpr const T &At( I i ) const
	{
		BALL_ASSERT( IsValidIndex( i ) );
		BALL_ASSERT( FIRST_INDEX <= i && i < N );

		return m_Elements[ i ];
	}

	constexpr T &operator[]( I i ) { return At( i ); }
	constexpr const T &operator[]( I i ) const { return At( i ); }

	constexpr T &Front()
	{
		static_assert( N > FIRST_INDEX );

		return m_Elements[ 0 ];
	}

	constexpr const T &Front() const
	{
		static_assert( N > FIRST_INDEX );

		return m_Elements[ 0 ];
	}

	constexpr T &Back()
	{
		static_assert( N > FIRST_INDEX );

		return m_Elements[ N - 1 ];
	}

	constexpr const T &Back() const
	{
		static_assert( N > FIRST_INDEX );

		return m_Elements[ N - 1 ];
	}

	// --------- iterators ----------
	constexpr       T *begin()       noexcept   { return Base(); }
	constexpr       T *end()         noexcept   { return ( N == 0 ) ? nullptr : ( m_Elements + N ); }
	constexpr const T *begin() const noexcept   { return Base(); }
	constexpr const T *end()   const noexcept   { return ( N == 0 ) ? nullptr : ( m_Elements + N ); }
	constexpr const T *cbegin() const noexcept  { return Base(); }
	constexpr const T *cend()   const noexcept  { return ( N == 0 ) ? nullptr : ( m_Elements + N ); }

	// --------- algorithms (no STL) ----------
	/// @brief Fill all slots with the provided value.
	constexpr void Fill( const T &value ) noexcept
	{
		for ( I i( 0 ); i < I( N ); ++i )
			m_Elements[ i ] = value;
	}

	/// @brief Set a single index.
	constexpr void Set( I i, const T &value ) noexcept
	{
		BALL_ASSERT( IsValidIndex( i ) );

		m_Elements[ i ] = value;
	}

	/// @brief Copy from an N-sized C array.
	constexpr void CopyFrom( const T ( &src )[ N ] ) noexcept
	{
		for ( I i( 0 ); i < I( N ); ++i )
			m_Elements[ i ] = src[ i ];
	}

	/// @brief Find first occurrence; returns INVALID_INDEX when not found.
	constexpr I Find( const T &value, const I iFrom = FIRST_INDEX ) const noexcept
	{
		if ( Empty() || iFrom >= I( N ) )
			return INVALID_INDEX;

		for ( I i = iFrom; i < I( N ); ++i )
			if ( m_Elements[ i ] == value )
				return i;

		return INVALID_INDEX;
	}

	/// @brief Find last occurrence; returns INVALID_INDEX when not found.
	constexpr I RFind( const T &value ) const noexcept
	{
		if ( Empty() )
			return INVALID_INDEX;

		for ( I i( N ); i > FIRST_INDEX; --i )
			if ( m_Elements[ i - 1 ] == value )
				return i - 1;

		return INVALID_INDEX;
	}

protected:
	// --------- storage ----------
	/// @brief Underlying fixed buffer. For N==0 the array is never dereferenced.
	T m_Elements[ N ];
}; // class CArray

template < typename T, size_t N > using Array_t =       CArray< size_t, T, N >;
template < typename T, uint8_t N > using Array8_t =     CArray< uint8_t, T, N >;
template < typename T, uint16_t N > using Array16_t =   CArray< uint16_t, T, N >;
template < typename T, uint32_t N > using Array32_t =   CArray< uint32_t, T, N  >;
template < typename T, uint64_t N > using Array64_t =   CArray< uint64_t, T, N >;

#endif // !defined( _INCLUDE_BALL_TYPES_ARRAY_HPP_ )
