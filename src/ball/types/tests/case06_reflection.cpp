#include "common.hpp"

namespace
{
	struct PlayerStats_t
	{
		BALL_REFLECT_BEGIN( PlayerStats_t );

		BALL_REFLECT_FIELD( int, m_nHealth );
		BALL_REFLECT_FIELD( float, m_fSpeed );
		BALL_REFLECT_FIELD( double, m_fScore );
		BALL_REFLECT_FIELD( char, m_cRank );

		BALL_REFLECT_END( PlayerStats_t );
	};

	struct PackedWithPadding_t
	{
		BALL_REFLECT_BEGIN( PackedWithPadding_t );

		BALL_REFLECT_FIELD( char, m_cSmallA );
		BALL_REFLECT_FIELD( char, m_cSmallB );
		BALL_REFLECT_FIELD( double, m_fWide );
		BALL_REFLECT_FIELD( int, m_nValue );
		BALL_REFLECT_FIELD( char, m_cTail );

		BALL_REFLECT_END( PackedWithPadding_t );
	};

	struct EntityBase_t
	{
		BALL_REFLECT_BEGIN( EntityBase_t );

		BALL_REFLECT_FIELD( int, m_nId );

		BALL_REFLECT_END( EntityBase_t );
	};

	struct SpawnedEntity_t : EntityBase_t
	{
		BALL_REFLECT_BEGIN( SpawnedEntity_t );

		BALL_REFLECT_BASE( EntityBase_t, m_nId );
		BALL_REFLECT_FIELD( double, m_fSpawnTime );

		BALL_REFLECT_END( SpawnedEntity_t );
	};

	struct SingleValue_t
	{
		BALL_REFLECT_BEGIN( SingleValue_t );

		BALL_REFLECT_FIELD( long, m_nValue );

		BALL_REFLECT_END( SingleValue_t );
	};

	struct EmptyReflection_t
	{
		BALL_REFLECT_BEGIN( EmptyReflection_t );
		BALL_REFLECT_END( EmptyReflection_t );
	};

	struct RegularStruct_t
	{
		int m_nValue;
	};

	struct ManualFieldOwner_t
	{
		int m_nValue;
	};

	struct ManualFieldAccessor_t
	{
		using Owner_t = ManualFieldOwner_t;
		using Value_t = int;

		static constexpr Value_t &Get( Owner_t &owner ) noexcept { return owner.m_nValue; }
		static constexpr const Value_t &Get( const Owner_t &owner ) noexcept { return owner.m_nValue; }

		template < typename T >
		static constexpr void Set( Owner_t &owner, T &&value ) noexcept
		{
			owner.m_nValue = static_cast< T && >( value );
		}
	};

	static constexpr BTL::CStringView< BTL::size32_t, const char > MANUAL_FIELD_NAME{ "m_nValue" };
	using ManualFieldDesc_t = BTL::MFieldStaticDesc< BTL::size32_t, int, ManualFieldOwner_t, ManualFieldOwner_t, MANUAL_FIELD_NAME, 0u, BTL::MConstFieldOffset< BTL::size32_t, offsetof( ManualFieldOwner_t, m_nValue ) >, sizeof( int ) >;
	using ManualField_t = BTL::MField< ManualFieldDesc_t, ManualFieldAccessor_t >;
	using BoundReflect_t = BTL::CReflect< int, ManualField_t >;

	struct ReflectedField_t
	{
		BTL::size32_t m_nOffset = 0u;
		BTL::size32_t m_nSize = 0u;
		BTL::size32_t m_nPaddingBefore = 0u;
		BTL::size32_t m_nPaddingAfter = 0u;
	};

	struct FieldCollector_t
	{
		template < typename TField >
		constexpr bool Apply( BTL::size32_t nPaddingBefore, BTL::size32_t nPaddingAfter )
		{
			if ( m_nCount == m_nCapacity )
				return false;

			m_pFields[ m_nCount++ ] = { TField::Offset(), TField::FIELD_SIZE, nPaddingBefore, nPaddingAfter };

			return true;
		}

		ReflectedField_t *m_pFields = nullptr;
		BTL::size32_t m_nCapacity = 0u;
		BTL::size32_t m_nCount = 0u;
	};

	static void LogReflectCheck( TestsOutput_t &sOut, const char *pszLabel, bool bOk )
	{
		sOut.AppendMultiple( "BTL::Reflect_t: ", BTL::StringView_t( pszLabel ), ": " );

		if ( bOk )
			sOut += "ok\n";
		else
			sOut += "mismatch\n";
	}

