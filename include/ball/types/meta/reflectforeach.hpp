#ifndef _INCLUDE_BALL_TYPES_META_REFLECTFOREACH_HPP_
#	define _INCLUDE_BALL_TYPES_META_REFLECTFOREACH_HPP_

#	pragma once

#	include "reflecttraits.hpp"

namespace Detail
{

/// Iterates over reflected fields while exposing padding before and after each field.
///
/// The end-of-object bound @p nEnd and the previous field's end @p nPrevEnd are
/// threaded as ordinary arguments rather than template parameters: field offsets are
/// only constant expressions for standard-layout owners, so a value-based recursion
/// keeps this usable for inherited (non-standard-layout) owners too. For
/// standard-layout owners every term still folds to a compile-time constant.
template < typename Fs, typename I, typename C >
struct MForeach;

template < typename I, typename C >
struct MForeach< MTuple<>, I, C >
{
	static constexpr bool Apply( C &, I, I ) { return true; }
};

template < typename H, typename I, typename C >
struct MForeach< MTuple< H >, I, C >
{
	static constexpr bool Apply( C &callback, I nEnd, I nPrevEnd )
	{
		const I nCurrEnd = H::Offset() + H::FIELD_SIZE;
		const I nPadBefore = H::Offset() - nPrevEnd;
		const I nPadAfter = nEnd - nCurrEnd;

		return callback.template Apply< H >( nPadBefore, nPadAfter );
	}
};

template < typename H, typename N, typename... Ts, typename I, typename C >
struct MForeach< MTuple< H, N, Ts... >, I, C >
{
	static constexpr bool Apply( C &callback, I nEnd, I nPrevEnd )
	{
		const I nCurrEnd = H::Offset() + H::FIELD_SIZE;
		const I nPadBefore = H::Offset() - nPrevEnd;
		const I nPadAfter = N::Offset() - nCurrEnd;

		if ( !callback.template Apply< H >( nPadBefore, nPadAfter ) )
			return false;

		return MForeach< MTuple< N, Ts... >, I, C >::Apply( callback, nEnd, nCurrEnd );
	}
};

} // namespace Detail

/// Enumerates reflected fields in declaration order.
template < typename O, typename C >
constexpr bool Reflect_ForEach( C &&callback )
{
	using Desc_t = Reflect_t< O >;
	using Index_t = typename Desc_t::Index_t;

	return Detail::MForeach< typename Desc_t::Fields_t, Index_t, C >::Apply( callback, Index_t( Desc_t::SIZE ), Index_t( 0 ) );
}

#endif // !defined( _INCLUDE_BALL_TYPES_META_REFLECTFOREACH_HPP_ )
