#ifndef _INCLUDE_BALL_TYPES_REFLECT_HPP_

#	define _INCLUDE_BALL_TYPES_REFLECT_HPP_

#	pragma once

#	include "base/arch.h"
#	include "base/fixed.h"
#	include "c/macros.h"
#	include "meta/size.hpp"
#	include "meta/tuple.hpp"
#	include "meta/reflectvalue.hpp"
#	include "meta/reflectfield.hpp"
#	include "meta/reflectdescriptor.hpp"
#	include "meta/reflecttraits.hpp"
#	include "meta/reflectforeach.hpp"

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
		static constexpr Value_t &Get( Owner_t &owner ) noexcept { return BTL::ReflectValueAccess( owner.FIELD_NAME ); } \
		static constexpr const Value_t &Get( const Owner_t &owner ) noexcept { return BTL::ReflectValueAccess( owner.FIELD_NAME ); } \
		template < typename TValue > static constexpr void Set( Owner_t &owner, TValue &&value ) noexcept { BTL::ReflectValueAssign( owner.FIELD_NAME, static_cast< TValue && >( value ) ); } \
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
		static constexpr Value_t &Get( Owner_t &owner ) noexcept { return BTL::ReflectValueAccess( owner.FIELD_NAME ); } \
		static constexpr const Value_t &Get( const Owner_t &owner ) noexcept { return BTL::ReflectValueAccess( owner.FIELD_NAME ); } \
		template < typename TValue > \
		static constexpr void Set( Owner_t &owner, TValue &&value ) noexcept { BTL::ReflectValueAssign( owner.FIELD_NAME, static_cast< TValue && >( value ) ); } \
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
