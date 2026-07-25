#ifndef _INCLUDE_BALL_TYPES_META_FIXED_HPP_
#	define _INCLUDE_BALL_TYPES_META_FIXED_HPP_

#	pragma once

#	include "fixed/bits.h"
#	include "fixed/signed.hpp"
#	include "fixed/uncertain.hpp"
#	include "fixed/unsigned.hpp"
#	include "isintegral.hpp"
#	include "issame.hpp"
#	include "removecv.hpp"
#	include "select.hpp"
#	include "signed.hpp"
#	include "unsigned.hpp"

enum FixedTag_t : unsigned char
{
	FIXED_SIGNED = 0,
	FIXED_UNCERTAIN = 1,
	FIXED_UNSIGNED = 2,
	FIXED_RESERVED = 3
};

template < bits_t NBIT >
struct MFixedMask
{
	static constexpr unsigned long long Value() noexcept
	{
		if constexpr ( NBIT >= 64 )
			return ~0ull;
		else
			return ( 1ull << NBIT ) - 1ull;
	}

	static constexpr unsigned long long SignValue() noexcept
	{
		if constexpr ( NBIT <= 1 )
			return 1ull;
		else if constexpr ( NBIT >= 64 )
			return 1ull << 63;
		else
			return 1ull << ( NBIT - 1 );
	}

	static constexpr unsigned long long VALUE = Value();
	static constexpr unsigned long long SIGN_VALUE = SignValue();
};

template < typename T, bits_t TBITS, FixedTag_t TTAG >
struct MFixedBase
{
	using Type = T;
	using Signed_t = typename MSigned< Type >::Type;
	using Unsigned_t = typename MUnsigned< Type >::Type;
	using Mask_t = MFixedMask< TBITS >;

	static constexpr FixedTag_t TAG = TTAG;
	static constexpr bits_t BITS = TBITS;
	static constexpr bits_t STORAGE_BITS = bits_t( sizeof( Type ) * 8u );
	static constexpr bool IS_SIGNED = IS_SAME< Type, Signed_t >;

	static constexpr Type IsSigned( Type value ) noexcept
	{
		return ( value & Mask_t::SIGN_VALUE ) != Unsigned_t( 0 );
	}

	static constexpr Type Normalize( Type value ) noexcept
	{
		Unsigned_t nRaw = static_cast< Unsigned_t >( value );

		if constexpr ( BITS < STORAGE_BITS )
		{
			constexpr Unsigned_t nMask = Mask_t::VALUE;

			nRaw = nRaw & nMask;

			if constexpr ( IS_SIGNED )
			{
				if ( IsSigned( nRaw ) )
					nRaw |= ~nMask;
			}
		}

		return static_cast< Type >( nRaw );
	}
};

template < typename T >
struct MFixedPackedBase
{
	using Type = T;
	using Raw_t = RemoveCV_t< Type >;
	static constexpr bool IS_INT = IS_INTEGRAL< Raw_t >;

	using Signed_t = typename MSelect< IS_INT >::template Apply_t< typename MSigned< Raw_t >::Type, bits_t >;
	using Unsigned_t = typename MSelect< IS_INT >::template Apply_t< typename MUnsigned< Raw_t >::Type, bits_t >;

	static constexpr bool IS_SIGNED = IS_INT && IS_SAME< Raw_t, Signed_t >;
	static constexpr bool IS_UNSIGNED = IS_INT && IS_SAME< Raw_t, Unsigned_t >;
	static constexpr bits_t BITS = static_cast< bits_t >( sizeof( Raw_t ) * 8u );
};

template <>
struct MFixedPackedBase< bool >
{
	using Type = bool;
	using Raw_t = bool;
	using Signed_t = signed char;
	using Unsigned_t = bool;
	static constexpr bool IS_INT = true;
	static constexpr bool IS_SIGNED = false;
	static constexpr bool IS_UNSIGNED = true;
	static constexpr bits_t BITS = 1;
};

template <>
struct MFixedPackedBase< FixedTag_t >
{
	using Type = FixedTag_t;
	using Raw_t = FixedTag_t;
	using Signed_t = signed char;
	using Unsigned_t = unsigned char;
	static constexpr bool IS_INT = false;
	static constexpr bool IS_SIGNED = false;
	static constexpr bool IS_UNSIGNED = true;
	static constexpr bits_t BITS = 2;
};

