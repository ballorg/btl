#ifndef _INCLUDE_BALL_TYPES_FIXED_HPP_
#	define _INCLUDE_BALL_TYPES_FIXED_HPP_

#	pragma once

#	include "base/arch.h"
#	include "meta/fixed/bits.h"
#	include "base/fixed.h"
#	include "meta/isintegral.hpp"
#	include "meta/issame.hpp"
#	include "meta/removecv.hpp"
#	include "meta/select.hpp"
#	include "meta/signed.hpp"
#	include "meta/unsigned.hpp"

#	define BALL_FIXED_FOR_EACH_BITS( M ) \
		M( 1 )  M( 2 )  M( 3 )  M( 4 )  M( 5 )  M( 6 )  M( 7 )  M( 8 )  \
		M( 9 )  M( 10 ) M( 11 ) M( 12 ) M( 13 ) M( 14 ) M( 15 ) M( 16 ) \
		M( 17 ) M( 18 ) M( 19 ) M( 20 ) M( 21 ) M( 22 ) M( 23 ) M( 24 ) \
		M( 25 ) M( 26 ) M( 27 ) M( 28 ) M( 29 ) M( 30 ) M( 31 ) M( 32 ) \
		M( 33 ) M( 34 ) M( 35 ) M( 36 ) M( 37 ) M( 38 ) M( 39 ) M( 40 ) \
		M( 41 ) M( 42 ) M( 43 ) M( 44 ) M( 45 ) M( 46 ) M( 47 ) M( 48 ) \
		M( 49 ) M( 50 ) M( 51 ) M( 52 ) M( 53 ) M( 54 ) M( 55 ) M( 56 ) \
		M( 57 ) M( 58 ) M( 59 ) M( 60 ) M( 61 ) M( 62 ) M( 63 ) M( 64 )

///-----------------------------------------------------------------------------
/// @brief Semantic category of a fixed-width wrapper.
///
/// The tag controls how values are normalized when the logical bit width
/// (`BITS`) is smaller than the storage width (`STORAGE_BITS`):
/// - `FIXED_UNSIGNED`  : value is truncated to `BITS` and interpreted as unsigned.
/// - `FIXED_SIGNED`    : value is truncated and then sign-extended from bit `BITS-1`.
/// - `FIXED_UNCERTAIN` : treated as signed for normalization/sign extension.
///
/// Notes:
/// - This is a compile-time policy only; no runtime branching by tag is required.
/// - `FIXED_UNCERTAIN` exists to preserve API intent for platform-dependent
///   integral aliases while keeping deterministic normalization behavior.
///-----------------------------------------------------------------------------
enum FixedTag_t : uint2_t
{
	FIXED_SIGNED = 0,
	FIXED_UNCERTAIN = 1,
	FIXED_UNSIGNED = 2,
	FIXED_RESERVED = 3
};

template < bits_t NBIT >
struct MFixedMask
{
	static constexpr ullong_t Value()
	{
		if constexpr ( NBIT >= 64 )
			return ~0ull;
		else
			return ( 1ull << NBIT ) - 1ull;
	}

	static constexpr ullong_t SignValue()
	{
		if constexpr ( NBIT <= 1 )
			return 1ull;
		else if constexpr ( NBIT >= 64 )
			return 1ull << 63;
		else
			return 1ull << ( NBIT - 1 );
	}

	static constexpr ullong_t VALUE = Value();
	static constexpr ullong_t SIGN_VALUE = SignValue();
};

