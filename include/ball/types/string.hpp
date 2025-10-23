#ifndef _INCLUDE_BALL_TYPES_BUFFERSTRING_HPP_
#	define _INCLUDE_BALL_TYPES_BUFFERSTRING_HPP_

#	include "base/arch.h"
#	include "vector.hpp"
#	include "math.hpp"
#	include "number.hpp"
#	include "stringview.hpp"
#	include "xvalue.hpp"

template < class B, typename I, typename T >
class CStringImpl : public CVectorImpl< B, I, T >
{
public:
	using Base_t =      CVectorImpl< B, I, T >;
	using View_t =      Base_t::View_t;
	using ConstView_t = Base_t::ConstView_t;

	using Base_t::Base_t;
	using Base_t::INVALID_INDEX;
	using Base_t::Empty;
	using Base_t::Find;
	using Base_t::Base;
	using Base_t::RemoveAll;

	I Length() const { return Base_t::Count(); }
	const T *String() const { return Empty() ? "\0" : Base(); }

protected:
	///-----------------------------------------------------------------------------
	/// @brief Insert unsigned integer @p u in base-NS at position @p nIndex.
	///        Uses EnsureInsert to do a single grow+shift and returns last written idx.
	///-----------------------------------------------------------------------------
	template < uint8_t NS, typename U >
	I InsertUnsigned( I nIndex, U u )
	{
		static_assert( NS >= 2 && NS <= 36, "InsertUnsigned: base must be in [2,36]" );

		const uint8_t nDigits = Math_Digits< uint8_t, NS, U >( u );

		// Make a single hole once; returns pointer to the gap start.
		T *pGap = Base_t::EnsureInsert( nIndex, static_cast< I >( nDigits ) );

		// Fill digits directly into the reserved gap.
		Num_WriteUnsigned< T, I, NS, U >( u, pGap, static_cast< I >( nDigits ) );

		// Return index of the last written character.
		return nIndex + static_cast< I >( nDigits );
	}

	///-----------------------------------------------------------------------------
	/// @brief Insert signed integer @p v in base-NS at position @p nIndex.
	///        Does a single EnsureInsert for sign + digits (when negative).
	///-----------------------------------------------------------------------------
	template < uint8_t NS, typename S >
	I InsertSigned( I nIndex, S v )
	{
		using U = typename MUnsigned< S >::Type;

		if ( v < 0 )
		{
			const U nMag0 = U( v ), nMag = ( ~nMag0 ) + U( 1 ); // two's complement magnitude

			const uint8_t nDigits = Math_Digits< uint8_t, NS, U >( nMag );
			const I nTotal = static_cast< I >( 1 + nDigits ); // '-' + digits

			T *pGap = Base_t::EnsureInsert( nIndex, nTotal );

			pGap[ 0 ] = static_cast< T >( '-' );
			Num_WriteUnsigned< T, I, NS, U >( nMag, pGap + 1, static_cast< I >( nDigits ) );

			return nIndex + nTotal - 1;
		}
		else
		{
			const U u = static_cast< U >( v );
			const uint8_t nDigits = Math_Digits< uint8_t, static_cast< uint8_t >( NS ), U >( u );

			T *pGap = Base_t::EnsureInsert( nIndex, static_cast< I >( nDigits ) );

			Num_WriteUnsigned< T, I, NS, U >( u, pGap, static_cast< I >( nDigits ) );

			return nIndex + static_cast< I >( nDigits );
		}
	}

	///-----------------------------------------------------------------------------
	/// @brief Convenience wrappers to keep old call sites working.
	///-----------------------------------------------------------------------------
	template < typename U > I InsertUnsigned8( I nIndex, U u ) { return InsertUnsigned< 8u, U >( nIndex, u ); }
	template < typename U > I InsertUnsigned10( I nIndex, U u ) { return InsertUnsigned< 10u, U >( nIndex, u ); }
	template < typename U > I InsertUnsigned16( I nIndex, U u ) { return InsertUnsigned< 16u, U >( nIndex, u ); }

	template < typename S > I InsertSigned8( I nIndex, S v ) { return InsertSigned< 8u, S >( nIndex, v ); }
	template < typename S > I InsertSigned10( I nIndex, S v ) { return InsertSigned< 10u, S >( nIndex, v ); }
	template < typename S > I InsertSigned16( I nIndex, S v ) { return InsertSigned< 16u, S >( nIndex, v ); }


