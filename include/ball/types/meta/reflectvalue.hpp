#ifndef _INCLUDE_BALL_TYPES_META_REFLECTVALUE_HPP_
#	define _INCLUDE_BALL_TYPES_META_REFLECTVALUE_HPP_

#	pragma once

#	include "enableif.hpp"
#	include "typeinfo.hpp"

///-----------------------------------------------------------------------------
/// @brief Declares a uniquely-typed reflected wrapper alias.
/// @details Emits a distinct empty tag `@p name Tag_t` and a
/// `CReflect< @p type, @p name Tag_t >` alias `@p name _t`. The unique tag makes
/// each declaration a separate type even when the underlying @p type is shared,
/// which is required wherever values must be addressed by type (e.g. the
/// multivector's per-column storage).
///-----------------------------------------------------------------------------
#	define BALL_REFLECT_TAGGED( name, type ) \
	struct M##name {}; \
	using name##_t = ::BTL::CReflect< type, M##name >

///-----------------------------------------------------------------------------
/// @brief Declares a uniquely-tagged reflected wrapper alias *template*.
/// @details Like BALL_REFLECT_TAGGED, but the payload type is deferred to the alias
/// template parameter: emits a distinct empty tag `M name` and an alias template
/// `name _t< T >` equal to `CReflect< T, M name >`. The tag can therefore be declared
/// at namespace scope (e.g. above a class template) while the payload type is supplied
/// per use, removing the need for a wrapper struct just to thread a template parameter.
///-----------------------------------------------------------------------------
#	define BALL_REFLECT_TAGGED_TEMPLATE( name ) \
	struct M##name {}; \
	template < typename TReflectType > \
	using name##_t = ::BTL::CReflect< TReflectType, M##name >

template < typename S, typename T > constexpr bool ReflectSerializeValue( S &storage, const T &value ) noexcept;
template < typename S, typename I, typename T > constexpr bool ReflectDeserializeValue( T &value, const S &storage, I &cursor ) noexcept;

/// Empty metadata base used by reflected values without a bound field descriptor.
struct MReflectNone {};

/// Value-like storage wrapper used by reflected fields.
/// It keeps a dedicated hook point for future serialization logic
/// while preserving ordinary assignment and implicit value access.
template < typename T, typename F = MReflectNone >
class CReflect : public F
{
public:
	using Type = T;
	using Field_t = F;

protected:
	T m_Value;

public:
	constexpr CReflect() = default;
	constexpr CReflect( const CReflect & ) = default;
	constexpr CReflect( CReflect && ) = default;
	constexpr CReflect &operator=( const CReflect & ) = default;
	constexpr CReflect &operator=( CReflect && ) = default;

	constexpr CReflect( const T &v ) noexcept : m_Value( v )
	{
	}

	constexpr CReflect( T &&v ) noexcept : m_Value( static_cast< T && >( v ) )
	{
	}

	constexpr operator T &() noexcept { return m_Value; }
	constexpr operator const T &() const noexcept { return m_Value; }

	template < typename V >
	constexpr CReflect &operator=( V &&v ) noexcept
	{
		m_Value = static_cast< V && >( v );

		return *this;
	}

	constexpr T &Get() noexcept { return m_Value; }
	constexpr const T &Get() const noexcept { return m_Value; }

	template < typename S >
	constexpr bool Serialize( S &storage ) const noexcept
	{
		return ReflectSerializeValue( storage, m_Value );
	}

	template < typename S, typename I >
	constexpr bool Deserialize( const S &storage, I &cursor ) noexcept
	{
		return ReflectDeserializeValue( m_Value, storage, cursor );
	}

	template < typename S >
	constexpr bool Deserialize( const S &storage ) noexcept
	{
		typename S::Index_t cursor = static_cast< typename S::Index_t >( 0 );

		return Deserialize( storage, cursor ) && cursor == static_cast< typename S::Index_t >( storage.Count() );
	}

	template < typename U = T, typename = EnableIf_t< MTypeInfo< U >::IS_CLASS_OR_UNION > >
	constexpr U *operator->() noexcept
	{
		return &m_Value;
	}

	template < typename U = T, typename = EnableIf_t< MTypeInfo< U >::IS_CLASS_OR_UNION > >
	constexpr const U *operator->() const noexcept
	{
		return &m_Value;
	}
};

/// Extracts the payload type from a reflected storage wrapper.
template < typename T >
struct MReflectType
{
	using Type = T;
};

template < typename T, typename F >
struct MReflectType< CReflect< T, F > >
{
	using Type = T;
};

template < typename T >
using ReflectValue_t = typename MReflectType< T >::Type;

template < typename T >
constexpr T &ReflectValueAccess( T &v ) noexcept
{
	return ( v );
}

template < typename T >
constexpr const T &ReflectValueAccess( const T &v ) noexcept
{
	return ( v );
}

template < typename T, typename F >
constexpr T &ReflectValueAccess( CReflect< T, F > &v ) noexcept
{
	return ( v.Get() );
}

template < typename T, typename F >
constexpr const T &ReflectValueAccess( const CReflect< T, F > &v ) noexcept
{
	return ( v.Get() );
}

template < typename T, typename V >
constexpr void ReflectValueAssign( T &dst, V &&v ) noexcept
{
	dst = static_cast< V && >( v );
}

template < typename T, typename F, typename V >
constexpr void ReflectValueAssign( CReflect< T, F > &dst, V &&v ) noexcept
{
	dst = static_cast< V && >( v );
}

template < typename S, typename T >
constexpr bool ReflectSerializeValue( S &storage, const T &value ) noexcept
{
	static_assert( MTypeInfo< T >::IS_COPYABLE, "BALL_REFLECT: only trivially copyable values can use raw serialization" );

	const unsigned char *pBytes = reinterpret_cast< const unsigned char * >( &value );

	for ( typename S::Index_t i = 0; i < sizeof( T ); ++i )
		storage.AddToTail( pBytes[ i ] );

	return true;
}

template < typename S, typename I, typename T >
constexpr bool ReflectDeserializeValue( T &value, const S &storage, I &nCursor ) noexcept
{
	static_assert( MTypeInfo< T >::IS_COPYABLE, "BALL_REFLECT: only trivially copyable values can use raw deserialization" );

	const I nSize = I( sizeof( T ) );

	if ( nCursor + nSize > I( storage.Count() ) )
		return false;

	unsigned char *pValue = reinterpret_cast< unsigned char * >( &value );
	const unsigned char *pStorage = reinterpret_cast< const unsigned char * >( storage.Base() );

	for ( I i = 0; i < nSize; ++i )
		pValue[ i ] = pStorage[ nCursor + i ];

	nCursor += nSize;

	return true;
}

#endif // !defined( _INCLUDE_BALL_TYPES_META_REFLECTVALUE_HPP_ )
