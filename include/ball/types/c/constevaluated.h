#ifndef _INCLUDE_BALL_TYPES_C_CONSTEVALUATED_H_
#	define _INCLUDE_BALL_TYPES_C_CONSTEVALUATED_H_

//	BALL_IS_CONSTANT_EVALUATED()
//	Reports whether the enclosing evaluation occurs inside a constant
//	expression. Maps to the compiler intrinsic, which is usable since C++17 on
//	every supported toolchain (Clang >= 9, GCC >= 9, MSVC >= 19.25), so it does
//	not require C++20's std::is_constant_evaluated(). Falls back to a run-time
//	context when no intrinsic is available
#	if defined( __has_builtin )
#		if __has_builtin( __builtin_is_constant_evaluated )
#			define BALL_IS_CONSTANT_EVALUATED() __builtin_is_constant_evaluated()
#		endif
#	endif

#	if !defined( BALL_IS_CONSTANT_EVALUATED )
#		if ( defined( _MSC_VER ) && _MSC_VER >= 1925 ) \
			|| ( defined( __GNUC__ ) && __GNUC__ >= 9 ) \
			|| ( defined( __clang__ ) && __clang_major__ >= 9 )
#			define BALL_IS_CONSTANT_EVALUATED() __builtin_is_constant_evaluated()
#		else
#			define BALL_IS_CONSTANT_EVALUATED() false
#		endif
#	endif

#endif // !defined( _INCLUDE_BALL_TYPES_C_CONSTEVALUATED_H_ )