	/// @brief Appends a floating-point value with fixed precision.
	/// @tparam P Number of digits after decimal point.
	/// @tparam F Floating type (float or double).
	template < uint_t P, typename F >
	I InsertFloatFixed( I nIndex, F x )
	{
		if ( x < F( 0 ) )
		{
			nIndex = Insert( nIndex, static_cast< T >( '-' ) );
			nIndex++;

			x = -x;
		}

		llong_t nBase = static_cast< llong_t >( x );

		nIndex = InsertSigned10< llong_t >( nIndex, nBase );
		nIndex++;

		if constexpr ( P == 0 )
			return nIndex;

		nIndex = Insert( nIndex, static_cast< T >('.') );
		nIndex++;

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
			nIndex++;
		}

		return nIndex;
	}

public:
	I Insert( I i, T character )                                        { return Base_t::Insert( i, character ); }
	template < size_t N > I Insert( I i, const T ( &str )[ N ] )        { return Base_t::Insert( i, N - 1, str ); }
	template < size_t N > I Insert( I i, T ( &&str )[ N ] )             { return Base_t::Insert( i, N - 1, Move( str ) ); }
	I Insert( I i, View_t sv )                                          { return Base_t::Insert( i, sv ); }
	I Insert( I i, ConstView_t sv )                                     { return Base_t::Insert( i, sv ); }
	I Insert( I i, I nLength, const T *pString )                        { return Insert( i, ConstView_t( nLength, pString ) ); }
	I Insert( I i, const T *pString )                                   { return Insert( i, ConstView_t( pString ) ); }
	I Insert( I i, bool_t b )                                           { return b ? Insert( i, "true" ) : Insert( i, "false" ); }
	I Insert( I i, int_t v )                                            { return InsertSigned10( i, v ); }
	I Insert( I i, long_t v )                                           { return InsertSigned10( i, v ); }
	I Insert( I i, llong_t v )                                          { return InsertSigned10( i, v ); }
	I Insert( I i, uint_t v )                                           { return InsertUnsigned10( i, v ); }
	I Insert( I i, ulong_t v )                                          { return InsertUnsigned10( i, v ); }
	I Insert( I i, ullong_t v )                                         { return InsertUnsigned10( i, v ); }
	template < size_t P = 6 > I Insert( I i, float_t x )                { return InsertFloatFixed< P >( i, x ); }
	template < size_t P = 6 > I Insert( I i, double_t x )               { return InsertFloatFixed< P >( i, x ); }
	I Insert( I i, const void *p )                                      { return InsertUnsigned16( Insert( i, "0x" ), reinterpret_cast< uintptr_t >( p ) ); }
	template < typename ...Ts > I InsertMultiple( I i, Ts &&...args )   { ( ( i = Insert( i, Forward< Ts >( args ) ) ), ... ); return i; }

	template < typename ...Ts > I Set( Ts &&...args )                   { RemoveAll(); return Insert( 0, Forward< Ts >( args )... ); }
	template < typename ...Ts > I SetMultiple( Ts &&...args )           { RemoveAll(); InsertMultiple( 0, Forward< Ts >( args )... ); return Length(); }

	template < typename ...Ts > CStringImpl &operator=( Ts &&...args )  { Set( Forward< Ts >( args )... ); return *this; }

	template < typename ...Ts > I Append( Ts &&...args )                { return Insert( Length(), Forward< Ts >( args )... ); }
	template < typename ...Ts > I AppendMultiple( Ts &&...args )        { InsertMultiple( Length(), Forward< Ts >( args )... ); return Length(); }

	template < typename ...Ts > CStringImpl &operator+=( Ts &&...args ) { Append( Forward< Ts >( args )... ); return *this; }

