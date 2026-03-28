#ifndef _INCLUDE_BALL_TYPES_META_MEMBERFUNCTION_HPP_
#	define _INCLUDE_BALL_TYPES_META_MEMBERFUNCTION_HPP_

#	pragma once

template < typename T, typename R, typename ...Ts >
struct MMemberFunction
{
	using Type = R ( T::* )( Ts... );
	using ConstType = R ( T::* )( Ts... ) const;
};

template < typename T >
constexpr bool IS_MEMBER_FUNCTION_POINTER = false;

#	define BALL_MEMBER_FUNCTION_POINTER_TRAIT( ... ) template < typename T, typename R, typename ...Ts > constexpr bool IS_MEMBER_FUNCTION_POINTER< R ( T::* )( Ts... ) __VA_ARGS__ > = true

BALL_MEMBER_FUNCTION_POINTER_TRAIT();
BALL_MEMBER_FUNCTION_POINTER_TRAIT( const );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( volatile );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( const volatile );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( & );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( const & );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( volatile & );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( const volatile & );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( && );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( const && );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( volatile && );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( const volatile && );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( noexcept );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( const noexcept );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( volatile noexcept );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( const volatile noexcept );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( & noexcept );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( const & noexcept );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( volatile & noexcept );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( const volatile & noexcept );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( && noexcept );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( const && noexcept );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( volatile && noexcept );
BALL_MEMBER_FUNCTION_POINTER_TRAIT( const volatile && noexcept );

#	undef BALL_MEMBER_FUNCTION_POINTER_TRAIT

template < typename T >
constexpr bool IS_CONST_MEMBER_FUNCTION_POINTER = false;

#	define BALL_CONST_MEMBER_FUNCTION_POINTER_TRAIT( ... ) template < typename T, typename R, typename ...Ts > constexpr bool IS_CONST_MEMBER_FUNCTION_POINTER< R ( T::* )( Ts... ) const __VA_ARGS__ > = true

BALL_CONST_MEMBER_FUNCTION_POINTER_TRAIT();
BALL_CONST_MEMBER_FUNCTION_POINTER_TRAIT( volatile );
BALL_CONST_MEMBER_FUNCTION_POINTER_TRAIT( & );
BALL_CONST_MEMBER_FUNCTION_POINTER_TRAIT( volatile & );
BALL_CONST_MEMBER_FUNCTION_POINTER_TRAIT( && );
BALL_CONST_MEMBER_FUNCTION_POINTER_TRAIT( volatile && );
BALL_CONST_MEMBER_FUNCTION_POINTER_TRAIT( noexcept );
BALL_CONST_MEMBER_FUNCTION_POINTER_TRAIT( volatile noexcept );
BALL_CONST_MEMBER_FUNCTION_POINTER_TRAIT( & noexcept );
BALL_CONST_MEMBER_FUNCTION_POINTER_TRAIT( volatile & noexcept );
BALL_CONST_MEMBER_FUNCTION_POINTER_TRAIT( && noexcept );
BALL_CONST_MEMBER_FUNCTION_POINTER_TRAIT( volatile && noexcept );

#	undef BALL_CONST_MEMBER_FUNCTION_POINTER_TRAIT

template < bool IsConst, typename T, typename R, typename ...Ts >
struct MMemberFunctionByConst;

template < typename T, typename R, typename ...Ts >
struct MMemberFunctionByConst< false, T, R, Ts... >
{
	using Type = typename MMemberFunction< T, R, Ts... >::Type;
};

template < typename T, typename R, typename ...Ts >
struct MMemberFunctionByConst< true, T, R, Ts... >
{
	using Type = typename MMemberFunction< T, R, Ts... >::ConstType;
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_MEMBERFUNCTION_HPP_ )