///-----------------------------------------------------------------------------
/// @brief Compile-time traits + normalization policy for fixed-width integral types.
///
/// @tparam T      Backing storage type (for example `sint33_t`, `uint40_t`, etc.).
/// @tparam TBITS  Logical payload width in bits.
/// @tparam TTAG   Signedness policy (`FixedTag_t`).
///
/// Responsibilities:
/// - Defines canonical aliases (`Type`, `Signed_t`, `Unsigned_t`).
/// - Exposes compile-time metadata (`BITS`, `STORAGE_BITS`, `IS_SIGNED`).
/// - Provides `Normalize()` which constrains any input to the representable
///   logical range encoded by (`TBITS`, `TTAG`).
///
/// Normalization algorithm:
/// 1. Cast input to `Unsigned_t` to avoid implementation-defined arithmetic shifts.
/// 2. If `BITS < STORAGE_BITS`, apply a low-bit mask (`(1 << BITS) - 1`).
/// 3. For signed policies, sign-extend from bit `BITS-1`.
/// 4. Cast back to `Type`.
///
/// Invariants:
/// - Stored values are always canonical for the declared logical width.
/// - Re-normalizing an already normalized value is idempotent.
///-----------------------------------------------------------------------------
template < typename T, bits_t TBITS, FixedTag_t TTAG >
struct MFixedBase
{
	using Type = T;
	using Signed_t = typename MSigned< Type >::Type;
	using Unsigned_t = typename MUnsigned< Type >::Type;
	using Mask_t = MFixedMask< TBITS >;

	static constexpr FixedTag_t TAG = TTAG; /// Compile-time signedness policy.
	static constexpr bits_t BITS = TBITS; /// Logical number of meaningful bits.
	static constexpr bits_t STORAGE_BITS = bits_t( sizeof( Type ) * 8u ); /// Physical storage width of `Type` in bits.
	static constexpr bool IS_SIGNED = IS_SAME< Type, Signed_t >; /// Effective signed behavior used by normalization/sign-extension.

	static constexpr Type IsSigned( Type value ) noexcept
	{
		return ( value & Mask_t::SIGN_VALUE ) != Unsigned_t( 0 );
	}

	///-----------------------------------------------------------------------------
	/// @brief Canonicalize an arbitrary input value to (`BITS`, `TAG`) domain.
	///
	/// For narrower logical widths (`BITS < STORAGE_BITS`), high bits are removed.
	/// For signed policies, the logical sign bit is propagated to the storage width.
	///-----------------------------------------------------------------------------
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

///-----------------------------------------------------------------------------
/// @brief Trivial value type that stores an always-normalized fixed-width integer.
///
/// `CFixedBase` is a thin wrapper over `MFixedBase`:
/// - construction/assignment normalize incoming values;
/// - conversion back to `Type` is explicit-by-context via `operator Type()`;
/// - comparison operators compare normalized payloads directly.
///
/// Design intent:
/// - deterministic truncation/sign-extension semantics for non-standard bit widths;
/// - zero-overhead metadata access through inherited compile-time constants.
///-----------------------------------------------------------------------------
template < typename T, bits_t TBITS, FixedTag_t TTAG >
struct CFixedBase : public MFixedBase< T, TBITS, TTAG >
{
	using Base_t = MFixedBase< T, TBITS, TTAG >;
	using typename Base_t::Type;
	using typename Base_t::Unsigned_t;
	using Base_t::TAG;
	using Base_t::BITS;
	using Base_t::STORAGE_BITS;
	using Base_t::IS_SIGNED;
	using Base_t::Normalize;

	Type m_Value;

	constexpr CFixedBase() noexcept : m_Value( Type( 0 ) ) {}
	constexpr CFixedBase( Type value ) noexcept : m_Value( Normalize( value ) ) {}
	constexpr CFixedBase &operator=( Type value ) noexcept { m_Value = Normalize( value ); return *this; }
	constexpr operator Type() const noexcept { return m_Value; }

