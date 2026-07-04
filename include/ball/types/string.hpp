#ifndef _INCLUDE_BALL_TYPES_STRING_HPP_
#	define _INCLUDE_BALL_TYPES_STRING_HPP_

#	pragma once

#	include "base/arch.h"
#	include "meta/xvalue.hpp"
#	include "math.hpp"
#	include "number.hpp"
#	include "stringview.hpp"
#	include "vector.hpp"

template < class B, typename I, typename T >
class CStringImpl : public CVectorImpl< B, I, T >
{
public:
	using Base_t = CVectorImpl< B, I, T >;
	using View_t = Base_t::View_t;
	using ConstView_t = Base_t::ConstView_t;
	template < I GN > using GrowableView_t = typename Base_t::template GrowableView_t< GN >;
	template < I GN > using ConstGrowableView_t = typename Base_t::template ConstGrowableView_t< GN >;

	using Base_t::Base_t;
	using Base_t::INVALID_INDEX;
	using Base_t::Empty;
	using Base_t::Base;
	using Base_t::Find;
	using Base_t::EnsureInsert;
	using Base_t::RemoveAll;

	constexpr I Length() const { return Base_t::Count(); }
	constexpr const T *String() const { return Empty() ? "\0" : Base(); }

protected:
	///-----------------------------------------------------------------------------
	/// @brief Insert unsigned integer @p u in base-NS at position @p nIndex.
	///        Uses EnsureInsert to do a single grow+shift and returns last written idx.
	///-----------------------------------------------------------------------------
	template < size8_t NS, typename U >
	constexpr I InsertUnsigned( I nIndex, U u )
	{
		static_assert( NS >= 2 && NS <= 36, "InsertUnsigned: base must be in [2,36]" );

		const U nDigits = Math_Digits< U, NS >( u );
		const I nDigitCount = static_cast< I >( nDigits );

		// Make a single hole once; returns pointer to the gap start.
		T *pGap = EnsureInsert( nIndex, nDigitCount );

		// Fill digits directly into the reserved gap.
		Num_WriteUnsigned< T, I, NS, U >( u, pGap, nDigitCount );

		// Return index of the last written character.
		return nIndex + nDigitCount;
	}

	///-----------------------------------------------------------------------------
	/// @brief Insert signed integer @p v in base-NS at position @p nIndex.
	///        Does a single EnsureInsert for sign + digits (when negative).
	///-----------------------------------------------------------------------------
	template < size8_t NS, typename S >
	constexpr I InsertSigned( I nIndex, S v )
	{
		using U = typename MUnsigned< S >::Type;

		if ( v < 0 )
		{
			const U nMag0 = U( v ), nMag = ( ~nMag0 ) + U( 1 ); // two's complement magnitude

			const U nDigits = Math_Digits< U, NS >( nMag );
			const I nDigitCount = static_cast< I >( nDigits );
			const I nTotal = 1 + nDigitCount; // '-' + digits

			T *pGap = EnsureInsert( nIndex, nTotal );

			pGap[ 0 ] = static_cast< T >( '-' );
			Num_WriteUnsigned< T, I, NS, U >( nMag, pGap + 1, nDigitCount );

			return nIndex + nTotal - 1;
		}
		else
		{
			const U u = static_cast< U >( v );
			const U nDigits = Math_Digits< U, NS >( u );
			const I nDigitCount = static_cast< I >( nDigits );

			T *pGap = EnsureInsert( nIndex, nDigitCount );

			Num_WriteUnsigned< T, I, NS, U >( u, pGap, nDigitCount );

			return nIndex + nDigitCount;
		}
	}

	///-----------------------------------------------------------------------------
	/// @brief Convenience wrappers to keep old call sites working.
	///-----------------------------------------------------------------------------
	template < typename U > constexpr I InsertUnsigned8( I nIndex, U u )  { return InsertUnsigned< 8u, U >( nIndex, u ); }
	template < typename U > constexpr I InsertUnsigned10( I nIndex, U u ) { return InsertUnsigned< 10u, U >( nIndex, u ); }
	template < typename U > constexpr I InsertUnsigned16( I nIndex, U u ) { return InsertUnsigned< 16u, U >( nIndex, u ); }

