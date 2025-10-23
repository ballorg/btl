#ifndef _INCLUDE_BALL_TYPES_STRINGVIEW_HPP_
#	define _INCLUDE_BALL_TYPES_STRINGVIEW_HPP_

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "meta/issame.hpp"
#	include "meta/removecv.hpp"
#	include "memoryview.hpp"
#	include "elements.hpp"

template < typename I = size_t, typename T = char >
class CStringView : public CMemoryView< I, T >
{
public:
	using Base_t = CMemoryView< I, T >;
	using Const_t = const CStringView< I, const T >;

	using Base_t::Get;

	static constexpr I INVALID_INDEX = Base_t::INVALID_INDEX;

	//------------------ Constructors ------------------

	explicit constexpr CStringView( I nLength, T *pStr ) noexcept : Base_t( nLength, pStr ) {}
	constexpr CStringView() noexcept : CStringView( I( 0 ), nullptr ) {}

	/// @brief Construct from zero-terminated string.
	constexpr CStringView( const T *pStr ) noexcept
		: Base_t( Length( pStr ), pStr )
	{
	}

	/// @brief Construct from a C-array (string literal).
	template < size_t N >
	constexpr CStringView( const T ( &str )[ N ] ) noexcept : 
		Base_t( N - 1, reinterpret_cast< const T *>( str ) )
	{
	}

	template < size_t N >
	constexpr CStringView( T ( &&str )[ N ] ) noexcept : 
		Base_t( N - 1, reinterpret_cast< const T *>( str ) )
	{
	}

	constexpr CStringView( T *pBegin, const T *pEnd ) noexcept : 
		Base_t( pBegin, pEnd )
	{
	}

	//------------------ Methods ------------------

	/// @brief strlen for zero-terminated strings.
	static constexpr I Length( const T *pString ) noexcept
	{
		if ( pString == nullptr )
			return I( 0 );

		using RCV_T = RemoveCV_t<T>;

		if constexpr ( IS_SAME< RCV_T, char8_t > )
		{
			return static_cast< I >( __builtin_u8strlen( pString ) );
		}
		else if constexpr ( IS_SAME< RCV_T, char > )
		{
			return static_cast< I >( __builtin_strlen( pString ) );
		}
		else if constexpr ( IS_SAME< RCV_T, wchar_t > )
		{
			return static_cast< I >( __builtin_wcslen( pString ) );
		}
		else
		{
			// For char16_t/char32_t (and any other T), use a portable generic loop.
			I n = 0;

			while ( pString[ n ] != T( '\0' ) )
				++n;

			return n;
		}
	}
	constexpr I Length() const noexcept { return Base_t::Count(); }

	static constexpr const T *EmptyString() noexcept { return reinterpret_cast< const T * >( "" ); }
	static constexpr const T *NullString() noexcept { return reinterpret_cast< const T * >( "(null)" ); }
	constexpr const T *String() const noexcept
	{
		const T *p = Base_t::Get();

		return p ? p : EmptyString();
	}

	static bool_t IsSpaceASCII( T ch ) noexcept
	{
		return ch == static_cast< T >( ' ' ) 
		    || ch == static_cast< T >( '\t' ) 
		    || ch == static_cast< T >( '\n' ) 
		    || ch == static_cast< T >( '\r' );
	}

	constexpr CStringView SubString( I nPos, I nCount = INVALID_INDEX ) const noexcept
	{
		if ( nPos >= Length() )
			return CStringView();

		I nMaxCount = Length() - nPos;
		I nTake = ( nCount == INVALID_INDEX || nCount > nMaxCount ) ? nMaxCount : nCount;

		return CStringView( nTake, String() + nPos );
	}

	constexpr CStringView RemovePrefix( I nCount ) const noexcept
	{
		return ( nCount >= Length() )
			? CStringView()
			: CStringView( Length() - nCount, String() + nCount );
	}

	constexpr CStringView RemoveSuffix( I nCount ) const noexcept
	{
		return ( nCount >= Length() )
			? CStringView()
			: CStringView( Length() - nCount, String() );
	}

