#ifndef _INCLUDE_BALL_TYPES_VECTORITERATOR_HPP_
#	define _INCLUDE_BALL_TYPES_VECTORITERATOR_HPP_

#	pragma once

template < typename T >
class CVector_Packed_Reference
{
public:
	using Traits_t = MFixedMetadata< T >;

	constexpr CVector_Packed_Reference( bits_t iShift, uchar_t *pData ) noexcept : m_iShift( iShift ), m_pData( pData ) {}
	constexpr operator T() const noexcept { return Get(); }
	constexpr CVector_Packed_Reference &operator=( const T &value ) noexcept { Set( value ); return *this; }
	constexpr CVector_Packed_Reference &operator=( const CVector_Packed_Reference &other ) noexcept { return operator=( static_cast< T >( other ) ); }

private:
	constexpr T Get() const noexcept { return Traits_t::Packed_Get( m_pData, m_iShift ); }
	constexpr void Set( const T &value ) noexcept { Traits_t::Packed_Set( m_pData, m_iShift, value ); }

	bits_t m_iShift;
	uchar_t *m_pData;
};

template < typename I, typename T, bool CONST >
class CVector_Packed_Iterator
{
public:
	using Index_t = I;
	using Traits_t = MFixedMetadata< T >;
	using Data_t = Conditional_t< CONST, const uchar_t, uchar_t >;

	constexpr CVector_Packed_Iterator( Index_t i, Data_t *pData ) noexcept : m_iShift( Traits_t::Packed_BitShift( i ) ), m_pData( pData + Traits_t::Packed_ByteOffset( i ) ) {}
	constexpr decltype( auto ) operator*() const noexcept
	{
		if constexpr ( CONST )
			return Get();
		else
			return CVector_Packed_Reference< T >( m_iShift, m_pData );
	}
	constexpr CVector_Packed_Iterator &operator++() noexcept { return operator+=( 1 ); }
	constexpr CVector_Packed_Iterator operator++( int ) noexcept { CVector_Packed_Iterator copy( *this ); ++*this; return copy; }
	constexpr CVector_Packed_Iterator &operator--() noexcept { return operator-=( 1 ); }
	constexpr CVector_Packed_Iterator operator--( int ) noexcept { CVector_Packed_Iterator copy( *this ); --*this; return copy; }
	constexpr CVector_Packed_Iterator &operator+=( Index_t n ) noexcept
	{
		const size_t nBits = m_iShift + Traits_t::Packed_Bits( n );

		m_iShift = static_cast< bits_t >( nBits & 7 );
		m_pData += nBits >> 3;

		return *this;
	}
	constexpr CVector_Packed_Iterator &operator-=( Index_t n ) noexcept
	{
		const size_t nBits = Traits_t::Packed_Bits( n );

		m_pData -= nBits >> 3;

		const bits_t nShift = static_cast< bits_t >( nBits & 7 );

		if ( nShift <= m_iShift )
			m_iShift -= nShift;
		else
		{
			m_iShift = static_cast< bits_t >( m_iShift + 8 - nShift );
			--m_pData;
		}

		return *this;
	}
	constexpr CVector_Packed_Iterator operator+( Index_t n ) const noexcept { CVector_Packed_Iterator copy( *this ); return copy += n; }
	constexpr CVector_Packed_Iterator operator-( Index_t n ) const noexcept { CVector_Packed_Iterator copy( *this ); return copy -= n; }
	constexpr Index_t operator-( const CVector_Packed_Iterator &other ) const noexcept
	{
		const sintptr_t nBits = ( m_pData - other.m_pData ) * 8 + static_cast< sintptr_t >( m_iShift ) - static_cast< sintptr_t >( other.m_iShift );

		return static_cast< Index_t >( nBits / Traits_t::BITS );
	}
	constexpr decltype( auto ) operator[]( Index_t n ) const noexcept { return *( *this + n ); }
	constexpr bool operator==( const CVector_Packed_Iterator &other ) const noexcept { return m_pData == other.m_pData && m_iShift == other.m_iShift; }
	constexpr bool operator!=( const CVector_Packed_Iterator &other ) const noexcept { return !( *this == other ); }
	constexpr bool operator<( const CVector_Packed_Iterator &other ) const noexcept { return m_pData < other.m_pData || ( m_pData == other.m_pData && m_iShift < other.m_iShift ); }
	constexpr bool operator>( const CVector_Packed_Iterator &other ) const noexcept { return other < *this; }
	constexpr bool operator<=( const CVector_Packed_Iterator &other ) const noexcept { return !( other < *this ); }
	constexpr bool operator>=( const CVector_Packed_Iterator &other ) const noexcept { return !( *this < other ); }

private:
	constexpr T Get() const noexcept
	{
		return static_cast< T >( CVector_Packed_Reference< T >( m_iShift, const_cast< uchar_t * >( m_pData ) ) );
	}

	bits_t m_iShift;
	Data_t *m_pData;
};

#endif // !defined( _INCLUDE_BALL_TYPES_VECTORITERATOR_HPP_ )