public:
	///-----------------------------------------------------------------------------
	/// @brief Replace a range [index, index + nRemove) with @p svRepl in-place.
	///        No aliasing with temporaries: we shift the suffix explicitly and
	///        copy the replacement into its final position.
	///        Returns index of the last written character of the inserted part
	///        (or prefix end - 1 if svRepl is empty and string shrinks).
	///-----------------------------------------------------------------------------
	I Replace( I i, I nRemove, ConstView_t svRepl )
	{
		const I n = Length();

		BALL_ASSERT( i <= n );
		BALL_ASSERT( nRemove <= ( n - i ) );

		const I nWith   = svRepl.Count();
		const I nDelta  = nWith - nRemove;          // can be negative
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
			const I nShrink = -nDelta;              // amount to pull left
			// 2.1 Copy replacement into its final spot [i .. i + nWith)
			for ( I k = 0; k < nWith; ++k )
				pData[ i + k ] = svRepl[ k ];

			// 2.2 Shift suffix left by nShrink.
			const I nSuffixBegin  = i + nRemove;
			const I nOldCount     = n;

			ShiftElements( &pData[ i + nWith ], &pData[ nSuffixBegin ], &pData[ nOldCount ] );

			// 2.3 Commit new logical size.
			Base_t::Set( n - nShrink, pData );

			return ( nWith == I( 0 ) ) ? ( ( i == I( 0 ) ) ? INVALID_INDEX : i )
			                           : ( i + nWith );
		}

		// Case 3: grow (nWith > nRemove) -> ensure capacity, shift suffix right, then copy.
		{
			const I nGrow = nDelta;                 // amount to push right
			const I nNew  = n + nGrow;

			// 3.1 Ensure capacity; pointer may change.
			pData = Base_t::EnsureCapacity( nNew );

			// 3.2 Shift suffix right by nGrow: [i + nRemove .. n) -> starts at [i + nWith .. )
			const I nSuffixBegin  = i + nRemove;
			const I nOldCount     = n;

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
	I Replace( I i, I nRemove, const T *pRepl )
	{
		// Treat nullptr as empty replacement.
		return Replace( i, nRemove, pRepl ? ConstView_t( pRepl ) : ConstView_t() );
	}

	///-----------------------------------------------------------------------------
	/// @brief Replace first occurrence of @p what with @p with. Returns index of replaced range start or INVALID_INDEX.
	///-----------------------------------------------------------------------------
	I ReplaceFirst( ConstView_t svWhat, ConstView_t svWith )
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
	I ReplaceAll( ConstView_t svWhat, ConstView_t svWith )
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
			const I nLength = Length();

			if ( nFrom > nLength )
				break;
		}

		return nReplaced;
	}

	///-----------------------------------------------------------------------------
	/// @brief Replace all occurrences of single character @p fromCh with @p toCh.
	///        Returns count of replacements.
	///-----------------------------------------------------------------------------
	I ReplaceChar( T fromCh, T toCh )
	{
		T *p = const_cast< T * >( Base() );
		const I nLength = Length();

		I n = 0;

		for ( I k = 0; k < nLength; ++k )
		{
			if ( p[ k ] == fromCh )
			{
				p[ k ] = toCh;
				++n;
			}
		}

		return n;
	}

public:
	///-----------------------------------------------------------------------------
	/// @brief Trim leading ASCII whitespace in-place.
	///-----------------------------------------------------------------------------
	I TrimLeft() noexcept
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
	I TrimRight() noexcept
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
	I Trim() noexcept
	{
		TrimRight();

		return TrimLeft();
	}
};

template < typename I, typename T, I N > class CBufferString;

template < typename I = size_t, typename T = char >
class CString : public CStringImpl< CVectorBase< CStringView< I, T >, I, T >, I, T >
{
public:
	using Base_t = CStringImpl< CVectorBase< CStringView< I, T >, I, T >, I, T >;
	using Base_t::Base_t;

	template < I N >
	CString( const CBufferString< I, T, N > &other ) :
		Base_t()
	{
		Base_t::CopyFrom( other.View() );
	}
};

template < typename I, typename T, I N >
class CBufferString : public CStringImpl< CVectorBase_Growable< CStringView< I, T >, I, T, N >, I, T >
{
public:
	using Base_t = CStringImpl< CVectorBase_Growable< CStringView< I, T >, I, T, N >, I, T >;
	using Base_t::Base_t;

	CBufferString( const CString< I, T > &other ) :
		Base_t()
	{
		Base_t::CopyFrom( other.View() );
	}
};