	/// @brief constexpr memcmp implementation.
	static constexpr int Compare( const T *pLeft, const T *pRight, I nLength ) noexcept
	{
		for ( I i = 0; i < nLength; ++i )
		{
			if ( pLeft[ i ] < pRight[ i ] ) return -1;
			if ( pLeft[ i ] > pRight[ i ] ) return  1;
		}

		return 0;
	}

	constexpr int8_t Compare( CStringView rhs ) const noexcept
	{
		const I nLength = Length();
		const I nOtherLength = rhs.Length();
		const I nMin = ( nLength < nOtherLength ) ? nLength : nOtherLength;

		if ( nMin > 0 )
		{
			int8_t r = Compare( String(), rhs.String(), nMin );

			if ( r != 0 )
				return r;
		}

		return ( nLength < nOtherLength ) ? -1 : ( nLength > nOtherLength ? 1 : 0 );
	}

	constexpr bool Equals( CStringView rhs ) const noexcept
	{
		const I n = Length();

		return ( n == rhs.Length() ) && ( n == I( 0 ) || Compare( String(), rhs.String(), n ) == 0 );
	}
};

using StringView_t =        const CStringView< size_t, const char_t >;
using StringView8_t =       const CStringView< uint8_t, const char_t >;
using StringView16_t =      const CStringView< uint16_t, const char_t >;
using StringView32_t =      const CStringView< uint32_t, const char_t >;
using StringView64_t =      const CStringView< uint64_t, const char_t >;

using WStringView_t =       const CStringView< size_t, const wchar_t >;
using WStringView8_t =      const CStringView< uint8_t, const wchar_t >;
using WStringView16_t =     const CStringView< uint16_t, const wchar_t >;
using WStringView32_t =     const CStringView< uint32_t, const wchar_t >;
using WStringView64_t =     const CStringView< uint64_t, const wchar_t >;

using UTF8StringView_t =    const CStringView< size_t, const char8_t >;
using UTF8StringView8_t =   const CStringView< uint8_t, const char8_t >;
using UTF8StringView16_t =  const CStringView< uint16_t, const char8_t >;
using UTF8StringView32_t =  const CStringView< uint32_t, const char8_t >;
using UTF8StringView64_t =  const CStringView< uint64_t, const char8_t >;

using UTF16StringView_t =   const CStringView< size_t, const char16_t >;
using UTF16StringView8_t =  const CStringView< uint8_t, const char16_t >;
using UTF16StringView16_t = const CStringView< uint16_t, const char16_t >;
using UTF16StringView32_t = const CStringView< uint32_t, const char16_t >;
using UTF16StringView64_t = const CStringView< uint64_t, const char16_t >;

using UTF32StringView_t =   const CStringView< size_t, const char32_t >;
using UTF32StringView8_t =  const CStringView< uint8_t, const char32_t >;
using UTF32StringView16_t = const CStringView< uint16_t, const char32_t >;
using UTF32StringView32_t = const CStringView< uint32_t, const char32_t >;
using UTF32StringView64_t = const CStringView< uint64_t, const char32_t >;

//------------------------------------------------------------------------------
// StringView literals
//  - "_sv"   -> default index type (size_t) for all char kinds
//  - "_sv8"  -> uint8_t  index
//  - "_sv16" -> uint16_t index
//  - "_sv32" -> uint32_t index
//  - "_sv64" -> uint64_t index
// Overloads are selected by the string literal prefix: "", L"", u8"", u"", U""
//------------------------------------------------------------------------------
inline StringView_t operator""_sv( const char *pszString, size_t nLength ) { return StringView_t( static_cast< size_t >( nLength - 1 ), pszString ); }
inline WStringView_t operator""_sv( const wchar_t *pszString, size_t nLength ) { return WStringView_t( static_cast< size_t >( nLength - 1 ), pszString ); }
inline UTF8StringView_t operator""_sv( const char8_t *pszString, size_t nLength ) { return UTF8StringView_t( static_cast< size_t >( nLength - 1 ), pszString ); }
inline UTF16StringView_t operator""_sv( const char16_t *pszString, size_t nLength ) { return UTF16StringView_t( static_cast< size_t >( nLength - 1 ), pszString ); }
inline UTF32StringView_t operator""_sv( const char32_t *pszString, size_t nLength ) { return UTF32StringView_t( static_cast< size_t >( nLength - 1 ), pszString ); }