	constexpr bool operator==( const CFixedBase &rhs ) const noexcept { return m_Value == rhs.m_Value; }
	constexpr bool operator!=( const CFixedBase &rhs ) const noexcept { return m_Value != rhs.m_Value; }
	constexpr bool operator< ( const CFixedBase &rhs ) const noexcept { return m_Value < rhs.m_Value; }
	constexpr bool operator> ( const CFixedBase &rhs ) const noexcept { return m_Value > rhs.m_Value; }
	constexpr bool operator<=( const CFixedBase &rhs ) const noexcept { return m_Value <= rhs.m_Value; }
	constexpr bool operator>=( const CFixedBase &rhs ) const noexcept { return m_Value >= rhs.m_Value; }
};

#	define BALL_FIXED_DECLARE_SIGNED( bits ) \
		using FixedSigned##bits##_t = CFixedBase< sint##bits##_t, bits_t( bits ), FIXED_SIGNED >;

#	define BALL_FIXED_DECLARE_UNCERTAIN( bits ) \
		using FixedUncertain##bits##_t = CFixedBase< int##bits##_t, bits_t( bits ), FIXED_UNCERTAIN >;

#	define BALL_FIXED_DECLARE_UNSIGNED( bits ) \
		using FixedUnsiged##bits##_t = CFixedBase< uint##bits##_t, bits_t( bits ), FIXED_UNSIGNED >;

BALL_FIXED_FOR_EACH_BITS( BALL_FIXED_DECLARE_SIGNED )
BALL_FIXED_FOR_EACH_BITS( BALL_FIXED_DECLARE_UNCERTAIN )
BALL_FIXED_FOR_EACH_BITS( BALL_FIXED_DECLARE_UNSIGNED )

template < typename T > struct MFixedPackedBase;

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

	static constexpr bits_t ComputeBits( const size_t nBytes ) noexcept
	{
		const size_t nBits = nBytes * 8;
		constexpr size_t nMaxBits = static_cast< size_t >( ~bits_t( 0 ) );

		return static_cast< bits_t >( nBits < nMaxBits ? nBits : nMaxBits );
	}

	static constexpr bits_t BITS = ComputeBits( sizeof( Raw_t ) );
};

template <> struct MFixedPackedBase< bool >
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

template <> struct MFixedPackedBase< FixedTag_t >
{
	using Type = FixedTag_t;
	using Raw_t = FixedTag_t;
	using Signed_t = sint2_t;
	using Unsigned_t = uint2_t;
	static constexpr bool IS_INT = false;
	static constexpr bool IS_SIGNED = false;
	static constexpr bool IS_UNSIGNED = true;
	static constexpr bits_t BITS = 2;
};

///-----------------------------------------------------------------------------
/// @brief Fixed-width metadata adapter for enum-backed packed storage.
///
/// `Type` remains the user-facing enum, while `Raw_t` points at the declared
/// underlying integral storage. This keeps `MultiVector` column access strongly
/// typed and still allows packed bit operations to work on a deterministic
/// unsigned/signed raw domain.
///-----------------------------------------------------------------------------
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

	static constexpr bits_t ComputeBits( const size_t nBytes ) noexcept
	{
		const size_t nBits = nBytes * 8;
		constexpr size_t nMaxBits = static_cast< size_t >( ~bits_t( 0 ) );

		return static_cast< bits_t >( nBits < nMaxBits ? nBits : nMaxBits );
	}

	static constexpr bits_t BITS = TBITS;
};

#	define BALL_FIXED_DECLARE_SIGNED_BASE_TRAIT( typeDef, bits, typeName ) \
	template <> struct MFixedPackedBase< typeName > : public MFixedPackedBase< typeDef > \
	{ \
		static constexpr bool IS_SIGNED = true; \
		static constexpr bool IS_UNSIGNED = false; \
		static constexpr bits_t BITS = bits_t( bits ); \
	};

#	define BALL_FIXED_DECLARE_UNCERTAIN_BASE_TRAIT( typeDef, bits, typeName ) \
	template <> struct MFixedPackedBase< typeName > : public MFixedPackedBase< typeDef > \
	{ \
		static constexpr bool IS_SIGNED = true; \
		static constexpr bool IS_UNSIGNED = false; \
		static constexpr bits_t BITS = bits_t( bits ); \
	};

