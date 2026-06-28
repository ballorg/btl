#ifndef _INCLUDE_BALL_TYPES_META_REFLECTFIELD_HPP_
#	define _INCLUDE_BALL_TYPES_META_REFLECTFIELD_HPP_

#	pragma once

#	include "size.hpp"
#	include "isstandardlayout.hpp"
#	include "reflectvalue.hpp"

/// Measures the byte offset of a member within @p O at run time.
///
/// @p C is the class that actually declares the member (the owner itself or one of
/// its bases). The pointer-to-member is rebased onto the most-derived owner @p O, so
/// a field inherited through `BALL_REFLECT_BASE` reports its offset within the
/// complete object. This path exists for non-standard-layout owners, where offsetof
/// is ill-formed; standard-layout owners use offsetof and never reach it.
template < typename O, typename C, typename M >
inline decltype( sizeof( O ) ) ReflectMemberOffset( M C::*member ) noexcept
{
	M O::*const pm = member; // base-to-derived pointer-to-member conversion

	// A non-null probe address keeps the member-access well-defined; the object is
	// never read, only the address of the subobject is taken.
	const O *const probe = reinterpret_cast< const O * >( alignof( O ) );

	return static_cast< decltype( sizeof( O ) ) >(
		reinterpret_cast< const char * >( &( probe->*pm ) ) - reinterpret_cast< const char * >( probe ) );
}

/// Offset provider carrying a fixed compile-time offset, used where the layout is
/// already known (e.g. a manually assembled field descriptor).
template < typename I, I V >
struct MConstFieldOffset
{
	template < typename O >
	static constexpr I Get() noexcept { return V; }
};

/// Offset provider that always measures at run time from a pointer-to-member. Used
/// by the type-only field-spec macros, which have no enclosing scope in which to
/// emit a provider able to call offsetof on the standard-layout fast path.
template < typename O, typename PtrT, PtrT M >
struct MRuntimeFieldOffset
{
	template < typename U = O >
	static auto Get() noexcept { return ReflectMemberOffset< U >( M ); }
};

/// Forward declaration of the field-name view. Only the declaration is needed by
/// these field-metadata templates; the full definition (in stringview.hpp) is
/// required at the point a reflected field is materialized. The default for `N`
/// is provided by that definition, so it is intentionally omitted here.
template < typename I, typename T, I N >
class CStringView;

/// Describes a reflected member before a stable field index is assigned.
///
/// @p NAME is a reference to a `static constexpr CStringView` that stores the
/// field name; it is threaded through the metadata as a non-type template
/// argument instead of a dedicated name-literal type.
template < typename I, typename F, typename O, typename D, const CStringView< I, const char, 0 > &NAME, typename A, typename OFF >
struct MFieldSpec
{
	using Index_t = I;
	using Field_t = F;
	using Value_t = ReflectValue_t< F >;
	using Owner_t = O;
	using DeclaredIn_t = D;
	using Name_t = CStringView< I, const char, 0 >;
	using Accessor_t = A;
	using Offset_t = OFF;

	static constexpr const Name_t &NAME_VIEW = NAME;

	static constexpr auto FIELD_SIZE = sizeof( F );

	static constexpr MFieldSpec GetDesc() noexcept { return {}; }

	static constexpr I Offset() noexcept { return OFF::template Get< O >(); }
	static constexpr I Size() noexcept { return FIELD_SIZE; }
	static constexpr auto SerializedSize() noexcept { return sizeof( Value_t ); }
	static constexpr auto Align() noexcept { return alignof( Field_t ); }
	static constexpr auto ValueAlign() noexcept { return alignof( Value_t ); }
	static constexpr const Name_t &Name() noexcept { return NAME; }
};

template < typename I, typename F, typename O, typename D, const CStringView< I, const char, 0 > &NAME, typename A, typename OFF >
using FieldSpec_t = MFieldSpec< I, F, O, D, NAME, A, OFF >;

/// Lazy field specification reference used before the owning class is complete.
template < typename O, typename I, I N, const CStringView< I, const char, 0 > &NAME >
struct MFieldSpecByIndex
{
	using Owner_t = O;
	using Index_t = I;
	using Name_t = CStringView< I, const char, 0 >;
	using NameView_t = Name_t;

