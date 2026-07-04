#ifndef _INCLUDE_BALL_TYPES_REFLECT_HPP_

#	define _INCLUDE_BALL_TYPES_REFLECT_HPP_

#	pragma once

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "c/macros.h"
#	include "meta/enableif.hpp"
#	include "meta/reflectfield.hpp"
#	include "meta/reflectdescriptor.hpp"
#	include "meta/reflecttraits.hpp"
#	include "meta/reflectforeach.hpp"
#	include "meta/typeinfo.hpp"
#	include "meta/size.hpp"
#	include "meta/tuple.hpp"

// ---------------------------------------------------------------------------
// Reflected value primitives (merged from meta/reflectvalue.hpp). Defined here,
// ahead of the field/descriptor cluster below, which depends on them.
// ---------------------------------------------------------------------------
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

// Emits a per-field offset provider. offsetof is only used on the standard-layout
// fast path, where it is well-defined and free of -Winvalid-offsetof; for
// non-standard-layout owners (which BALL_REFLECT_BASE produces) the offset is
// measured at run time from a pointer-to-member. The owner is a template parameter
// so the discarded if-constexpr branch is never instantiated, keeping offsetof out
// of the non-standard-layout case entirely.
#	define BALL_REFLECT_OFFSET_PROVIDER( PROVIDER_NAME, FIELD_NAME ) \
	struct PROVIDER_NAME \
	{ \
		template < typename O = OwnerType_t > \
		static constexpr ReflectionIndex_t Get() noexcept \
		{ \
			if constexpr ( BTL::IS_STANDARD_LAYOUT< O > ) \
				return ReflectionIndex_t( __builtin_offsetof( O, FIELD_NAME ) ); \
			else \
				return ReflectionIndex_t( BTL::ReflectMemberOffset< O >( &O::FIELD_NAME ) ); \
		} \
	}

#	define BALL_REFLECT_BEGIN( OWNER_TYPE ) \
	using OwnerType_t = OWNER_TYPE; \
	using ReflectionIndex_t = BTL::size32_t; \
	using ReflectionSentinel_t = void; \
	static constexpr ReflectionIndex_t M_REFLECT_BEGIN_N = __COUNTER__;