#	define BALL_FIXED_DECLARE_UNSIGNED_BASE_TRAIT( typeDef, bits, typeName ) \
	template <> struct MFixedPackedBase< typeName > : public MFixedPackedBase< typeDef > \
	{ \
		static constexpr bool IS_SIGNED = false; \
		static constexpr bool IS_UNSIGNED = true; \
		static constexpr bits_t BITS = bits_t( bits ); \
	};

#	define BALL_FIXED_DECLARE_SIGNED_TRAIT( bits ) BALL_FIXED_DECLARE_SIGNED_BASE_TRAIT( sint##bits##_t, bits, FixedSigned##bits##_t )
#	define BALL_FIXED_DECLARE_UNCERTAIN_TRAIT( bits ) BALL_FIXED_DECLARE_UNCERTAIN_BASE_TRAIT( int##bits##_t, bits, FixedUncertain##bits##_t )
#	define BALL_FIXED_DECLARE_UNSIGNED_TRAIT( bits ) BALL_FIXED_DECLARE_UNSIGNED_BASE_TRAIT( uint##bits##_t, bits, FixedUnsiged##bits##_t )

#	define BALL_FIXED_DECLARE_SIGNED_ENUM_TRAIT( enumName, bits ) \
	template <> struct MFixedPackedBase< enumName > : public MFixedPackedEnumBase< enumName, sint##bits##_t, bits_t( bits ), true, false > {};

#	define BALL_FIXED_DECLARE_UNCERTAIN_ENUM_TRAIT( enumName, bits ) \
	template <> struct MFixedPackedBase< enumName > : public MFixedPackedEnumBase< enumName, int##bits##_t, bits_t( bits ), true, false > {};

#	define BALL_FIXED_DECLARE_UNSIGNED_ENUM_TRAIT( enumName, bits ) \
	template <> struct MFixedPackedBase< enumName > : public MFixedPackedEnumBase< enumName, uint##bits##_t, bits_t( bits ), false, true > {};

#	define BALL_FIXED_DECLARE_ENUM_BASE( M, enumName, typeDef, bits ) enum enumName : typeDef; M( enumName, bits ) enum enumName : typeDef
#	define BALL_FIXED_DECLARE_ENUM_CLASS_BASE( M, enumName, typeDef, bits ) enum class enumName : typeDef; M( enumName, bits ) enum class enumName : typeDef
#	define BALL_FIXED_SIGNED_ENUM( enumName, bits ) BALL_FIXED_DECLARE_ENUM_BASE( BALL_FIXED_DECLARE_SIGNED_ENUM_TRAIT, enumName, sint##bits##_t, bits )
#	define BALL_FIXED_UNCERTAIN_ENUM( enumName, bits ) BALL_FIXED_DECLARE_ENUM_BASE( BALL_FIXED_DECLARE_UNCERTAIN_ENUM_TRAIT, enumName, int##bits##_t, bits )
#	define BALL_FIXED_UNSIGNED_ENUM( enumName, bits ) BALL_FIXED_DECLARE_ENUM_BASE( BALL_FIXED_DECLARE_UNSIGNED_ENUM_TRAIT, enumName, uint##bits##_t, bits )
#	define BALL_FIXED_SIGNED_ENUM_CLASS( enumName, bits ) BALL_FIXED_DECLARE_ENUM_CLASS_BASE( BALL_FIXED_DECLARE_SIGNED_ENUM_TRAIT, enumName, sint##bits##_t, bits )
#	define BALL_FIXED_UNCERTAIN_ENUM_CLASS( enumName, bits ) BALL_FIXED_DECLARE_ENUM_CLASS_BASE( BALL_FIXED_DECLARE_UNCERTAIN_ENUM_TRAIT, enumName, int##bits##_t, bits )
#	define BALL_FIXED_UNSIGNED_ENUM_CLASS( enumName, bits ) BALL_FIXED_DECLARE_ENUM_CLASS_BASE( BALL_FIXED_DECLARE_UNSIGNED_ENUM_TRAIT, enumName, uint##bits##_t, bits )
#	define BALL_FIXED_SIGNED_ENUM_TRAIT( enumName, bits ) \
	template <> struct MFixedPackedBase< enumName > : public MFixedPackedEnumBase< enumName, sint##bits##_t, bits_t( bits ), true, false > {};