	template < typename T >
	static FieldCollector_t CollectFields( ReflectedField_t *pFields, BTL::size32_t nCapacity )
	{
		FieldCollector_t collector{ pFields, nCapacity, 0u };

		BTL::Reflect_ForEach< T >( collector );

		return collector;
	}

	static bool CheckPlayerStatsMetadata()
	{
		using Desc_t = BTL::Reflect_t< PlayerStats_t >;
		using HealthField_t = typename Desc_t::Fields_t::Head_t;

		ReflectedField_t arrFields[ 4 ];
		const FieldCollector_t fields = CollectFields< PlayerStats_t >( arrFields, 4u );

		bool bOk = BTL::IS_REFLECTABLE< PlayerStats_t >;

		bOk &= Desc_t::SIZE == sizeof( PlayerStats_t );
		bOk &= Desc_t::ALIGN == alignof( PlayerStats_t );
		bOk &= Desc_t::FIELD_COUNT == 4u;
		bOk &= fields.m_nCount == 4u;

		bOk &= arrFields[ 0 ].m_nOffset == offsetof( PlayerStats_t, m_nHealth );
		bOk &= arrFields[ 1 ].m_nOffset == offsetof( PlayerStats_t, m_fSpeed );
		bOk &= arrFields[ 2 ].m_nOffset == offsetof( PlayerStats_t, m_fScore );
		bOk &= arrFields[ 3 ].m_nOffset == offsetof( PlayerStats_t, m_cRank );

		bOk &= arrFields[ 0 ].m_nSize == sizeof( int );
		bOk &= arrFields[ 1 ].m_nSize == sizeof( float );
		bOk &= arrFields[ 2 ].m_nSize == sizeof( double );
		bOk &= arrFields[ 3 ].m_nSize == sizeof( char );

		bOk &= BTL::IS_SAME< typename Desc_t::Owner_t, PlayerStats_t >;
		bOk &= BTL::IS_SAME< typename HealthField_t::DeclaredIn_t, PlayerStats_t >;
		bOk &= BTL::IS_SAME< typename HealthField_t::Accessor_t::Owner_t, PlayerStats_t >;

		return bOk;
	}

	static bool CheckFieldAccess()
	{
		using HealthField_t = typename BTL::Reflect_t< PlayerStats_t >::Fields_t::Head_t;
		using HealthValue_t = decltype( static_cast< PlayerStats_t * >( nullptr )->m_nHealth );

		PlayerStats_t stats{};

		HealthField_t::Set( stats, 90 );

		const int nFirstRead = HealthField_t::Get( stats );

		HealthField_t{}[ stats ] = 75;

		const int nSecondRead = HealthField_t{}( stats );

		bool bOk = nFirstRead == 90;

		bOk &= nSecondRead == 75;
		bOk &= stats.m_nHealth == 75;
		bOk &= BTL::IS_SAME< typename HealthValue_t::Field_t, PlayerStats_t::m_nHealth_Spec_t >;
		bOk &= stats.m_nHealth.Offset() == HealthField_t::Offset();
		bOk &= stats.m_nHealth.Name().Equals( BTL::StringView32_t( "m_nHealth" ) );

		return bOk;
	}

