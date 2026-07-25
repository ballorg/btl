#ifndef _INCLUDE_BALL_TYPES_META_CONSTEVALUATED_HPP_
#	define _INCLUDE_BALL_TYPES_META_CONSTEVALUATED_HPP_

#	pragma once

//	BALL_META_IS_CONSTANT_EVALUATED()
//	Compiler intrinsic reporting whether the enclosing evaluation occurs inside a
//	constant expression. Usable since C++17 on every supported toolchain
//	(Clang >= 9, GCC >= 9, MSVC >= 19.25), so it does not require C++20's
//	std::is_constant_evaluated(). Falls back to a run-time context when no
//	intrinsic is available. Internal: consumed only by IsConstantEvaluated below.
#	if defined( __has_builtin )
#		if __has_builtin( __builtin_is_constant_evaluated )
#			define BALL_META_IS_CONSTANT_EVALUATED() __builtin_is_constant_evaluated()
#		endif
#	endif

#	if !defined( BALL_META_IS_CONSTANT_EVALUATED )
#		if ( defined( _MSC_VER ) && _MSC_VER >= 1925 ) \
			|| ( defined( __GNUC__ ) && __GNUC__ >= 9 ) \
			|| ( defined( __clang__ ) && __clang_major__ >= 9 )
#			define BALL_META_IS_CONSTANT_EVALUATED() __builtin_is_constant_evaluated()
#		else
#			define BALL_META_IS_CONSTANT_EVALUATED() false
#		endif
#	endif

///-----------------------------------------------------------------------------
/// @brief Reports whether the caller is currently constant-evaluated.
///
/// @details Like the standard function, this is a trivial `constexpr` wrapper
/// around the compiler builtin. The intrinsic answers for the *caller's*
/// evaluation context, so it is read at each call site: a constant expression
/// that reaches this function sees `true`, an ordinary run-time evaluation sees
/// `false`. That is also why it cannot be a `constexpr bool` variable -- a
/// variable would freeze its value at its own (constant-evaluated) initialization
/// and read `true` everywhere.
///
/// @complexity O(1): the intrinsic folds to a compile-time boolean.
///-----------------------------------------------------------------------------
constexpr bool IsConstantEvaluated() noexcept { return BALL_META_IS_CONSTANT_EVALUATED(); }

#	undef BALL_META_IS_CONSTANT_EVALUATED

#endif // !defined( _INCLUDE_BALL_TYPES_META_CONSTEVALUATED_HPP_ )
