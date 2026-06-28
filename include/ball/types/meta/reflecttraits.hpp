#ifndef _INCLUDE_BALL_TYPES_META_REFLECTTRAITS_HPP_
#	define _INCLUDE_BALL_TYPES_META_REFLECTTRAITS_HPP_

#	pragma once

#	include "reflectdescriptor.hpp"

/// Canonical `void` alias used for SFINAE-based detection.
template < typename... >
using Void_t = void;

/// Detects whether a type publishes the reflection entry points expected by this module.
template < typename O, typename = void >
struct MIsReflectable
{
	static constexpr bool VALUE = false;
};

template < typename O >
struct MIsReflectable< O, Void_t<
	typename O::ReflectionSentinel_t,
	decltype( MakeReflectionDescriptor( static_cast< O * >( nullptr ) ) )
> >
{
	static constexpr bool VALUE = true;
};

template < typename O >
static constexpr bool IS_REFLECTABLE = MIsReflectable< O >::VALUE;

#if __cplusplus >= 202002L
template < typename O > concept Reflectable_t = IS_REFLECTABLE< O >;
#endif

/// Resolves the descriptor type associated with a reflectable owner type.
template < typename O >
struct MReflectOf
{
	static_assert( IS_REFLECTABLE< O >, "M_REFLECT: type is not reflectable. Did you forget M_REFLECT?" );

	using Type = decltype( MakeReflectionDescriptor( static_cast< O * >( nullptr ) ) );
};

template < typename O >
using Reflect_t = typename MReflectOf< O >::Type;

template < typename O >
using ReflectOf_t = typename MReflectOf< O >::Type;

#endif // !defined( _INCLUDE_BALL_TYPES_META_REFLECTTRAITS_HPP_ )