	template < typename S > constexpr I InsertSigned8( I nIndex, S v )  { return InsertSigned< 8u, S >( nIndex, v ); }
	template < typename S > constexpr I InsertSigned10( I nIndex, S v ) { return InsertSigned< 10u, S >( nIndex, v ); }
	template < typename S > constexpr I InsertSigned16( I nIndex, S v ) { return InsertSigned< 16u, S >( nIndex, v ); }


	/// @brief Appends a floating-point value with fixed precision.
	/// @tparam P Number of digits after decimal point.
	/// @tparam F Floating type (float or double).
	template < uint_t P, typename F >
	constexpr I InsertFloatFixed( I nIndex, F x )
	{
		if ( x < F( 0 ) )
		{
			nIndex = Insert( nIndex, static_cast< T >( '-' ) );

			x = -x;
		}

		llong_t nBase = static_cast< llong_t >( x );

		nIndex = InsertSigned10< llong_t >( nIndex, nBase );

		if constexpr ( P == 0 )
			return nIndex;

		nIndex = Insert( nIndex, static_cast< T >('.') );

		const ullong_t nScale = Math_Pow10< P >();

		F flFrac = x - static_cast< F >( nBase );

		ullong_t f = static_cast< ullong_t >( flFrac * static_cast< F >( nScale ) + F( 0.5 ) );

		if ( f >= nScale )
		{
			f = 0ull;
		}

		ullong_t nDiv = nScale / 10ull;

		for ( uint_t n = 0; n < P; ++n, nDiv /= 10ull )
		{
			uint_t nDigit = static_cast< uint_t >( ( f / ( nDiv ? nDiv : 1ull ) ) % 10ull );

			nIndex = Insert( nIndex, static_cast< T >( '0' + static_cast< int >( nDigit ) ) );
		}

		return nIndex;
	}

public:
	constexpr I Insert( I i, T character )                                          { return Base_t::Insert( i, character ); }
	constexpr I Insert( I i, const View_t &sv )                                     { return Base_t::Insert( i, sv ); }
	constexpr I Insert( I i, const ConstView_t &sv )                                { return Base_t::Insert( i, sv ); }
	template < I N > constexpr I Insert( I i, const T ( &str )[ N ] )               { return Base_t::template InsertArray< N >( i, N - 1, str ); }
	template < I N > constexpr I Insert( I i, T ( &&str )[ N ] )                    { return Base_t::template InsertArray< N >( i, N - 1, Move( str ) ); } // 
	constexpr I Insert( I i, I nLength, const T *pString )                          { return Insert( i, ConstView_t( nLength, pString ) ); }
	constexpr I Insert( I i, bool_t b )                                             { return b ? Insert( i, "true" ) : Insert( i, "false" ); }
	constexpr I Insert( I i, int_t v )                                              { return InsertSigned10( i, v ); }
	constexpr I Insert( I i, long_t v )                                             { return InsertSigned10( i, v ); }
	constexpr I Insert( I i, llong_t v )                                            { return InsertSigned10( i, v ); }
	constexpr I Insert( I i, uint_t v )                                             { return InsertUnsigned10( i, v ); }
	constexpr I Insert( I i, ulong_t v )                                            { return InsertUnsigned10( i, v ); }
	constexpr I Insert( I i, ullong_t v )                                           { return InsertUnsigned10( i, v ); }
	template < I P = 6 > constexpr I Insert( I i, float_t x )                       { return InsertFloatFixed< P >( i, x ); }
	template < I P = 6 > constexpr I Insert( I i, double_t x )                      { return InsertFloatFixed< P >( i, x ); }
	constexpr I Insert( I i, const void *p )                                        { return InsertUnsigned16( Insert( i, "0x" ), reinterpret_cast< uintptr_t >( p ) ); }
	template < typename ...Ts > constexpr I InsertMultiple( I i, Ts &&...args )     { ( ( i = Insert( i, Forward< Ts >( args ) ) ), ... ); return i; }