using String_t =        CString< size_t, char_t >;
using String8_t =       CString< uint8_t, char_t >;
using String16_t =      CString< uint16_t, char_t >;
using String32_t =      CString< uint32_t, char_t >;
using String64_t =      CString< uint64_t, char_t >;

using WString_t =       CString< size_t, wchar_t >;
using WString8_t =      CString< uint8_t, wchar_t >;
using WString16_t =     CString< uint16_t, wchar_t >;
using WString32_t =     CString< uint32_t, wchar_t >;
using WString64_t =     CString< uint64_t, wchar_t >;

using UTF8String_t =    CString< size_t, char8_t >;
using UTF8String8_t =   CString< uint8_t, char8_t >;
using UTF8String16_t =  CString< uint16_t, char8_t >;
using UTF8String32_t =  CString< uint32_t, char8_t >;
using UTF8String64_t =  CString< uint64_t, char8_t >;

using UTF16String_t =   CString< size_t, char16_t >;
using UTF16String8_t =  CString< uint8_t, char16_t >;
using UTF16String16_t = CString< uint16_t, char16_t >;
using UTF16String32_t = CString< uint32_t, char16_t >;
using UTF16String64_t = CString< uint64_t, char16_t >;

using UTF32String_t =   CString< size_t, char32_t >;
using UTF32String8_t =  CString< uint8_t, char32_t >;
using UTF32String16_t = CString< uint16_t, char32_t >;
using UTF32String32_t = CString< uint32_t, char32_t >;
using UTF32String64_t = CString< uint64_t, char32_t >;

template < size_t N >   using BufferString_t =           CBufferString< size_t, char_t, N >;
template < uint8_t N >  using BufferString8_t =          CBufferString< uint8_t, char_t, N >;
template < uint16_t N > using BufferString16_t =         CBufferString< uint16_t, char_t, N >;
template < uint32_t N > using BufferString32_t =         CBufferString< uint32_t, char_t, N >;
template < uint64_t N > using BufferString64_t =         CBufferString< uint64_t, char_t, N >;

template < size_t N >   using WBufferString_t =          CBufferString< size_t, wchar_t, N >;
template < uint8_t N >  using WBufferString8_t =         CBufferString< uint8_t, wchar_t, N >;
template < uint16_t N > using WBufferString16_t =        CBufferString< uint16_t, wchar_t, N >;
template < uint32_t N > using WBufferString32_t =        CBufferString< uint32_t, wchar_t, N >;
template < uint64_t N > using WBufferString64_t =        CBufferString< uint64_t, wchar_t, N >;

template < size_t N >   using UTF8BufferString_t =       CBufferString< size_t, char8_t, N >;
template < uint8_t N >  using UTF8BufferString8_t =      CBufferString< uint8_t, char8_t, N >;
template < uint16_t N > using UTF8BufferString16_t =     CBufferString< uint16_t, char8_t, N >;
template < uint32_t N > using UTF8BufferString32_t =     CBufferString< uint32_t, char8_t, N >;
template < uint64_t N > using UTF8BufferString64_t =     CBufferString< uint64_t, char8_t, N >;

template < size_t N >   using UTF16BufferString_t =      CBufferString< size_t, char16_t, N >;
template < uint8_t N >  using UTF16BufferString8_t =     CBufferString< uint8_t, char16_t, N >;
template < uint16_t N > using UTF16BufferString16_t =    CBufferString< uint16_t, char16_t, N >;
template < uint32_t N > using UTF16BufferString32_t =    CBufferString< uint32_t, char16_t, N >;
template < uint64_t N > using UTF16BufferString64_t =    CBufferString< uint64_t, char16_t, N >;

template < size_t N >   using UTF32BufferString_t =      CBufferString< size_t, char32_t, N >;
template < uint8_t N >  using UTF32BufferString8_t =     CBufferString< uint8_t, char32_t, N >;
template < uint16_t N > using UTF32BufferString16_t =    CBufferString< uint16_t, char32_t, N >;
template < uint32_t N > using UTF32BufferString32_t =    CBufferString< uint32_t, char32_t, N >;
template < uint64_t N > using UTF32BufferString64_t =    CBufferString< uint64_t, char32_t, N >;

#endif // !defined( _INCLUDE_BALL_TYPES_BUFFERSTRING_HPP_ )
