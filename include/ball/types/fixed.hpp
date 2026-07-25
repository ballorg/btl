#ifndef _INCLUDE_BALL_TYPES_FIXED_HPP_
#	define _INCLUDE_BALL_TYPES_FIXED_HPP_

#	pragma once

#	include "meta/fixed.hpp"
#	include "base/arch.h"
#	include "base/fixed.h"

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
/// @brief Trivial value type that stores an always-normalized fixed-width integer.
///
/// `CFixedBase` is a thin wrapper over `MFixedBase`:
/// - construction/assignment normalize incoming values;
/// - conversion back to `Type` is explicit-by-context via `operator Type()`;
/// - comparison operators compare normalized payloads directly.
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

#	define BALL_FIXED_DECLARE_BASE_TRAIT( typeDef, bits, typeName, isSigned, isUnsigned ) \
	template <> struct MFixedPackedBase< typeName > : public MFixedPackedBase< typeDef > \
	{ \
		static constexpr bool IS_SIGNED = isSigned; \
		static constexpr bool IS_UNSIGNED = isUnsigned; \
		static constexpr bits_t BITS = bits_t( bits ); \
	};

#	define BALL_FIXED_DECLARE_SIGNED_TRAIT( bits ) BALL_FIXED_DECLARE_BASE_TRAIT( sint##bits##_t, bits, FixedSigned##bits##_t, true, false )
#	define BALL_FIXED_DECLARE_UNCERTAIN_TRAIT( bits ) BALL_FIXED_DECLARE_BASE_TRAIT( int##bits##_t, bits, FixedUncertain##bits##_t, true, false )
#	define BALL_FIXED_DECLARE_UNSIGNED_TRAIT( bits ) BALL_FIXED_DECLARE_BASE_TRAIT( uint##bits##_t, bits, FixedUnsiged##bits##_t, false, true )

BALL_FIXED_FOR_EACH_BITS( BALL_FIXED_DECLARE_SIGNED_TRAIT )
BALL_FIXED_FOR_EACH_BITS( BALL_FIXED_DECLARE_UNCERTAIN_TRAIT )
BALL_FIXED_FOR_EACH_BITS( BALL_FIXED_DECLARE_UNSIGNED_TRAIT )

#	undef BALL_FIXED_DECLARE_UNSIGNED_TRAIT
#	undef BALL_FIXED_DECLARE_UNCERTAIN_TRAIT
#	undef BALL_FIXED_DECLARE_SIGNED_TRAIT
#	undef BALL_FIXED_DECLARE_BASE_TRAIT
#	undef BALL_FIXED_DECLARE_UNSIGNED
#	undef BALL_FIXED_DECLARE_UNCERTAIN
#	undef BALL_FIXED_DECLARE_SIGNED

#endif // !defined( _INCLUDE_BALL_TYPES_FIXED_HPP_ )
