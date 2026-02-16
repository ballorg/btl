#ifndef _INCLUDE_BALL_TYPES_FIXED_HPP_
#	define _INCLUDE_BALL_TYPES_FIXED_HPP_

#	pragma once

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "meta/issame.hpp"
#	include "meta/fixed.hpp"
#	include "meta/removecv.hpp"
#	include "meta/select.hpp"

#	ifndef BALL_FIXED_FOR_EACH_BITS
#		define BALL_FIXED_FOR_EACH_BITS( M ) \
			M( 1 )  M( 2 )  M( 3 )  M( 4 )  M( 5 )  M( 6 )  M( 7 )  M( 8 )  \
			M( 9 )  M( 10 ) M( 11 ) M( 12 ) M( 13 ) M( 14 ) M( 15 ) M( 16 ) \
			M( 17 ) M( 18 ) M( 19 ) M( 20 ) M( 21 ) M( 22 ) M( 23 ) M( 24 ) \
			M( 25 ) M( 26 ) M( 27 ) M( 28 ) M( 29 ) M( 30 ) M( 31 ) M( 32 ) \
			M( 33 ) M( 34 ) M( 35 ) M( 36 ) M( 37 ) M( 38 ) M( 39 ) M( 40 ) \
			M( 41 ) M( 42 ) M( 43 ) M( 44 ) M( 45 ) M( 46 ) M( 47 ) M( 48 ) \
			M( 49 ) M( 50 ) M( 51 ) M( 52 ) M( 53 ) M( 54 ) M( 55 ) M( 56 ) \
			M( 57 ) M( 58 ) M( 59 ) M( 60 ) M( 61 ) M( 62 ) M( 63 ) M( 64 )
#	endif

#	ifndef BALL_FIXED_DECLARE_SIGNED_BY_BITS
#		define BALL_FIXED_DECLARE_SIGNED_BY_BITS( bits ) \
			template <> struct MFixedSignedByBits< bits > : public MFixedSignedImpl< sint##bits##_t, Unsigned_t< sint##bits##_t >, bits > \
			{ \
				using Type = sint##bits##_t; \
				using U = Unsigned_t< Type >; \
				static constexpr bits_t ALIAS_BITS = bits; \
			};
#	endif

#	ifndef BALL_FIXED_DECLARE_UNCERTAIN_BY_BITS
#		define BALL_FIXED_DECLARE_UNCERTAIN_BY_BITS( bits ) \
			template <> struct MFixedUncertainByBits< bits > : public MFixedUncertainImpl< int##bits##_t, Unsigned_t< int##bits##_t >, bits > \
			{ \
				using Type = int##bits##_t; \
				using U = Unsigned_t< Type >; \
				static constexpr bits_t ALIAS_BITS = bits; \
			};
#	endif

#	ifndef BALL_FIXED_DECLARE_UNSIGNED_BY_BITS
#		define BALL_FIXED_DECLARE_UNSIGNED_BY_BITS( bits ) \
			template <> struct MFixedUnsignedByBits< bits > : public MFixedUnsignedImpl< uint##bits##_t, Unsigned_t< uint##bits##_t >, bits > \
			{ \
				using Type = uint##bits##_t; \
				using U = Unsigned_t< Type >; \
				static constexpr bits_t ALIAS_BITS = bits; \
			};
#	endif

template < bits_t > struct MFixedSignedByBits;
template < bits_t > struct MFixedUncertainByBits;
template < bits_t > struct MFixedUnsignedByBits;

template < typename T >
struct MFixedDataBy
{
	using Type = T;
	using Signed_t = Signed_t< Type >;
	using Unsigned_t = Unsigned_t< Type >;
	static constexpr bool IS_BOOL = IS_SAME< Type, bool >;
	static constexpr bits_t BITS = IS_BOOL ? 1ull : static_cast< bits_t >( sizeof( Type ) * 8ull );
	static constexpr bits_t BYTES = ( BITS + bits_t( 7ull ) ) / bits_t( 8ull );
	static constexpr bool HAS_BITS_SPECIALIZATION = ( BITS <= bits_t( 64ull ) );

	static constexpr bool IS_SIGNED = IS_SAME< Type, Signed_t >;
	static constexpr bool IS_UNSIGNED = IS_SAME< Type, Unsigned_t >;

	using SignedByBits_t = typename MSelect< HAS_BITS_SPECIALIZATION >::template Apply_t< MFixedSignedByBits< BITS >, MFixedSignedImpl< Type, Unsigned_t, BITS > >;
	using UncertainByBits_t = typename MSelect< HAS_BITS_SPECIALIZATION >::template Apply_t< MFixedUncertainByBits< BITS >, MFixedUncertainImpl< Type, Unsigned_t, BITS > >;
	using UnsignedByBits_t = typename MSelect< HAS_BITS_SPECIALIZATION >::template Apply_t< MFixedUnsignedByBits< BITS >, MFixedUnsignedImpl< Type, Unsigned_t, BITS > >;

	using SignedImpl_t = MFixedSignedImpl< Type, Unsigned_t, BITS >;
	using UnsignedImpl_t = MFixedUnsignedImpl< Type, Unsigned_t, BITS >;
	using UncertainImpl_t = MFixedUncertainImpl< Type, Unsigned_t, BITS >;

	using Resolved_t = typename MSelect< IS_BOOL >::template Apply_t<
		UnsignedImpl_t,
		typename MSelect< IS_SIGNED >::template Apply_t<
			SignedByBits_t,
			typename MSelect< IS_UNSIGNED >::template Apply_t< 
				UncertainByBits_t,
				UnsignedImpl_t
			>
		>
	>;
};

BALL_FIXED_FOR_EACH_BITS( BALL_FIXED_DECLARE_SIGNED_BY_BITS )
BALL_FIXED_FOR_EACH_BITS( BALL_FIXED_DECLARE_UNCERTAIN_BY_BITS )
BALL_FIXED_FOR_EACH_BITS( BALL_FIXED_DECLARE_UNSIGNED_BY_BITS )

template < bits_t BITS >
using NumberSignedByBits_t = typename MFixedSignedByBits< BITS >::Type;

template < bits_t BITS >
using NumberUncertainByBits_t = typename MFixedUncertainByBits< BITS >::Type;

template < bits_t BITS >
using NumberUnsignedByBits_t = typename MFixedUnsignedByBits< BITS >::Type;

template < typename T >
struct MFixedTraits
{
	using Type = RemoveCV_t< T >;
	using ByType_t = MFixedDataBy< Type >;
	using Unsigned_t = typename ByType_t::Unsigned_t;
	using Resolved_t = typename ByType_t::Resolved_t;
};

// Any fixed integer.
template < typename T >
struct MFixedData : public MFixedTraits< T >::Resolved_t
{
	using Base_t = MFixedTraits< T >;
	using Unsigned_t = Base_t::Unsigned_t;
};

#	undef BALL_FIXED_DECLARE_UNSIGNED_BY_BITS
#	undef BALL_FIXED_DECLARE_UNCERTAIN_BY_BITS
#	undef BALL_FIXED_DECLARE_SIGNED_BY_BITS
#	undef BALL_FIXED_FOR_EACH_BITS

#endif // !defined( _INCLUDE_BALL_TYPES_FIXED_HPP_ )
