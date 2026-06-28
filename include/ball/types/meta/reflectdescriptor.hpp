#ifndef _INCLUDE_BALL_TYPES_META_REFLECTDESCRIPTOR_HPP_
#	define _INCLUDE_BALL_TYPES_META_REFLECTDESCRIPTOR_HPP_

#	pragma once

#include "typeinfo.hpp"
#include "tuple.hpp"
#include "indexsequence.hpp"
#include "reflectfield.hpp"

/// Converts an ordered list of field specifications into indexed field descriptors.
template < typename I, I N, typename Fs >
struct MBuildFields;

template < typename I, I N >
struct MBuildFields< I, N, MTuple<> >
{
	using Type = MTuple<>;
};

template < typename I, I N, typename H, typename... Ts >
struct MBuildFields< I, N, MTuple< H, Ts... > >
{
	using Type = TupleCat_t<
		MTuple< MakeField_t< I, N, H > >,
		typename MBuildFields< I, N + I( 1 ), MTuple< Ts... > >::Type
	>;
};

template < typename I, typename Fs >
using BuildFields_t = typename MBuildFields< I, I( 0 ), Fs >::Type;

/// Resolves the field specification published for a specific owner/index pair.
template < typename O, typename I, I N >
using FieldSpecOf_t = decltype( MakeReflectionField( MSize< I, N >{}, static_cast< O * >( nullptr ) ) );

template < typename O, typename S >
struct MCollectFieldSpecs;

template < typename O, typename I, I... Is >
struct MCollectFieldSpecs< O, MIndexSequence< I, Is... > >
{
	using Type = MTuple< FieldSpecOf_t< O, I, Is >... >;
};

/// Collects field specifications for the half-open counter range `[B, E)`.
template < typename O, typename I, I B, I E >
struct MCollectFieldSpecsRange
{
	using Type = typename MCollectFieldSpecs< O, IndexSequenceOffset_t< I, B, MakeIndexSequence_t< I, E - B > > >::Type;
};

template < typename O, typename I, I B, I E >
using CollectFieldSpecsRange_t = typename MCollectFieldSpecsRange< O, I, B, E >::Type;

template < typename Fs, typename I >
struct MSerializedSize;

template < typename I >
struct MSerializedSize< MTuple<>, I >
{
	static constexpr I VALUE = I( 0 );
};

template < typename H, typename... Ts, typename I >
struct MSerializedSize< MTuple< H, Ts... >, I >
{
	static constexpr I VALUE = H::SERIALIZED_SIZE + MSerializedSize< MTuple< Ts... >, I >::VALUE;
};

template < typename Fs >
struct MSerializeFields;

template <>
struct MSerializeFields< MTuple<> >
{
	template < typename O, typename S >
	static constexpr bool Apply( const O &, S & ) noexcept
	{
		return true;
	}
};

template < typename H, typename... Ts >
struct MSerializeFields< MTuple< H, Ts... > >
{
	template < typename O, typename S >
	static constexpr bool Apply( const O &owner, S &storage ) noexcept
	{
		if ( !H::Serialize( owner, storage ) )
			return false;

		return MSerializeFields< MTuple< Ts... > >::Apply( owner, storage );
	}
};

template < typename Fs >
struct MDeserializeFields;

template <>
struct MDeserializeFields< MTuple<> >
{
	template < typename O, typename S, typename I >
	static constexpr bool Apply( O &, const S &, I & ) noexcept
	{
		return true;
	}
};

template < typename H, typename... Ts >
struct MDeserializeFields< MTuple< H, Ts... > >
{
	template < typename O, typename S, typename I >
	static constexpr bool Apply( O &owner, const S &storage, I &cursor ) noexcept
	{
		if ( !H::Deserialize( owner, storage, cursor ) )
			return false;

		return MDeserializeFields< MTuple< Ts... > >::Apply( owner, storage, cursor );
	}
};

/// Validates that reflected fields form a non-overlapping ordered layout.
template < typename Fs, typename I, I P >
struct MValidateFields;

template < typename I, I P >
struct MValidateFields< MTuple<>, I, P >
{
	static constexpr bool VALUE = true;
};

template < typename H, typename... Ts, typename I, I P >
struct MValidateFields< MTuple< H, Ts... >, I, P >
{
	static_assert( H::Offset() >= P, "BALL_REFLECT: field offset is before the end of the previous field" );

	static constexpr auto NEXT_END = H::Offset() + H::FIELD_SIZE;
	static constexpr bool VALUE = MValidateFields< MTuple< Ts... >, I, NEXT_END >::VALUE;
};

/// Gates the compile-time layout validation to standard-layout owners. Other owners
/// expose only run-time field offsets, which cannot be threaded as template
/// arguments, so the check is skipped for them.
template < typename O, typename Fs, typename I, bool = IS_STANDARD_LAYOUT< O > >
struct MValidateLayout
{
	static constexpr bool VALUE = true;
};

template < typename O, typename Fs, typename I >
struct MValidateLayout< O, Fs, I, true >
{
	static constexpr bool VALUE = MValidateFields< Fs, I, I( 0 ) >::VALUE;
};

/// Describes a fully reflected owner type and its field list.
template < typename I, typename N, typename O, typename Fs >
struct MClass
{
	using Index_t = I;
	using Name_t = N;
	using Owner_t = O;
	using OwnerSize_t = typename MTypeInfo< O, I >::Size_t;
	using OwnerAlign_t = typename MTypeInfo< O, I >::Align_t;
	using Fields_t = Fs;

	static constexpr I SIZE = MTypeInfo< O, I >::SIZE;
	static constexpr I ALIGN = MTypeInfo< O, I >::ALIGN;
	static constexpr I FIELD_COUNT = I( Fs::COUNT );
	static constexpr I SERIALIZED_SIZE = MSerializedSize< Fs, I >::VALUE;

	template < typename S >
	static constexpr bool Serialize( const Owner_t &owner, S &storage ) noexcept
	{
		return MSerializeFields< Fields_t >::Apply( owner, storage );
	}

	template < typename S >
	static constexpr bool Deserialize( Owner_t &owner, const S &storage ) noexcept
	{
		I cursor = 0;

		return MDeserializeFields< Fields_t >::Apply( owner, storage, cursor ) && cursor == storage.Count();
	}

	static_assert( SIZE > 0, "BALL_REFLECT: class size must be greater than zero" );
	static_assert( ALIGN > 0, "BALL_REFLECT: class align must be greater than zero" );
	static_assert( ALIGN <= SIZE, "BALL_REFLECT: class align cannot exceed class size" );
	static_assert( MValidateLayout< O, Fs, I >::VALUE, "BALL_REFLECT: invalid field layout" );
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_REFLECTDESCRIPTOR_HPP_ )