inline StringView8_t operator""_sv8( const char *pszString, size_t nLength ) { return StringView8_t( static_cast< uint8_t >( nLength - 1 ), pszString ); }
inline WStringView8_t operator""_sv8( const wchar_t *pszString, size_t nLength ) { return WStringView8_t( static_cast< uint8_t >( nLength - 1 ), pszString ); }
inline UTF8StringView8_t operator""_sv8( const char8_t *pszString, size_t nLength ) { return UTF8StringView8_t( static_cast< uint8_t >( nLength - 1 ), pszString ); }
inline UTF16StringView8_t operator""_sv8( const char16_t *pszString, size_t nLength ) { return UTF16StringView8_t( static_cast< uint8_t >( nLength - 1 ), pszString ); }
inline UTF32StringView8_t operator""_sv8( const char32_t *pszString, size_t nLength ) { return UTF32StringView8_t( static_cast< uint8_t >( nLength - 1 ), pszString ); }

inline StringView16_t operator""_sv16( const char *pszString, size_t nLength ) { return StringView16_t( static_cast< uint16_t >( nLength - 1 ), pszString ); }
inline WStringView16_t operator""_sv16( const wchar_t *pszString, size_t nLength ) { return WStringView16_t( static_cast< uint16_t >( nLength - 1 ), pszString ); }
inline UTF8StringView16_t operator""_sv16( const char8_t *pszString, size_t nLength ) { return UTF8StringView16_t( static_cast< uint16_t >( nLength - 1 ), pszString ); }
inline UTF16StringView16_t operator""_sv16( const char16_t *pszString, size_t nLength ) { return UTF16StringView16_t( static_cast< uint16_t >( nLength - 1 ), pszString ); }
inline UTF32StringView16_t operator""_sv16( const char32_t *pszString, size_t nLength ) { return UTF32StringView16_t( static_cast< uint16_t >( nLength - 1 ), pszString ); }

inline StringView32_t operator""_sv32( const char *pszString, size_t nLength ) { return StringView32_t( static_cast< uint32_t >( nLength - 1 ), pszString ); }
inline WStringView32_t operator""_sv32( const wchar_t *pszString, size_t nLength ) { return WStringView32_t( static_cast< uint32_t >( nLength - 1 ), pszString ); }
inline UTF8StringView32_t operator""_sv32( const char8_t *pszString, size_t nLength ) { return UTF8StringView32_t( static_cast< uint32_t >( nLength - 1 ), pszString ); }
inline UTF16StringView32_t operator""_sv32( const char16_t *pszString, size_t nLength ) { return UTF16StringView32_t( static_cast< uint32_t >( nLength - 1 ), pszString ); }
inline UTF32StringView32_t operator""_sv32( const char32_t *pszString, size_t nLength ) { return UTF32StringView32_t( static_cast< uint32_t >( nLength - 1 ), pszString ); }

inline StringView64_t operator""_sv64( const char *pszString, size_t nLength ) { return StringView64_t( static_cast< uint64_t >( nLength - 1 ), pszString ); }
inline WStringView64_t operator""_sv64( const wchar_t *pszString, size_t nLength ) { return WStringView64_t( static_cast< uint64_t >( nLength - 1 ), pszString ); }
inline UTF8StringView64_t operator""_sv64( const char8_t *pszString, size_t nLength ) { return UTF8StringView64_t( static_cast< uint64_t >( nLength - 1 ), pszString ); }
inline UTF16StringView64_t operator""_sv64( const char16_t *pszString, size_t nLength ) { return UTF16StringView64_t( static_cast< uint64_t >( nLength - 1 ), pszString ); }
inline UTF32StringView64_t operator""_sv64( const char32_t *pszString, size_t nLength ) { return UTF32StringView64_t( static_cast< uint64_t >( nLength - 1 ), pszString ); }

#endif // !defined( _INCLUDE_BALL_TYPES_STRINGVIEW_HPP_ )
