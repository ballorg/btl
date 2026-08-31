#ifndef _INCLUDE_BALL_TYPES_REFLECT_HPP_

#	define _INCLUDE_BALL_TYPES_REFLECT_HPP_

#	pragma once

// ---------------------------------------------------------------------------
// Reflected value primitives (merged from meta/reflectvalue.hpp). Defined here,
// ahead of the field/descriptor cluster below, which depends on them.
// ---------------------------------------------------------------------------
template < typename S, typename T > constexpr bool ReflectSerialize( S &storage, const T &value ) noexcept;
template < typename S, typename I, typename T > constexpr bool ReflectDeserialize( T &value, const S &storage, I &cursor ) noexcept;

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
		return ReflectSerialize( storage, m_Value );
	}

	template < typename S, typename I >
	constexpr bool Deserialize( const S &storage, I &cursor ) noexcept
	{
		return ReflectDeserialize( m_Value, storage, cursor );
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

template < typename T >
constexpr T &ReflectAccess( T &v ) noexcept
{
	return ( v );
}

template < typename T >
constexpr const T &ReflectAccess( const T &v ) noexcept
{
	return ( v );
}

template < typename T, typename F >
constexpr T &ReflectAccess( CReflect< T, F > &v ) noexcept
{
	return ( v.Get() );
}

template < typename T, typename F >
constexpr const T &ReflectAccess( const CReflect< T, F > &v ) noexcept
{
	return ( v.Get() );
}

template < typename T, typename V >
constexpr void ReflectAssign( T &dst, V &&v ) noexcept
{
	dst = static_cast< V && >( v );
}

template < typename T, typename F, typename V >
constexpr void ReflectAssign( CReflect< T, F > &dst, V &&v ) noexcept
{
	dst = static_cast< V && >( v );
}

template < typename S, typename T >
constexpr bool ReflectSerialize( S &storage, const T &value ) noexcept
{
	static_assert( MTypeInfo< T >::IS_COPYABLE, "BALL_REFLECT: only trivially copyable values can use raw serialization" );

	const unsigned char *pBytes = reinterpret_cast< const unsigned char * >( &value );

	for ( typename S::Index_t i = 0; i < sizeof( T ); ++i )
		storage.AddToTail( pBytes[ i ] );

	return true;
}

template < typename S, typename I, typename T >
constexpr bool ReflectDeserialize( T &value, const S &storage, I &nCursor ) noexcept
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


#endif // !defined( _INCLUDE_BALL_TYPES_REFLECT_HPP_ )