	template < typename ...Ts > constexpr I Set( Ts &&...args )                     { RemoveAll(); return Insert( 0, Forward< Ts >( args )... ); }
	template < typename ...Ts > constexpr I SetMultiple( Ts &&...args )             { RemoveAll(); InsertMultiple( 0, Forward< Ts >( args )... ); return Length(); }

	template < typename ...Ts > constexpr CStringImpl &operator=( Ts &&...args )    { Set( Forward< Ts >( args )... ); return *this; }

	template < typename ...Ts > constexpr I Append( Ts &&...args )                  { return Insert( Length(), Forward< Ts >( args )... ); }
	template < typename ...Ts > constexpr I AppendMultiple( Ts &&...args )          { InsertMultiple( Length(), Forward< Ts >( args )... ); return Length(); }

	template < typename ...Ts > constexpr CStringImpl &operator+=( Ts &&...args )   { Append( Forward< Ts >( args )... ); return *this; }

public:
	///-----------------------------------------------------------------------------
	/// @brief Trim leading ASCII whitespace in-place.
	///-----------------------------------------------------------------------------
	constexpr I TrimLeft() noexcept
	{
		const T *p = Base();
		const I nLength = Length();

		I n = 0;

		for ( ; n < nLength; ++n )
			if ( !ConstView_t::IsSpaceASCII( p[ n ] ) )
				break;

		if ( n == 0 )
			return nLength;

		return Set( ConstView_t( nLength - n, p + n ) );
	}

	///-----------------------------------------------------------------------------
	/// @brief Trim trailing ASCII whitespace in-place.
	///-----------------------------------------------------------------------------
	constexpr I TrimRight() noexcept
	{
		const T *p = Base();
		const I nLength  = Length();

		if ( nLength == I( 0 ) )
			return I( 0 );

		I n = nLength - 1;

		for ( ; ; )
		{
			if ( !ConstView_t::IsSpaceASCII( p[ n ] ) )
				break;

			if ( n == I( 0 ) )
			{
				RemoveAll();

				return I( 0 );
			}

			--n;
		}

		return Set( ConstView_t( n + 1, p ) );
	}

	///-----------------------------------------------------------------------------
	/// @brief Trim both ends. Returns new length.
	///-----------------------------------------------------------------------------
	constexpr I Trim() noexcept
	{
		TrimRight();

		return TrimLeft();
	}
};

template < typename I, typename T, I N, class A > class CBufferString;

template < typename I = size_t, typename T = char_t, class A = CAllocator< I, T > >
class CString : public CStringImpl< CVectorBase< CStringView< I, T >, I, T, A >, I, T >
{
public:
	using Base_t = CStringImpl< CVectorBase< CStringView< I, T >, I, T >, I, T >;
	using Base_t::Base_t;
	using Base_t::CopyFrom;

	template < I N > constexpr CString( const CBufferString< I, T, N, A > &other ) { CopyFrom< N >( other ); }
	template < I N > constexpr CString &operator=( const CBufferString< I, T, N, A > &other ) { return CopyFrom< N >( other ); }
};

template < typename I, typename T, I N, class A = CAllocator< I, T > >
class CBufferString : public CStringImpl< CVectorBase< CStringView< I, T, N >, I, T, A >, I, T >
{
public:
	using Base_t = CStringImpl< CVectorBase< CStringView< I, T, N >, I, T >, I, T >;
	using Base_t::Base_t;
	using Base_t::CopyFrom;

	constexpr CBufferString( const CString< I, T > &other ) { CopyFrom( other ); }
	constexpr CBufferString &operator=( const CString< I, T > &other ) { return CopyFrom( other ); }
};

using String_t =        CString< size_t, char_t >;
using String8_t =       CString< size8_t, char_t >;
using String16_t =      CString< size16_t, char_t >;
using String32_t =      CString< size32_t, char_t >;
using String64_t =      CString< size64_t, char_t >;