#	define BALL_REFLECT_BASE( DECLARED_IN, FIELD_NAME ) \
	struct FIELD_NAME##_Accessor_t \
	{ \
		using Owner_t = OwnerType_t; \
		using Value_t = BTL::ReflectValue_t< decltype( static_cast< DECLARED_IN * >( nullptr )->FIELD_NAME ) >; \
		static constexpr Value_t &Get( Owner_t &owner ) noexcept { return BTL::ReflectAccess( owner.FIELD_NAME ); } \
		static constexpr const Value_t &Get( const Owner_t &owner ) noexcept { return BTL::ReflectAccess( owner.FIELD_NAME ); } \
		template < typename TValue > static constexpr void Set( Owner_t &owner, TValue &&value ) noexcept { BTL::ReflectAssign( owner.FIELD_NAME, static_cast< TValue && >( value ) ); } \
		static constexpr ReflectionIndex_t INDEX = __COUNTER__; \
	}; \
	static constexpr BTL::CStringView< ReflectionIndex_t, const char > FIELD_NAME##_Name{ #FIELD_NAME }; \
	BALL_REFLECT_OFFSET_PROVIDER( FIELD_NAME##_Offset_t, FIELD_NAME ); \
	friend constexpr auto MakeReflectionField( BTL::MSize< ReflectionIndex_t, FIELD_NAME##_Accessor_t::INDEX >, OwnerType_t * ) \
	{ \
		using FieldSpec_t = BTL::FieldSpec_t< \
			ReflectionIndex_t, \
			BTL::ReflectValue_t< decltype( static_cast< DECLARED_IN * >( nullptr )->FIELD_NAME ) >, \
			OwnerType_t, \
			DECLARED_IN, \
			FIELD_NAME##_Name, \
			FIELD_NAME##_Accessor_t, \
			FIELD_NAME##_Offset_t \
		>; \
		\
		return FieldSpec_t{}; \
	}

#	define BALL_REFLECT_FIELD( FIELD_TYPE, FIELD_NAME ) \
	struct FIELD_NAME##_Accessor_t \
	{ \
		using Owner_t = OwnerType_t; \
		using Value_t = FIELD_TYPE; \
		static constexpr Value_t &Get( Owner_t &owner ) noexcept { return BTL::ReflectAccess( owner.FIELD_NAME ); } \
		static constexpr const Value_t &Get( const Owner_t &owner ) noexcept { return BTL::ReflectAccess( owner.FIELD_NAME ); } \
		template < typename TValue > static constexpr void Set( Owner_t &owner, TValue &&value ) noexcept { BTL::ReflectAssign( owner.FIELD_NAME, static_cast< TValue && >( value ) ); } \
		static constexpr ReflectionIndex_t INDEX = __COUNTER__; \
	}; \
	static constexpr BTL::CStringView< ReflectionIndex_t, const char > FIELD_NAME##_Name{ #FIELD_NAME }; \
	using FIELD_NAME##_Spec_t = BTL::MFieldSpecByIndex< OwnerType_t, ReflectionIndex_t, FIELD_NAME##_Accessor_t::INDEX, FIELD_NAME##_Name >; \
	BTL::CReflect< FIELD_TYPE, FIELD_NAME##_Spec_t > FIELD_NAME; \
	BALL_REFLECT_OFFSET_PROVIDER( FIELD_NAME##_Offset_t, FIELD_NAME ); \
	friend constexpr auto MakeReflectionField( BTL::MSize< ReflectionIndex_t, FIELD_NAME##_Accessor_t::INDEX >, OwnerType_t * ) \
	{ \
		using FieldSpec_t = BTL::FieldSpec_t< \
			ReflectionIndex_t, \
			FIELD_TYPE, \
			OwnerType_t, \
			OwnerType_t, \
			FIELD_NAME##_Name, \
			FIELD_NAME##_Accessor_t, \
			FIELD_NAME##_Offset_t \
		>; \
		return FieldSpec_t{}; \
	}

#	define BALL_REFLECT_END( OWNER_TYPE ) \
	friend constexpr auto MakeReflectionDescriptor( OWNER_TYPE * ) \
	{ \
		using Descriptor_t = BTL::MClass< \
			ReflectionIndex_t, \
			BTL::CStringView< ReflectionIndex_t, const char >, \
			OWNER_TYPE, \
			BTL::BuildFields_t< ReflectionIndex_t, BTL::CollectFieldSpecsRange_t< OWNER_TYPE, ReflectionIndex_t, M_REFLECT_BEGIN_N + 1, __COUNTER__ > > \
		>; \
		return Descriptor_t{}; \
	} \
	template < typename S, typename O = OWNER_TYPE > constexpr bool Serialize( S &storage ) const noexcept { return BTL::Reflect_t< O >::Serialize( *this, storage ); } \
	template < typename S, typename O = OWNER_TYPE > constexpr bool Deserialize( const S &storage ) noexcept { return BTL::Reflect_t< O >::Deserialize( *this, storage ); }

#	define BALL_REFLECT_TYPE( OWNER_TYPE, ... ) \
	using ReflectionIndex_t = BTL::size32_t; \
	using ReflectionSentinel_t = void; \
	friend constexpr auto MakeReflectionDescriptor( OWNER_TYPE * ) \
	{ \
		using Descriptor_t = BTL::MClass< \
			ReflectionIndex_t, \
			BTL::CStringView< ReflectionIndex_t, const char >, \
			OWNER_TYPE, \
			BTL::BuildFields_t< ReflectionIndex_t, BTL::MTuple< __VA_ARGS__ > > \
		>; \
		\
		return Descriptor_t{}; \
	} \
	template < typename S, typename O = OWNER_TYPE > constexpr bool Serialize( S &storage ) const noexcept { return BTL::Reflect_t< O >::Serialize( *this, storage ); } \
	template < typename S, typename O = OWNER_TYPE > constexpr bool Deserialize( const S &storage ) noexcept { return BTL::Reflect_t< O >::Deserialize( *this, storage ); }

#	define BALL_REFLECT_FIELD_SPEC( OWNER, DECLARED_IN, FIELD_NAME, NAME_CHARS ) \
	BTL::FieldSpec_t< \
		BTL::size32_t, \
		BTL::ReflectValue_t< decltype( static_cast< OWNER * >( nullptr )->FIELD_NAME ) >, \
		OWNER, \
		DECLARED_IN, \
		NAME_CHARS, \
		BTL::MMemberAccessor< OWNER, decltype( static_cast< OWNER * >( nullptr )->FIELD_NAME ), &OWNER::FIELD_NAME >, \
		BTL::MRuntimeFieldOffset< OWNER, decltype( &OWNER::FIELD_NAME ), &OWNER::FIELD_NAME > \
	>

#	define BALL_REFLECT_BASE_FIELD_SPEC( OWNER, DECLARED_IN, FIELD_NAME, NAME_CHARS ) \
	BALL_REFLECT_FIELD_SPEC( OWNER, DECLARED_IN, FIELD_NAME, NAME_CHARS )

#	define BALL_REFLECT( FIELD_TYPE, FIELD_NAME ) BALL_REFLECT_FIELD( FIELD_TYPE, FIELD_NAME )
#	define BALL_REFLECT_BASE_FIELD( OWNER, DECLARED_IN, FIELD_NAME, NAME_CHARS ) BALL_REFLECT_BASE_FIELD_SPEC( OWNER, DECLARED_IN, FIELD_NAME, NAME_CHARS )

#endif // !defined( _INCLUDE_BALL_TYPES_REFLECT_HPP_ )