	static bool RunFieldMetadataMethodTests( TestsOutput_t &sOut )
	{
		using HealthField_t = typename BTL::Reflect_t< PlayerStats_t >::Fields_t::Head_t;
		using SpeedField_t = typename BTL::Reflect_t< PlayerStats_t >::Fields_t::Tail_t::Head_t;

		constexpr typename HealthField_t::FieldDesc_t health = HealthField_t::GetDesc();
		constexpr typename SpeedField_t::FieldDesc_t speed = SpeedField_t::GetDesc();

		bool bAllOk = true;

		{
			bool bOk = health.Index() == 0u;

			bOk &= health.Offset() == offsetof( PlayerStats_t, m_nHealth );
			bOk &= health.Size() == sizeof( int );
			bOk &= health.SerializedSize() == sizeof( int );
			bOk &= health.Align() == alignof( int );
			bOk &= health.ValueAlign() == alignof( int );
			bOk &= health.Name().Equals( BTL::StringView32_t( "m_nHealth" ) );
			bOk &= health.Name().Length() == 9u;
			bOk &= health.Name().String()[ 3u ] == 'H';
			bOk &= BTL::IS_SAME< decltype( HealthField_t::GetDesc() ), typename HealthField_t::FieldDesc_t >;
			bOk &= BTL::IS_SAME< typename HealthField_t::Name_t, BTL::StringView32_t >;
			bOk &= BTL::IS_SAME< decltype( health.Name() ), typename HealthField_t::NameView_t >;
			bOk &= BTL::IS_SAME< decltype( health.Index() ), BTL::size32_t >;
			bOk &= BTL::IS_SAME< decltype( health.Offset() ), BTL::size32_t >;
			bOk &= BTL::IS_SAME< decltype( health.SerializedSize() ), BTL::size32_t >;
			bOk &= BTL::IS_SAME< decltype( health.Name().Length() ), BTL::size32_t >;

			LogReflectCheck( sOut, "field metadata GetDesc", bOk );

			bAllOk &= bOk;
		}

		{
			bool bOk = health.Index() == 0u;

			bOk &= speed.Index() == 1u;

			LogReflectCheck( sOut, "field metadata Index", bOk );

			bAllOk &= bOk;
		}

		{
			bool bOk = health.Offset() == offsetof( PlayerStats_t, m_nHealth );

			bOk &= speed.Offset() == offsetof( PlayerStats_t, m_fSpeed );

			LogReflectCheck( sOut, "field metadata Offset", bOk );

			bAllOk &= bOk;
		}

		{
			bool bOk = health.Size() == sizeof( int );

			bOk &= speed.Size() == sizeof( float );

			LogReflectCheck( sOut, "field metadata Size", bOk );

			bAllOk &= bOk;
		}

		{
			bool bOk = health.SerializedSize() == sizeof( int );

			bOk &= speed.SerializedSize() == sizeof( float );

			LogReflectCheck( sOut, "field metadata SerializedSize", bOk );

			bAllOk &= bOk;
		}

		{
			bool bOk = health.Align() == alignof( int );

			bOk &= speed.Align() == alignof( float );

			LogReflectCheck( sOut, "field metadata Align", bOk );

			bAllOk &= bOk;
		}

		{
			bool bOk = health.ValueAlign() == alignof( int );

			bOk &= speed.ValueAlign() == alignof( float );

			LogReflectCheck( sOut, "field metadata ValueAlign", bOk );

			bAllOk &= bOk;
		}

		{
			bool bOk = health.Name().Equals( BTL::StringView32_t( "m_nHealth" ) );

			bOk &= speed.Name().Equals( BTL::StringView32_t( "m_fSpeed" ) );

			LogReflectCheck( sOut, "field metadata Name", bOk );

			bAllOk &= bOk;
		}

		{
			bool bOk = health.Name().Length() == 9u;

			bOk &= speed.Name().Length() == 8u;
			bOk &= health.Name().String()[ 0u ] == 'm';
			bOk &= health.Name().String()[ 3u ] == 'H';
			bOk &= speed.Name().String()[ 3u ] == 'S';
			bOk &= health.Name().Equals( BTL::StringView32_t( "m_nHealth" ) );
			bOk &= speed.Name().Equals( BTL::StringView32_t( "m_fSpeed" ) );
			bOk &= !health.Name().Equals( BTL::StringView32_t( "m_fSpeed" ) );

			LogReflectCheck( sOut, "field metadata Name object", bOk );
			bAllOk &= bOk;
		}

		return bAllOk;
	}

	static bool CheckPaddingLayout()
	{
		using Desc_t = BTL::Reflect_t< PackedWithPadding_t >;

		ReflectedField_t arrFields[ 5 ];
		const FieldCollector_t fields = CollectFields< PackedWithPadding_t >( arrFields, 5u );

		const BTL::size32_t nExpectedPayloadSize = sizeof( char ) + sizeof( char ) + sizeof( double ) + sizeof( int ) + sizeof( char );
		const BTL::size32_t nExpectedPadding = BTL::size32_t( sizeof( PackedWithPadding_t ) ) - nExpectedPayloadSize;
		BTL::size32_t nActualPadding = 0u;

		for ( BTL::size32_t i = 0u; i < fields.m_nCount; ++i )
			nActualPadding += arrFields[ i ].m_nPaddingBefore + arrFields[ i ].m_nPaddingAfter;

		bool bOk = Desc_t::FIELD_COUNT == 5u;

		bOk &= fields.m_nCount == 5u;
		bOk &= arrFields[ 0 ].m_nOffset == offsetof( PackedWithPadding_t, m_cSmallA );
		bOk &= arrFields[ 1 ].m_nOffset == offsetof( PackedWithPadding_t, m_cSmallB );
		bOk &= arrFields[ 2 ].m_nOffset == offsetof( PackedWithPadding_t, m_fWide );
		bOk &= arrFields[ 3 ].m_nOffset == offsetof( PackedWithPadding_t, m_nValue );
		bOk &= arrFields[ 4 ].m_nOffset == offsetof( PackedWithPadding_t, m_cTail );

		bOk &= nActualPadding == nExpectedPadding;

		return bOk;
	}