using WString_t =       CString< size_t, wchar_t >;
using WString8_t =      CString< size8_t, wchar_t >;
using WString16_t =     CString< size16_t, wchar_t >;
using WString32_t =     CString< size32_t, wchar_t >;
using WString64_t =     CString< size64_t, wchar_t >;

using UTF8String_t =    CString< size_t, char8_t >;
using UTF8String8_t =   CString< size8_t, char8_t >;
using UTF8String16_t =  CString< size16_t, char8_t >;
using UTF8String32_t =  CString< size32_t, char8_t >;
using UTF8String64_t =  CString< size64_t, char8_t >;

using UTF16String_t =   CString< size_t, char16_t >;
using UTF16String8_t =  CString< size8_t, char16_t >;
using UTF16String16_t = CString< size16_t, char16_t >;
using UTF16String32_t = CString< size32_t, char16_t >;
using UTF16String64_t = CString< size64_t, char16_t >;

using UTF32String_t =   CString< size_t, char32_t >;
using UTF32String8_t =  CString< size8_t, char32_t >;
using UTF32String16_t = CString< size16_t, char32_t >;
using UTF32String32_t = CString< size32_t, char32_t >;
using UTF32String64_t = CString< size64_t, char32_t >;

template < size_t N >   using BufferString_t =           CBufferString< size_t, char_t, N >;
template < size8_t N >  using BufferString8_t =          CBufferString< size8_t, char_t, N >;
template < size16_t N > using BufferString16_t =         CBufferString< size16_t, char_t, N >;
template < size32_t N > using BufferString32_t =         CBufferString< size32_t, char_t, N >;
template < size64_t N > using BufferString64_t =         CBufferString< size64_t, char_t, N >;

template < size_t N >   using WBufferString_t =          CBufferString< size_t, wchar_t, N >;
template < size8_t N >  using WBufferString8_t =         CBufferString< size8_t, wchar_t, N >;
template < size16_t N > using WBufferString16_t =        CBufferString< size16_t, wchar_t, N >;
template < size32_t N > using WBufferString32_t =        CBufferString< size32_t, wchar_t, N >;
template < size64_t N > using WBufferString64_t =        CBufferString< size64_t, wchar_t, N >;

template < size_t N >   using UTF8BufferString_t =       CBufferString< size_t, char8_t, N >;
template < size8_t N >  using UTF8BufferString8_t =      CBufferString< size8_t, char8_t, N >;
template < size16_t N > using UTF8BufferString16_t =     CBufferString< size16_t, char8_t, N >;
template < size32_t N > using UTF8BufferString32_t =     CBufferString< size32_t, char8_t, N >;
template < size64_t N > using UTF8BufferString64_t =     CBufferString< size64_t, char8_t, N >;

template < size_t N >   using UTF16BufferString_t =      CBufferString< size_t, char16_t, N >;
template < size8_t N >  using UTF16BufferString8_t =     CBufferString< size8_t, char16_t, N >;
template < size16_t N > using UTF16BufferString16_t =    CBufferString< size16_t, char16_t, N >;
template < size32_t N > using UTF16BufferString32_t =    CBufferString< size32_t, char16_t, N >;
template < size64_t N > using UTF16BufferString64_t =    CBufferString< size64_t, char16_t, N >;

template < size_t N >   using UTF32BufferString_t =      CBufferString< size_t, char32_t, N >;
template < size8_t N >  using UTF32BufferString8_t =     CBufferString< size8_t, char32_t, N >;
template < size16_t N > using UTF32BufferString16_t =    CBufferString< size16_t, char32_t, N >;
template < size32_t N > using UTF32BufferString32_t =    CBufferString< size32_t, char32_t, N >;
template < size64_t N > using UTF32BufferString64_t =    CBufferString< size64_t, char32_t, N >;

#endif // !defined( _INCLUDE_BALL_TYPES_STRING_HPP_ )