template < typename T, typename TRaw, bits_t TBITS, bool TIS_SIGNED, bool TIS_UNSIGNED >
struct MFixedPackedEnumBase
{
	using Type = T;
	using Raw_t = TRaw;
	using Signed_t = typename MSigned< Raw_t >::Type;
	using Unsigned_t = typename MUnsigned< Raw_t >::Type;

	static constexpr bool IS_INT = false;
	static constexpr bool IS_SIGNED = TIS_SIGNED;
	static constexpr bool IS_UNSIGNED = TIS_UNSIGNED;
	static constexpr bits_t BITS = TBITS;
};

template < bits_t BITS >
struct MFixedSignedStorage
{
	static_assert( BITS >= 1 && BITS <= 64, "fixed storage supports 1..64 bits" );

	using Type = typename MSelect< BITS <= 8 >::template Apply_t<
		signed char,
		typename MSelect< BITS <= 16 >::template Apply_t<
			short,
			typename MSelect< BITS <= 32 >::template Apply_t< int, long long >
		>
	>;
};

template < bits_t BITS >
using FixedSignedStorage_t = typename MFixedSignedStorage< BITS >::Type;

template < bits_t BITS >
using FixedUnsignedStorage_t = typename MUnsigned< FixedSignedStorage_t< BITS > >::Type;

#	define BALL_FIXED_DECLARE_SIGNED_ENUM_TRAIT( enumName, bits ) \
	template <> struct MFixedPackedBase< enumName > : public MFixedPackedEnumBase< enumName, FixedSignedStorage_t< bits >, bits_t( bits ), true, false > {};

#	define BALL_FIXED_DECLARE_UNCERTAIN_ENUM_TRAIT( enumName, bits ) \
	template <> struct MFixedPackedBase< enumName > : public MFixedPackedEnumBase< enumName, FixedSignedStorage_t< bits >, bits_t( bits ), true, false > {};

#	define BALL_FIXED_DECLARE_UNSIGNED_ENUM_TRAIT( enumName, bits ) \
	template <> struct MFixedPackedBase< enumName > : public MFixedPackedEnumBase< enumName, FixedUnsignedStorage_t< bits >, bits_t( bits ), false, true > {};

#	define BALL_FIXED_SIGNED_ENUM_TRAIT( enumName, bits ) BALL_FIXED_DECLARE_SIGNED_ENUM_TRAIT( enumName, bits )
#	define BALL_FIXED_UNCERTAIN_ENUM_TRAIT( enumName, bits ) BALL_FIXED_DECLARE_UNCERTAIN_ENUM_TRAIT( enumName, bits )
#	define BALL_FIXED_UNSIGNED_ENUM_TRAIT( enumName, bits ) BALL_FIXED_DECLARE_UNSIGNED_ENUM_TRAIT( enumName, bits )

#	define BALL_FIXED_DECLARE_ENUM_BASE( M, enumName, typeDef, bits ) enum enumName : typeDef; M( enumName, bits ) enum enumName : typeDef
#	define BALL_FIXED_DECLARE_ENUM_CLASS_BASE( M, enumName, typeDef, bits ) enum class enumName : typeDef; M( enumName, bits ) enum class enumName : typeDef
#	define BALL_FIXED_SIGNED_ENUM( enumName, bits ) BALL_FIXED_DECLARE_ENUM_BASE( BALL_FIXED_DECLARE_SIGNED_ENUM_TRAIT, enumName, FixedSignedStorage_t< bits >, bits )
#	define BALL_FIXED_UNCERTAIN_ENUM( enumName, bits ) BALL_FIXED_DECLARE_ENUM_BASE( BALL_FIXED_DECLARE_UNCERTAIN_ENUM_TRAIT, enumName, FixedSignedStorage_t< bits >, bits )
#	define BALL_FIXED_UNSIGNED_ENUM( enumName, bits ) BALL_FIXED_DECLARE_ENUM_BASE( BALL_FIXED_DECLARE_UNSIGNED_ENUM_TRAIT, enumName, FixedUnsignedStorage_t< bits >, bits )
#	define BALL_FIXED_SIGNED_ENUM_CLASS( enumName, bits ) BALL_FIXED_DECLARE_ENUM_CLASS_BASE( BALL_FIXED_DECLARE_SIGNED_ENUM_TRAIT, enumName, FixedSignedStorage_t< bits >, bits )
#	define BALL_FIXED_UNCERTAIN_ENUM_CLASS( enumName, bits ) BALL_FIXED_DECLARE_ENUM_CLASS_BASE( BALL_FIXED_DECLARE_UNCERTAIN_ENUM_TRAIT, enumName, FixedSignedStorage_t< bits >, bits )
#	define BALL_FIXED_UNSIGNED_ENUM_CLASS( enumName, bits ) BALL_FIXED_DECLARE_ENUM_CLASS_BASE( BALL_FIXED_DECLARE_UNSIGNED_ENUM_TRAIT, enumName, FixedUnsignedStorage_t< bits >, bits )