	static bool CheckBaseField()
	{
		using Desc_t = BTL::Reflect_t< SpawnedEntity_t >;
		using IdField_t = typename Desc_t::Fields_t::Head_t;

		ReflectedField_t arrFields[ 2 ];
		const FieldCollector_t fields = CollectFields< SpawnedEntity_t >( arrFields, 2u );

		SpawnedEntity_t entity{};

		IdField_t::Set( entity, 1001 );
		entity.m_fSpawnTime = 42.5;

		bool bOk = Desc_t::FIELD_COUNT == 2u;

		bOk &= fields.m_nCount == 2u;
		bOk &= BTL::IS_SAME< typename IdField_t::DeclaredIn_t, EntityBase_t >;
		// SpawnedEntity_t inherits EntityBase_t and so is non-standard-layout, where
		// offsetof is ill-formed; measure the reference offsets at run time instead.
		bOk &= arrFields[ 0 ].m_nOffset == BTL::ReflectMemberOffset< SpawnedEntity_t >( &SpawnedEntity_t::m_nId );
		bOk &= arrFields[ 1 ].m_nOffset == BTL::ReflectMemberOffset< SpawnedEntity_t >( &SpawnedEntity_t::m_fSpawnTime );
		bOk &= arrFields[ 1 ].m_nSize == sizeof( double );
		bOk &= IdField_t::Get( entity ) == 1001;
		bOk &= entity.m_fSpawnTime == 42.5;

		return bOk;
	}

	static bool CheckSerialization()
	{
		using Desc_t = BTL::Reflect_t< PlayerStats_t >;
		using Storage_t = BTL::Vector_t< unsigned char >;

		PlayerStats_t stats{};

		stats.m_nHealth = 100;
		stats.m_fSpeed = 12.5f;
		stats.m_fScore = 9000.25;
		stats.m_cRank = 'S';

		Storage_t bytes;

		const bool bSerialized = Desc_t::Serialize( stats, bytes );

		PlayerStats_t restored{};

		const bool bDeserialized = Desc_t::Deserialize( restored, bytes );

		bool bOk = bSerialized;

		bOk &= bDeserialized;
		bOk &= Desc_t::SERIALIZED_SIZE == sizeof( int ) + sizeof( float ) + sizeof( double ) + sizeof( char );
		bOk &= bytes.Count() == Desc_t::SERIALIZED_SIZE;
		bOk &= restored.m_nHealth == stats.m_nHealth;
		bOk &= restored.m_fSpeed == stats.m_fSpeed;
		bOk &= restored.m_fScore == stats.m_fScore;
		bOk &= restored.m_cRank == stats.m_cRank;

		Storage_t runtimeBytes;
		const bool bRuntimeSerialized = stats.Serialize( runtimeBytes );

		PlayerStats_t runtimeRestored{};
		const bool bRuntimeDeserialized = runtimeRestored.Deserialize( runtimeBytes );

		bOk &= bRuntimeSerialized;
		bOk &= bRuntimeDeserialized;
		bOk &= runtimeRestored.m_nHealth == stats.m_nHealth;
		bOk &= runtimeRestored.m_fSpeed == stats.m_fSpeed;
		bOk &= runtimeRestored.m_fScore == stats.m_fScore;
		bOk &= runtimeRestored.m_cRank == stats.m_cRank;

		Storage_t valueBytes;
		const bool bValueSerialized = stats.m_nHealth.Serialize( valueBytes );

		BTL::CReflect< int > restoredHealth;
		const bool bValueDeserialized = restoredHealth.Deserialize( valueBytes );

		bOk &= bValueSerialized;
		bOk &= bValueDeserialized;
		bOk &= restoredHealth.Get() == stats.m_nHealth;

		using HealthField_t = typename Desc_t::Fields_t::Head_t;

		Storage_t fieldBytes;
		constexpr HealthField_t healthField{};
		const bool bFieldSerialized = healthField.Serialize( stats, fieldBytes );

		PlayerStats_t fieldRestored{};

		Desc_t::Index_t cursor = 0u;
		const bool bFieldDeserialized = healthField.Deserialize( fieldRestored, fieldBytes, cursor );

		bOk &= bFieldSerialized;
		bOk &= bFieldDeserialized;
		bOk &= cursor == HealthField_t::SERIALIZED_SIZE;
		bOk &= fieldRestored.m_nHealth == stats.m_nHealth;

		Storage_t appended;

		appended.AddToTail( static_cast< unsigned char >( 0xCAu ) );

		const bool bAppended = Desc_t::Serialize( stats, appended );

		bOk &= bAppended;
		bOk &= appended.Count() == Desc_t::SERIALIZED_SIZE + 1u;
		bOk &= appended.Base()[ 0 ] == 0xCAu;

		Storage_t truncated;

		Desc_t::Serialize( stats, truncated );
		truncated.Remove( truncated.Count() - 1u );

		PlayerStats_t failed{};

		bOk &= !Desc_t::Deserialize( failed, truncated );

		return bOk;
	}

