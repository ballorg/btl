#ifndef _INCLUDE_BALL_META_CONDITIONAL_HPP_
#	define _INCLUDE_BALL_META_CONDITIONAL_HPP_

// MConditional is a minimal, dependency-free replacement for std::conditional.
// If B is true, MConditional<B, T, F>::type == T; otherwise == F.
// The alias Conditional_t<B, T, F> is a convenient shorthand.
//
// Design goals:
// - header-only, no STL
// - trivial to instantiate
// - works in constant-expression contexts

template < bool B, typename T, typename F >
struct MConditional
{
	using Type = T;
};

template < typename T, typename F >
struct MConditional< false, T, F >
{
	using Type = F;
};

template < bool B, typename T, typename F >
using Conditional_t = typename MConditional<B, T, F>::Type;

#endif // !defined( _INCLUDE_BALL_META_CONDITIONAL_HPP_ )