template < typename T >
struct MFixedMetadataBase : public MFixedPackedBase< T >
{
	using Type = T;
	using Base_t = MFixedPackedBase< T >;
	using typename Base_t::Raw_t;
	using typename Base_t::Signed_t;
	using typename Base_t::Unsigned_t;
	using Base_t::IS_INT;
	using Base_t::IS_SIGNED;
	using Base_t::IS_UNSIGNED;
	using Base_t::BITS;

	static constexpr bits_t STORAGE_BITS = static_cast< bits_t >( sizeof( Raw_t ) * 8u );
	static constexpr bool IS_PACKED = BITS < STORAGE_BITS;
	static constexpr bits_t UNSIGNED_BITS = static_cast< bits_t >( sizeof( Unsigned_t ) * 8u );
	static constexpr bytes_t BYTES = IS_PACKED ? ( BITS + bits_t( 7 ) ) / bits_t( 8 ) : sizeof( Raw_t );
	static constexpr Unsigned_t MASK = static_cast< Unsigned_t >( MFixedMask< BITS >::VALUE );
};

template < typename T >
struct MFixedMetadata : public MFixedMetadataBase< RemoveCV_t< T > >
{
	using Base_t = MFixedMetadataBase< RemoveCV_t< T > >;
	using Type = typename Base_t::Type;
	using Signed_t = typename Base_t::Signed_t;
	using Unsigned_t = typename Base_t::Unsigned_t;
};

// Any fixed integer.
template < typename T >
struct MFixed
{
	using Type = T;
	using Raw_t = RemoveCV_t< Type >;
	using Signed_t = typename MSigned< Raw_t >::Type;
	using Unsigned_t = typename MUnsigned< Raw_t >::Type;
	using Size_t = decltype( sizeof( Raw_t ) );

	static constexpr Size_t SIZE = sizeof( Raw_t );
	static constexpr bool IS_BOOL = IS_SAME< Raw_t, bool >;
	static constexpr Size_t BYTES = SIZE;
	static constexpr bits_t BITS = IS_BOOL ? 1ull : BYTES * 8ull;
	static constexpr bool IS_UNSIGNED = IS_SAME< Raw_t, Unsigned_t >;
	static constexpr bool IS_SIGNED = !IS_UNSIGNED;

	static constexpr Unsigned_t MIN_UNSIGNED = 0;
	static constexpr Unsigned_t MAX_UNSIGNED = static_cast< Unsigned_t >( ~0ull );
	static constexpr Type MIN_SIGNED = static_cast< Raw_t >( 1ull << ( BITS - 1 ) );
	static constexpr Type ALL_BITS = static_cast< Raw_t >( ~0ull );
	static constexpr Type INVALID = ALL_BITS;
	static constexpr Type MAX_SIGNED = static_cast< Raw_t >( static_cast< Unsigned_t >( MIN_SIGNED ) - Unsigned_t( 1 ) );
	static constexpr Unsigned_t MIN = IS_SIGNED ? static_cast< Unsigned_t >( MIN_SIGNED ) : MIN_UNSIGNED;
	static constexpr Unsigned_t MAX = IS_SIGNED ? static_cast< Unsigned_t >( MAX_SIGNED ) : MAX_UNSIGNED;
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_FIXED_HPP_ )
