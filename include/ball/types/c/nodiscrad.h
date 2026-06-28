#ifndef _INCLUDE_BALL_TYPES_C_NODISCARD_H_
#	define _INCLUDE_BALL_TYPES_C_NODISCARD_H_

#	if __cplusplus >= 201703L
#		define BALL_NO_DISCARD [[ nodiscard ]]
#	else
#		define BALL_NO_DISCARD
#	endif

#endif // !defined( _INCLUDE_BALL_TYPES_C_NODISCARD_H_ )
