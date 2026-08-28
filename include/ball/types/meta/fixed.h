#ifndef _INCLUDE_BALL_TYPES_META_FIXED_H_
#	define _INCLUDE_BALL_TYPES_META_FIXED_H_

#	define BALL_FIXED_DECLARE_SIGNED_ENUM_TRAIT( enumName, bits ) \
	template <> struct MFixedPacked_Base< enumName > : public MFixedPacked_EnumBase< enumName, FixedSignedStorage_t< bits >, bits_t( bits ), true, false > {};

#	define BALL_FIXED_DECLARE_UNCERTAIN_ENUM_TRAIT( enumName, bits ) \
	template <> struct MFixedPacked_Base< enumName > : public MFixedPacked_EnumBase< enumName, FixedSignedStorage_t< bits >, bits_t( bits ), true, false > {};

#	define BALL_FIXED_DECLARE_UNSIGNED_ENUM_TRAIT( enumName, bits ) \
	template <> struct MFixedPacked_Base< enumName > : public MFixedPacked_EnumBase< enumName, FixedUnsignedStorage_t< bits >, bits_t( bits ), false, true > {};

#	define BALL_FIXED_SIGNED_ENUM_TRAIT( enumName, bits ) BALL_FIXED_DECLARE_SIGNED_ENUM_TRAIT( enumName, bits )
#	define BALL_FIXED_UNCERTAIN_ENUM_TRAIT( enumName, bits ) BALL_FIXED_DECLARE_UNCERTAIN_ENUM_TRAIT( enumName, bits )
#	define BALL_FIXED_UNSIGNED_ENUM_TRAIT( enumName, bits ) BALL_FIXED_DECLARE_UNSIGNED_ENUM_TRAIT( enumName, bits )

#	define BALL_FIXED_DECLARE_ENUM_BASE( M, enumName, typeDef, bits ) enum enumName : typeDef; M( enumName, bits ) enum enumName : typeDef
#	define BALL_FIXED_DECLARE_ENUM_CLASS_BASE( M, enumName, typeDef, bits ) enum class enumName : typeDef; M( enumName, bits ) enum class enumName : typeDef
#	define BALL_FIXED_SIGNED_ENUM( enumName, bits ) BALL_FIXED_DECLARE_ENUM_BASE( BALL_FIXED_DECLARE_SIGNED_ENUM_TRAIT, enumName, FixedSignedStorage_t< bits >, bits )
#	define BALL_FIXED_UNCERTAIN_ENUM( enumName, bits ) BALL_FIXED_DECLARE_ENUM_BASE( BALL_FIXED_DECLARE_UNCERTAIN_ENUM_TRAIT, enumName, FixedSignedStorage_t< bits >, bits )
#	define BALL_FIXED_UNSIGNED_ENUM( enumName, bits ) BALL_FIXED_DECLARE_ENUM_BASE( BALL_FIXED_DECLARE_UNSIGNED_ENUM_TRAIT, enumName, FixedUnsignedStorage_t< bits >, bits )
#	define BALL_FIXED_SIGNED_ENUM_CLASS( enumName, bits ) BALL_FIXED_DECLARE_ENUM_CLASS_BASE( BALL_FIXED_DECLARE_SIGNED_ENUM_TRAIT, enumName, FixedSignedStorage_t< bits >, bits )
#	define BALL_FIXED_UNCERTAIN_ENUM_CLASS( enumName, bits ) BALL_FIXED_DECLARE_ENUM_CLASS_BASE( BALL_FIXED_DECLARE_UNCERTAIN_ENUM_TRAIT, enumName, FixedSignedStorage_t< bits >, bits )
#	define BALL_FIXED_UNSIGNED_ENUM_CLASS( enumName, bits ) BALL_FIXED_DECLARE_ENUM_CLASS_BASE( BALL_FIXED_DECLARE_UNSIGNED_ENUM_TRAIT, enumName, FixedUnsignedStorage_t< bits >, bits )

#endif // !defined( _INCLUDE_BALL_TYPES_META_FIXED_H_ )
