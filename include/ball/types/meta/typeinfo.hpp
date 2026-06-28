#ifndef _INCLUDE_BALL_TYPES_META_TYPEINFO_HPP_
#	define _INCLUDE_BALL_TYPES_META_TYPEINFO_HPP_

#	pragma once

#	include "size.hpp"
#	include "removecv.hpp"
#	include "removereference.hpp"
#	include "isclass.hpp"
#	include "isunion.hpp"
#	include "isclassorunion.hpp"
#	include "istrivial.hpp"
#	include "istriviallycopyable.hpp"
#	include "ismemmovesafe.hpp"
#	include "isdefaultconstructible.hpp"
#	include "istriviallyconstructible.hpp"
#	include "istriviallydestructible.hpp"
#	include "iscopyconstructible.hpp"
#	include "iscopyassignable.hpp"
#	include "ismoveconstructible.hpp"
#	include "ismoveassignable.hpp"

#	if defined( _MSC_VER )
#		if defined( _WIN64 )
using TypeInfoDefaultIndex_t = unsigned __int64;
#		else
using TypeInfoDefaultIndex_t = unsigned int;
#		endif
#	elif defined( __SIZE_TYPE__ )
using TypeInfoDefaultIndex_t = __SIZE_TYPE__;
#	else
using TypeInfoDefaultIndex_t = unsigned long;
#	endif

template < typename T, typename I = TypeInfoDefaultIndex_t >
struct MTypeInfo
{
	using Index_t = I;
	using Type = T; // RemoveCV_t< RemoveReference_t< T > >;

	static constexpr I SIZE = I( sizeof( Type ) );
	static constexpr I ALIGN = I( alignof( Type ) );

	using Size_t = MSize< I, SIZE >;
	using Align_t = MSize< I, ALIGN >;

	static constexpr bool IS_CLASS = MIsClass< Type >::VALUE;
	static constexpr bool IS_UNION = MIsUnion< Type >::VALUE;
	static constexpr bool IS_CLASS_OR_UNION = MIsClassOrUnion< Type >::VALUE;

	static constexpr bool IS_TRIVIAL = MIsTrivial< Type >::VALUE;
	static constexpr bool IS_COPYABLE = MIsTriviallyCopyable< Type >::VALUE;
	static constexpr bool IS_MEMMOVE_SAFE = MIsMemmoveSafe< Type >::VALUE;

	static constexpr bool IS_DEFAULT_CONSTRUCTIBLE = MIsDefaultConstructible< Type >::VALUE;
	static constexpr bool IS_TRIVIALLY_CONSTRUCTIBLE = MIsTriviallyConstructible< Type >::VALUE;
	static constexpr bool IS_TRIVIALLY_DESTRUCTIBLE = MIsTriviallyDestructible< Type >::VALUE;

	static constexpr bool IS_COPY_CONSTRUCTIBLE = MIsCopyConstructible< Type >::VALUE;
	static constexpr bool IS_COPY_ASSIGNABLE = MIsCopyAssignable< Type >::VALUE;
	static constexpr bool IS_MOVE_CONSTRUCTIBLE = MIsMoveConstructible< Type >::VALUE;
	static constexpr bool IS_MOVE_ASSIGNABLE = MIsMoveAssignable< Type >::VALUE;
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_TYPEINFO_HPP_ )