	static constexpr const Name_t &NAME_VIEW = NAME;

	static constexpr auto GetDesc() noexcept
	{
		return MakeReflectionField( MSize< I, N >{}, static_cast< O * >( nullptr ) ).GetDesc();
	}

	constexpr auto Offset() const noexcept { return GetDesc().Offset(); }
	constexpr auto Size() const noexcept { return GetDesc().Size(); }
	constexpr auto SerializedSize() const noexcept { return GetDesc().SerializedSize(); }
	constexpr auto Align() const noexcept { return GetDesc().Align(); }
	constexpr auto ValueAlign() const noexcept { return GetDesc().ValueAlign(); }
	constexpr Name_t Name() const noexcept { return NAME; }
};

/// Static metadata carried by a finalized reflected field. Also serves as the
/// lightweight descriptor returned by `MField::GetDesc()`: it exposes the field's
/// metadata through value-semantic accessors without carrying the accessor functor.
template < typename I, typename F, typename O, typename D, const CStringView< I, const char, 0 > &NAME, I N, typename OFFSET, I SIZE >
struct MFieldStaticDesc
{
	using Index_t = I;
	using Field_t = F;
	using Value_t = ReflectValue_t< F >;
	using Owner_t = O;
	using DeclaredIn_t = D;
	using Name_t = CStringView< I, const char, 0 >;
	using NameView_t = Name_t;
	using Offset_t = OFFSET;

	static constexpr const Name_t &NAME_VIEW = NAME;

	static constexpr I INDEX = N;
	static constexpr I FIELD_INDEX = N;
	static constexpr I FIELD_SIZE = SIZE;
	static constexpr I SERIALIZED_SIZE = I( sizeof( Value_t ) );

	static constexpr I Offset() noexcept { return Offset_t::template Get< O >(); }

	constexpr Index_t Index() const noexcept { return INDEX; }
	constexpr Index_t Size() const noexcept { return FIELD_SIZE; }
	constexpr Index_t SerializedSize() const noexcept { return SERIALIZED_SIZE; }
	constexpr auto Align() const noexcept { return alignof( Field_t ); }
	constexpr auto ValueAlign() const noexcept { return alignof( Value_t ); }
	constexpr Name_t Name() const noexcept { return NAME_VIEW; }
};

/// Provides uniform compile-time get/set access to a member pointer.
template < typename O, typename F, F O::*M >
struct MMemberAccessor
{
	using Owner_t = O;
	using Value_t = ReflectValue_t< F >;

	static constexpr Value_t &Get( Owner_t &owner ) noexcept { return ReflectValueAccess( owner.*M ); }
	static constexpr const Value_t &Get( const Owner_t &owner ) noexcept { return ReflectValueAccess( owner.*M ); }

	template < typename V >
	static constexpr void Set( Owner_t &owner, V &&value ) noexcept
	{
		ReflectValueAssign( owner.*M, static_cast< V && >( value ) );
	}
};

/// Final reflected field with an assigned sequence index.
template < typename D, typename A >
struct MField : D
{
	using Desc_t = D;
	using Index_t = typename Desc_t::Index_t;
	using Field_t = typename Desc_t::Field_t;
	using Value_t = typename Desc_t::Value_t;
	using Owner_t = typename Desc_t::Owner_t;
	using DeclaredIn_t = typename Desc_t::DeclaredIn_t;
	using Name_t = typename Desc_t::Name_t;
	using NameView_t = Name_t;
	using Accessor_t = A;
	using FieldDesc_t = Desc_t;

	static constexpr const Name_t &NAME_VIEW = Desc_t::NAME_VIEW;

	static constexpr Index_t INDEX = Desc_t::INDEX;
	static constexpr Index_t FIELD_INDEX = Desc_t::FIELD_INDEX;
	static constexpr Index_t FIELD_SIZE = Desc_t::FIELD_SIZE;
	static constexpr Index_t SIZE = FIELD_SIZE;
	static constexpr Index_t SERIALIZED_SIZE = Desc_t::SERIALIZED_SIZE;