#	define BALL_FIXED_UNCERTAIN_ENUM_TRAIT( enumName, bits ) \
	template <> struct MFixedPackedBase< enumName > : public MFixedPackedEnumBase< enumName, int##bits##_t, bits_t( bits ), true, false > {};

#	define BALL_FIXED_UNSIGNED_ENUM_TRAIT( enumName, bits ) \
	template <> struct MFixedPackedBase< enumName > : public MFixedPackedEnumBase< enumName, uint##bits##_t, bits_t( bits ), false, true > {};

BALL_FIXED_FOR_EACH_BITS( BALL_FIXED_DECLARE_SIGNED_TRAIT )
BALL_FIXED_FOR_EACH_BITS( BALL_FIXED_DECLARE_UNCERTAIN_TRAIT )
BALL_FIXED_FOR_EACH_BITS( BALL_FIXED_DECLARE_UNSIGNED_TRAIT )

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

	static constexpr bool ComputeSigned() noexcept
	{
		if constexpr ( IS_SIGNED )
			return true;
		else if constexpr ( IS_UNSIGNED )
			return false;
		else
			return static_cast< Raw_t >( -1 ) < static_cast< Raw_t >( 0 );
	}

	static constexpr bits_t ComputeBits( const size_t nBytes ) noexcept
	{
		const size_t nBits = nBytes * 8;
		constexpr size_t nMaxBits = static_cast< size_t >( ~bits_t( 0 ) );

		return static_cast< bits_t >( nBits < nMaxBits ? nBits : nMaxBits );
	}

	static constexpr bits_t STORAGE_BITS = ComputeBits( sizeof( Raw_t ) );
	static constexpr bool IS_PACKED = BITS < STORAGE_BITS;
	static constexpr bits_t UNSIGNED_BITS = ComputeBits( sizeof( Unsigned_t ) );
	static constexpr size_t BYTES = IS_PACKED ? ( static_cast< size_t >( BITS + bits_t( 7ull ) ) / bits_t( 8ull ) ) : sizeof( Raw_t );

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

#	undef BALL_FIXED_DECLARE_UNSIGNED_TRAIT
#	undef BALL_FIXED_DECLARE_UNCERTAIN_TRAIT
#	undef BALL_FIXED_DECLARE_SIGNED_TRAIT
#	undef BALL_FIXED_DECLARE_UNSIGNED_ENUM_TRAIT
#	undef BALL_FIXED_DECLARE_UNCERTAIN_ENUM_TRAIT
#	undef BALL_FIXED_DECLARE_SIGNED_ENUM_TRAIT
#	undef BALL_FIXED_DECLARE_UNSIGNED_BASE_TRAIT
#	undef BALL_FIXED_DECLARE_UNCERTAIN_BASE_TRAIT
#	undef BALL_FIXED_DECLARE_SIGNED_BASE_TRAIT
#	undef BALL_FIXED_DECLARE_UNSIGNED
#	undef BALL_FIXED_DECLARE_UNCERTAIN
#	undef BALL_FIXED_DECLARE_SIGNED
#	undef BALL_FIXED_DECLARE_UNSIGNED_BY_BITS
#	undef BALL_FIXED_DECLARE_UNCERTAIN_BY_BITS
#	undef BALL_FIXED_DECLARE_SIGNED_BY_BITS

#endif // !defined( _INCLUDE_BALL_TYPES_FIXED_HPP_ )