	static bool CheckReflectValueFieldInheritance()
	{
		using Storage_t = BTL::Vector_t< unsigned char >;

		BoundReflect_t value;

		value = 64;

		Storage_t bytes;
		const bool bSerialized = value.Serialize( bytes );

		BoundReflect_t restored;
		const bool bDeserialized = restored.Deserialize( bytes );

		bool bOk = __is_base_of( ManualField_t, BoundReflect_t );

		bOk &= BTL::IS_SAME< typename BoundReflect_t::Field_t, ManualField_t >;
		bOk &= BTL::IS_SAME< BTL::ReflectValue_t< BoundReflect_t >, int >;
		bOk &= value.Index() == ManualField_t::INDEX;
		bOk &= value.Offset() == offsetof( ManualFieldOwner_t, m_nValue );
		bOk &= value.Size() == sizeof( int );
		bOk &= value.Name().Equals( BTL::StringView32_t( "m_nValue" ) );
		bOk &= bSerialized;
		bOk &= bDeserialized;
		bOk &= restored.Get() == value.Get();

		return bOk;
	}

	static bool CheckEdgeCases()
	{
		using SingleDesc_t = BTL::Reflect_t< SingleValue_t >;
		using EmptyDesc_t = BTL::Reflect_t< EmptyReflection_t >;

		ReflectedField_t arrSingle[ 1 ];
		const FieldCollector_t single = CollectFields< SingleValue_t >( arrSingle, 1u );

		ReflectedField_t arrEmpty[ 1 ];
		const FieldCollector_t empty = CollectFields< EmptyReflection_t >( arrEmpty, 1u );

		bool bOk = SingleDesc_t::FIELD_COUNT == 1u;

		bOk &= single.m_nCount == 1u;
		bOk &= arrSingle[ 0 ].m_nOffset == offsetof( SingleValue_t, m_nValue );
		bOk &= arrSingle[ 0 ].m_nPaddingAfter == sizeof( SingleValue_t ) - sizeof( long );

		bOk &= EmptyDesc_t::FIELD_COUNT == 0u;
		bOk &= empty.m_nCount == 0u;
		bOk &= !BTL::IS_REFLECTABLE< RegularStruct_t >;

		return bOk;
	}
}

void Case06_Reflection( TestsOutput_t &sOut )
{
	bool bAllOk = true;

	{
		const bool bOk = CheckPlayerStatsMetadata();

		LogReflectCheck( sOut, "metadata and field order", bOk );

		bAllOk &= bOk;
	}

	{
		const bool bOk = CheckFieldAccess();

		LogReflectCheck( sOut, "field get / set", bOk );

		bAllOk &= bOk;
	}

	{
		const bool bOk = RunFieldMetadataMethodTests( sOut );

		bAllOk &= bOk;
	}

	{
		const bool bOk = CheckPaddingLayout();

		LogReflectCheck( sOut, "padding layout", bOk );

		bAllOk &= bOk;
	}

	{
		const bool bOk = CheckBaseField();

		LogReflectCheck( sOut, "base field", bOk );

		bAllOk &= bOk;
	}

	{
		const bool bOk = CheckSerialization();

		LogReflectCheck( sOut, "serialization round-trip", bOk );

		bAllOk &= bOk;
	}

	{
		const bool bOk = CheckReflectValueFieldInheritance();

		LogReflectCheck( sOut, "reflect value field inheritance", bOk );

		bAllOk &= bOk;
	}

	{
		const bool bOk = CheckEdgeCases();

		LogReflectCheck( sOut, "single / empty / regular structs", bOk );

		bAllOk &= bOk;
	}

	sOut.AppendMultiple( "BTL::Reflect_t: " );

	if ( bAllOk )
		sOut += "ok\n";
	else
		sOut += "mismatch\n";

	sOut += "---\n";
}