	static constexpr FieldDesc_t GetDesc() noexcept { return {}; }

	static constexpr Index_t Index() noexcept { return INDEX; }
	static constexpr Index_t Offset() noexcept { return Desc_t::Offset(); }
	static constexpr Index_t Size() noexcept { return SIZE; }
	static constexpr Index_t SerializedSize() noexcept { return SERIALIZED_SIZE; }
	static constexpr auto Align() noexcept { return alignof( Field_t ); }
	static constexpr auto ValueAlign() noexcept { return alignof( Value_t ); }
	static constexpr Name_t Name() noexcept { return NAME_VIEW; }

	static constexpr Value_t &Get( Owner_t &owner ) noexcept { return Accessor_t::Get( owner ); }
	static constexpr const Value_t &Get( const Owner_t &owner ) noexcept { return Accessor_t::Get( owner ); }

	constexpr Value_t &operator()( Owner_t &owner ) const noexcept { return Get( owner ); }
	constexpr const Value_t &operator()( const Owner_t &owner ) const noexcept { return Get( owner ); }
	constexpr Value_t &operator[]( Owner_t &owner ) const noexcept { return Get( owner ); }
	constexpr const Value_t &operator[]( const Owner_t &owner ) const noexcept { return Get( owner ); }

	template < typename V >
	static constexpr void Set( Owner_t &owner, V &&value ) noexcept
	{
		Accessor_t::Set( owner, static_cast< V && >( value ) );
	}

	template < typename S >
	static constexpr bool Serialize( const Owner_t &owner, S &storage ) noexcept
	{
		return ReflectSerializeValue( storage, Get( owner ) );
	}

	template < typename S >
	constexpr bool SerializeValue( const Owner_t &owner, S &storage ) const noexcept
	{
		return Serialize( owner, storage );
	}

	template < typename S >
	static constexpr bool Deserialize( Owner_t &owner, const S &storage, Index_t &cursor ) noexcept
	{
		return ReflectDeserializeValue( Get( owner ), storage, cursor );
	}

	template < typename S >
	constexpr bool DeserializeValue( Owner_t &owner, const S &storage, Index_t &cursor ) const noexcept
	{
		return Deserialize( owner, storage, cursor );
	}

	// Field offsets are only constant expressions for standard-layout owners; the
	// bounds checks below short-circuit (and so never evaluate Offset()) otherwise.
	static_assert( !IS_STANDARD_LAYOUT< Owner_t > || Offset() < Index_t( sizeof( Owner_t ) ), "BALL_REFLECT: field offset must be within the owner object" );
	static_assert( !IS_STANDARD_LAYOUT< Owner_t > || Offset() + SIZE <= Index_t( sizeof( Owner_t ) ), "BALL_REFLECT: field extends past the owner object" );
	static_assert( SIZE > 0, "BALL_REFLECT: field size must be greater than zero" );
	static_assert( sizeof( Field_t ) == SIZE, "BALL_REFLECT: registered field size does not match sizeof( TField )" );
	static_assert( sizeof( Value_t ) <= SIZE, "BALL_REFLECT: serialized value size cannot exceed field storage size" );
	static_assert( alignof( Field_t ) > 0, "BALL_REFLECT: field align must be greater than zero" );
};

/// Materializes `MField` from a field specification and a positional index.
template < typename I, I N, typename S >
struct MMakeField;

template < typename I, I N, typename F, typename O, typename D, const CStringView< I, const char, 0 > &NAME, typename A, typename OFF >
struct MMakeField< I, N, MFieldSpec< I, F, O, D, NAME, A, OFF > >
{
	using Desc_t = MFieldStaticDesc< I, F, O, D, NAME, N, OFF, I( sizeof( F ) ) >;
	using Type = MField< Desc_t, A >;
};

template < typename I, I N, typename S > using MakeField_t = typename MMakeField< I, N, S >::Type;

#endif // !defined( _INCLUDE_BALL_TYPES_META_REFLECTFIELD_HPP_ )
