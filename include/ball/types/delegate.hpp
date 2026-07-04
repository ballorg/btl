#ifndef _INCLUDE_BALL_TYPES_DELEGATES_HPP_
#	define _INCLUDE_BALL_TYPES_DELEGATES_HPP_

#	pragma once

#	include "c/assert.h"
#	include "c/nodiscrad.h"
#	include "meta/decay.hpp"
#	include "meta/enableif.hpp"
#	include "meta/get.hpp"
#	include "meta/isconst.hpp"
#	include "meta/ispointer.hpp"
#	include "meta/issame.hpp"
#	include "meta/memberfunction.hpp"
#	include "meta/pack.hpp"
#	include "meta/removecv.hpp"
#	include "meta/removereference.hpp"
#	include "meta/return.hpp"
#	include "meta/sequence.hpp"
#	include "meta/variant.hpp"
#	include "meta/xvalue.hpp"
#	include "multivector.hpp"
#	include "vector.hpp"

#	ifndef DELEGATE_INLINE_ALLOCATION_SIZE
#		define DELEGATE_INLINE_ALLOCATION_SIZE 64
#	endif

#	define BALL_DECLARE_DELEGATE( name, ... ) \
	using name = CDelegate< void, __VA_ARGS__ >

#	define BALL_DECLARE_DELEGATE_RET( name, R, ... ) \
	using name = CDelegate< R, __VA_ARGS__ >

#	define BALL_DECLARE_MULTICAST_DELEGATE( name, ... ) \
	using name = CMulticastDelegate< __VA_ARGS__ >; \
	using name##Delegate = typename name::Delegate_t

#	define BALL_DECLARE_MULTICAST_DELEGATE_EVENT( name, ownerType, ... ) \
	class name : public CMulticastDelegate< __VA_ARGS__ > \
	{ \
	private: \
		using Base_t = CMulticastDelegate< __VA_ARGS__ >; \
		friend class ownerType; \
		using Base_t::Broadcast; \
		using Base_t::RemoveAll; \
		using Base_t::Remove; \
	}

#	define BALL_DECLARE_MULICAST_DELEGATE_EVENT( name, ownerType, ... ) \
	BALL_DECLARE_MULTICAST_DELEGATE_EVENT( name, ownerType, __VA_ARGS__ )

/// @brief Non-templated base interface for type-erased delegate implementations.
///        This layer exposes lifecycle operations required by CDelegateBase:
///        owner lookup, copy construction into inline storage, move
///        construction, and explicit destruction.
class IDelegateBase
{
public:
	virtual constexpr const void *GetOwner() const noexcept { return nullptr; }
	virtual void CopyConstruct( void *pDestination ) const = 0;
	virtual void MoveConstruct( void *pDestination ) = 0;
	virtual void Destroy() noexcept = 0;

	virtual ~IDelegateBase() noexcept = default;
};

/// @brief Typed execution interface for a bound delegate implementation.
///        IDelegate extends IDelegateBase with the call contract used by
///        CDelegate<R, Ts...>::Execute().
template < typename R, typename ...Ts >
class IDelegate : public IDelegateBase
{
public:
	virtual R Execute( Ts... args ) const = 0;
};

template < typename R, typename ...Ts >
class CStaticDelegate;

/// @brief Delegate implementation that binds a free or static function plus
///        an optional payload pack appended at invocation time.
template < typename R, typename ...Ts, typename ...TPayload >
class CStaticDelegate< R ( Ts... ), TPayload... > : public IDelegate< R, Ts... >
{
public:
	using Function_t = R ( * )( Ts..., TPayload... );
	using Payload_t = MPack< size_t, TPayload... >;

	constexpr CStaticDelegate( const Function_t &pFunction, TPayload &&...payload ) noexcept : m_pFunction( pFunction ), m_Payload( Forward< TPayload >( payload )... ) {}

	CStaticDelegate( const CStaticDelegate & ) noexcept = default;
	CStaticDelegate( CStaticDelegate && ) noexcept = default;
	CStaticDelegate &operator=( const CStaticDelegate & ) noexcept = default;
	CStaticDelegate &operator=( CStaticDelegate && ) noexcept = default;

	R Execute( Ts... args ) const override { return ExecuteBy( Sequence_t< TPayload... >(), args... ); }
	void CopyConstruct( void *pDestination ) const override { new ( pDestination ) CStaticDelegate( *this ); }
	void MoveConstruct( void *pDestination ) override { new ( pDestination ) CStaticDelegate( Move( *this ) ); }
	void Destroy() noexcept override { this->~CStaticDelegate(); }

protected:
	template < size_t ...Is >
	R ExecuteBy( MSequence< Is... >, Ts... args ) const
	{
		return m_pFunction( args..., Get< Is >( m_Payload )... );
	}

private:
	Function_t m_pFunction;
	Payload_t m_Payload;
};

template < bool IsConst, typename T, typename TSignature, typename ...TPayload >
class CObjectDelegate;

/// @brief Delegate implementation that binds an object pointer and a member
///        function pointer, optionally followed by stored payload values.
template < bool IsConst, typename T, typename R, typename ...Ts, typename ...TPayload >
class CObjectDelegate< IsConst, T, R ( Ts... ), TPayload... > : public IDelegate< R, Ts... >
{
public:
	using Function_t = typename MMemberFunctionByConst< IsConst, T, R, Ts..., TPayload... >::Type;
	using Payload_t = MPack< size_t, TPayload... >;

	constexpr CObjectDelegate( T *pObject, const Function_t &pFunction, TPayload &&...payload ) noexcept : m_pObject( pObject ), m_pFunction( pFunction ), m_Payload( Forward< TPayload >( payload )... ) {}

	CObjectDelegate( const CObjectDelegate & ) noexcept = default;
	CObjectDelegate( CObjectDelegate && ) noexcept = default;
	CObjectDelegate &operator=( const CObjectDelegate & ) noexcept = default;
	CObjectDelegate &operator=( CObjectDelegate && ) noexcept = default;

	R Execute( Ts... args ) const override
	{
		if ( m_pObject == nullptr )
		{
			return MReturn< R >::Default();
		}

		return ExecuteBy( Sequence_t< TPayload... >(), args... );
	}

	constexpr const void *GetOwner() const noexcept override { return m_pObject; }
	void CopyConstruct( void *pDestination ) const override { new ( pDestination ) CObjectDelegate( *this ); }
	void MoveConstruct( void *pDestination ) override { new ( pDestination ) CObjectDelegate( Move( *this ) ); }
	void Destroy() noexcept override { this->~CObjectDelegate(); }

protected:
	template < size_t ...Is >
	R ExecuteBy( MSequence< Is... >, Ts... args ) const
	{
		return ( m_pObject->*m_pFunction )( args..., Get< Is >( m_Payload )... );
	}

private:
	T *m_pObject;
	Function_t m_pFunction;
	Payload_t m_Payload;
};

template < bool IsConst, typename T, typename R, typename ...Ts >
using CMemberDelegate = CObjectDelegate< IsConst, T, R, Ts... >;

template < typename LT, typename R, typename ...Ts >
class CLambdaDelegate;

/// @brief Delegate implementation that stores an arbitrary callable object and
///        optional payload values appended during execution.
template < typename LT, typename R, typename ...Ts, typename ...TPayload >
class CLambdaDelegate< LT, R ( Ts... ), TPayload... > : public IDelegate< R, Ts... >
{
public:
	using Lambda_t = LT;
	using Payload_t = MPack< size_t, TPayload... >;

	constexpr CLambdaDelegate( LT &&lambda, TPayload &&...payload ) noexcept : m_Lambda( Forward< LT >( lambda ) ), m_Payload( Forward< TPayload >( payload )... ) {}

	CLambdaDelegate( const CLambdaDelegate & ) noexcept = default;
	CLambdaDelegate( CLambdaDelegate && ) noexcept = default;
	CLambdaDelegate &operator=( const CLambdaDelegate & ) noexcept = default;
	CLambdaDelegate &operator=( CLambdaDelegate && ) noexcept = default;

	R Execute( Ts... args ) const override { return ExecuteBy( Sequence_t< TPayload... >(), args... ); }
	void CopyConstruct( void *pDestination ) const override { new ( pDestination ) CLambdaDelegate( *this ); }
	void MoveConstruct( void *pDestination ) override { new ( pDestination ) CLambdaDelegate( Move( *this ) ); }
	void Destroy() noexcept override { this->~CLambdaDelegate(); }

protected:
	template < size_t ...Is >
	R ExecuteBy( MSequence< Is... >, Ts... args ) const
	{
		return static_cast< R >( m_Lambda( args..., Get< Is >( m_Payload )... ) );
	}

private:
	mutable Lambda_t m_Lambda;
	Payload_t m_Payload;
};

/// @brief Stable opaque identifier used to address a slot inside a multicast delegate.
class DelegateHandle_t
{
public:
	static constexpr uint_t INVALID_ID = ~uint_t( 0 );

	constexpr DelegateHandle_t( uint_t id = INVALID_ID ) noexcept : m_Id( id ) {}
	explicit constexpr DelegateHandle_t( bool bGenerateId ) noexcept : m_Id( bGenerateId ? GetNewID() : INVALID_ID ) {}

	constexpr DelegateHandle_t( const DelegateHandle_t &other ) noexcept : m_Id( INVALID_ID ) { CopyFrom( other ); }
	constexpr DelegateHandle_t &operator=( const DelegateHandle_t &other ) noexcept { return CopyFrom( other ); }
	constexpr DelegateHandle_t( DelegateHandle_t &&other ) noexcept : m_Id( INVALID_ID ) { MoveFrom( Move( other ) ); }
	constexpr DelegateHandle_t &operator=( DelegateHandle_t &&other ) noexcept { return MoveFrom( Move( other ) ); }

	constexpr operator bool() const noexcept { return IsValid(); }
	constexpr bool operator==( const DelegateHandle_t &other ) const noexcept { return m_Id == other.m_Id; }
	constexpr bool operator!=( const DelegateHandle_t &other ) const noexcept { return !( *this == other ); }
	constexpr bool operator<( const DelegateHandle_t &other ) const noexcept { return m_Id < other.m_Id; }

	constexpr bool IsValid() const noexcept { return m_Id != INVALID_ID; }
	constexpr void Reset() noexcept { m_Id = INVALID_ID; }

private:
	constexpr DelegateHandle_t &CopyFrom( const DelegateHandle_t &other ) noexcept { m_Id = other.m_Id; return *this; }
	constexpr DelegateHandle_t &MoveFrom( DelegateHandle_t &&other ) noexcept
	{
		if ( this != &other )
		{
			m_Id = other.m_Id;
			other.Reset();
		}

		return *this;
	}

	static uint_t GetNewID() noexcept
	{
		uint_t nOutput = CURRENT_ID++;

		if ( CURRENT_ID == INVALID_ID )
			CURRENT_ID = 0;

		return nOutput;
	}

	uint_t m_Id;
	static inline uint_t CURRENT_ID = 0;
};

/// @brief Inline storage owner for a single bound delegate implementation.
///        CDelegateBase manages erased storage, copy/move of the stored
///        implementation object, and binding state queries.
class CDelegateBase
{
protected:
	static constexpr size_t STORAGE_BLOCK_SIZE = sizeof( Variant_t );
	static constexpr size_t INLINE_BLOCK_COUNT = ( DELEGATE_INLINE_ALLOCATION_SIZE + STORAGE_BLOCK_SIZE - 1 ) / STORAGE_BLOCK_SIZE;

	BALL_STATIC_ASSERT( DELEGATE_INLINE_ALLOCATION_SIZE >= STORAGE_BLOCK_SIZE, "DELEGATE_INLINE_ALLOCATION_SIZE is smaller than the delegate storage alignment block" );

public:
	constexpr CDelegateBase() noexcept : m_Storage() {}
	~CDelegateBase() noexcept { Release(); }
	CDelegateBase( const CDelegateBase &other ) : m_Storage() { CopyFrom( other ); }
	CDelegateBase( CDelegateBase &&other ) noexcept : m_Storage() { MoveFrom( Move( other ) ); }
	CDelegateBase &operator=( const CDelegateBase &other ) { return CopyFrom( other ); }
	CDelegateBase &operator=( CDelegateBase &&other ) noexcept { return MoveFrom( Move( other ) ); }

	const void *GetOwner() const noexcept { return HasAllocation() ? GetDelegate()->GetOwner() : nullptr; }
	size_t GetSize() const noexcept { return m_Storage.Size(); }
	void ClearIfBoundTo( void *pObject ) noexcept { if ( pObject != nullptr && IsBoundTo( pObject ) ) Release(); }
	void Clear() noexcept { Release(); }
	bool IsBound() const noexcept { return HasAllocation(); }
	bool IsBoundTo( void *pObject ) const noexcept { return pObject != nullptr && HasAllocation() && GetDelegate()->GetOwner() == pObject; }

protected:
	CDelegateBase &CopyFrom( const CDelegateBase &other )
	{
		if ( this != &other )
		{
			Release();

			if ( other.HasAllocation() )
				other.GetDelegate()->CopyConstruct( Allocate( other.GetSize() ) );
		}

		return *this;
	}

	CDelegateBase &MoveFrom( CDelegateBase &&other ) noexcept
	{
		if ( this != &other )
		{
			Release();

			if ( other.HasAllocation() )
			{
				other.GetMutableDelegate()->MoveConstruct( Allocate( other.GetSize() ) );
				other.Release();
			}
		}

		return *this;
	}

	static constexpr size_t BlockCountForSize( size_t nSize ) noexcept
	{
		return nSize == 0 ? 0 : ( nSize + STORAGE_BLOCK_SIZE - 1 ) / STORAGE_BLOCK_SIZE;
	}

	static constexpr size_t StorageSize( size_t nBlocks ) noexcept
	{
		return nBlocks * STORAGE_BLOCK_SIZE;
	}

	void *Allocate( size_t nSize )
	{
		const size_t nBlocks = BlockCountForSize( nSize );
		const size_t nStorageSize = StorageSize( nBlocks );

		if ( m_Storage.Size() != nStorageSize )
		{
			m_Storage.RemoveAll();

			if ( nBlocks > 0 )
				m_Storage.Grow( nBlocks );
		}

		return GetAllocation();
	}

	void FreeAllocation() noexcept
	{
		m_Storage.RemoveAll();
	}

	bool HasAllocation() const noexcept { return m_Storage.Size() > 0; }
	const void *GetAllocation() const noexcept { return HasAllocation() ? static_cast< const void * >( m_Storage.Base() ) : nullptr; }
	void *GetAllocation() noexcept { return const_cast< void * >( static_cast< const CDelegateBase * >( this )->GetAllocation() ); }
	const IDelegateBase *GetDelegate() const noexcept { return static_cast< const IDelegateBase * >( GetAllocation() ); }
	IDelegateBase *GetMutableDelegate() noexcept { return static_cast< IDelegateBase * >( GetAllocation() ); }

	void Release() noexcept
	{
		if ( HasAllocation() )
		{
			GetMutableDelegate()->Destroy();
			FreeAllocation();
		}
	}

	BufferVector_t< Variant_t, INLINE_BLOCK_COUNT - 1 > m_Storage; // Subtract vector size field from block count to get align.
};

/// @brief Single-cast delegate wrapper with type-erased inline storage.
///        The public API exposes factory and bind helpers for static
///        functions, member functions, and lambda objects.
template < typename R, typename ...Ts >
class CDelegate : public CDelegateBase
{
public:
	template < typename T, typename ...TPayload > using MemberFunction_t = typename MMemberFunction< T, R, Ts..., Decay_t< TPayload >... >::Type;
	template < typename T, typename ...TPayload > using ConstMemberFunction_t = typename MMemberFunction< T, R, Ts..., Decay_t< TPayload >... >::ConstType;

	using DelegateInterface_t = IDelegate< R, Ts... >;

	constexpr CDelegate() noexcept = default;
	CDelegate( const CDelegate & ) = default;
	CDelegate( CDelegate && ) noexcept = default;
	CDelegate &operator=( const CDelegate & ) = default;
	CDelegate &operator=( CDelegate && ) noexcept = default;
	~CDelegate() noexcept = default;

	template < typename T, typename TFunction, typename ...TPayload, EnableIf_t< IS_MEMBER_FUNCTION_POINTER< Decay_t< TFunction > >, int > = 0 >
	CDelegate( T *pObject, const TFunction &pFunction, TPayload &&...payload )
	{
		Bind< CObjectDelegate< IS_CONST_MEMBER_FUNCTION_POINTER< Decay_t< TFunction > >, T, R ( Ts... ), Decay_t< TPayload >... > >( pObject, pFunction, Forward< TPayload >( payload )... );
	}

	template < typename ...TPayload >
	CDelegate( R ( *pFunction )( Ts..., TPayload... ), TPayload &&...payload )
	{
		Bind< CStaticDelegate< R ( Ts... ), Decay_t< TPayload >... > >( pFunction, Forward< TPayload >( payload )... );
	}

	template < typename TCallable, typename ...TPayload, EnableIf_t< !IS_POINTER< Decay_t< TCallable > > && !IS_SAME< Decay_t< TCallable >, CDelegate >, int > = 0 >
	CDelegate( TCallable &&lambda, TPayload &&...payload )
	{
		using Lambda_t = Decay_t< TCallable >;

		Lambda_t storedLambda( Forward< TCallable >( lambda ) );

		Bind< CLambdaDelegate< Lambda_t, R ( Ts... ), Decay_t< TPayload >... > >( Move( storedLambda ), Forward< TPayload >( payload )... );
	}

	template < typename ...TPayload >
	void BindStatic( R ( *pFunction )( Ts..., Decay_t< TPayload >... ), TPayload &&...payload )
	{
		Bind< CStaticDelegate< R ( Ts... ), Decay_t< TPayload >... > >( pFunction, Forward< TPayload >( payload )... );
	}

	template < typename TCallable, typename ...TPayload >
	void BindLambda( TCallable &&lambda, TPayload &&...payload )
	{
		using Lambda_t = Decay_t< TCallable >;

		Lambda_t storedLambda( Forward< TCallable >( lambda ) );

		Bind< CLambdaDelegate< Lambda_t, R ( Ts... ), Decay_t< TPayload >... > >( Move( storedLambda ), Forward< TPayload >( payload )... );
	}

	template < typename T, typename ...TPayload >
	void BindMember( T *pObject, const MemberFunction_t< T, TPayload... > &pFunction, TPayload &&...payload )
	{
		BALL_STATIC_ASSERT( !IS_CONST< T >, "Cannot bind a non-const function on a const object" );

		Bind< CMemberDelegate< false, T, R ( Ts... ), Decay_t< TPayload >... > >( pObject, pFunction, Forward< TPayload >( payload )... );
	}

	template < typename T, typename ...TPayload >
	void BindMember( T *pObject, const ConstMemberFunction_t< T, TPayload... > &pFunction, TPayload &&...payload )
	{
		Bind< CMemberDelegate< true, T, R ( Ts... ), Decay_t< TPayload >... > >( pObject, pFunction, Forward< TPayload >( payload )... );
	}

	R Execute( Ts... args ) const
	{
		BALL_ASSERT_MESSAGE( IsBound(), "Delegate is not bound" );

		return static_cast< const DelegateInterface_t * >( GetDelegate() )->Execute( args... );
	}

	R ExecuteIfBound( Ts... args ) const
	{
		if ( IsBound() )
			return static_cast< const DelegateInterface_t * >( GetDelegate() )->Execute( args... );

		return MReturn< R >::Default();
	}

private:
	template < typename TDelegate, typename ...TArgs >
	void Bind( TArgs &&...args )
	{
		Release();

		new ( Allocate( sizeof( TDelegate ) ) ) TDelegate( Forward< TArgs >( args )... );
	}
};

/// @brief Multi-cast delegate that stores multiple CDelegate<void, Ts...>
///        instances and broadcasts a single argument list to every live entry.
template < typename ...Ts >
class CMulticastDelegate
{
public:
	using SingleDelegate_t = CDelegate< void, Ts... >;
	template < typename T, typename ...TPayload > using MemberFunction_t = typename SingleDelegate_t::template MemberFunction_t< T, Decay_t< TPayload >... >;
	template < typename T, typename ...TPayload > using ConstMemberFunction_t = typename SingleDelegate_t::template ConstMemberFunction_t< T, Decay_t< TPayload >... >;
	using Index_t = size32_t; // See m_Events field.

	constexpr CMulticastDelegate() noexcept : m_nLocks( 0 ) {}

	~CMulticastDelegate() noexcept = default;
	CMulticastDelegate( const CMulticastDelegate & ) = default;
	CMulticastDelegate &operator=( const CMulticastDelegate & ) = default;
	CMulticastDelegate( CMulticastDelegate && ) noexcept = default;
	CMulticastDelegate &operator=( CMulticastDelegate && ) noexcept = default;

	template < typename TCallable >
	DelegateHandle_t operator+=( TCallable &&callable )
	{
		return AddLambda( Forward< TCallable >( callable ) );
	}

	DelegateHandle_t operator+=( const SingleDelegate_t &delegate ) { return Add( delegate ); }
	DelegateHandle_t operator+=( SingleDelegate_t &&delegate ) noexcept { return Add( Move( delegate ) ); }
	bool operator-=( DelegateHandle_t &handle ) noexcept { return Remove( handle ); }

	DelegateHandle_t Add( const SingleDelegate_t &delegate )
	{
		for ( Index_t i = 0; i < m_Events.Count(); ++i )
		{
			if ( Handle( i ).IsValid() == false )
			{
				Handle( i ) = DelegateHandle_t( true );
				Callback( i ) = delegate;

				return Handle( i );
			}
		}

		m_Events.AddToTail( DelegateHandle_t( true ), delegate );

		return Handle( m_Events.Count() - 1 );
	}

	DelegateHandle_t Add( SingleDelegate_t &&delegate ) noexcept
	{
		for ( Index_t i = 0; i < m_Events.Count(); ++i )
		{
			if ( Handle( i ).IsValid() == false )
			{
				Handle( i ) = DelegateHandle_t( true );
				Callback( i ) = Move( delegate );

				return Handle( i );
			}
		}

		m_Events.AddToTail( DelegateHandle_t( true ), Move( delegate ) );

		return Handle( m_Events.Count() - 1 );
	}

	template < typename ...TPayload >
	DelegateHandle_t AddStatic( void ( *pFunction )( Ts..., Decay_t< TPayload >... ), TPayload &&...payload ) { return Add( SingleDelegate_t( pFunction, Forward< TPayload >( payload )... ) ); }

	template < typename TCallable, typename ...TPayload >
	DelegateHandle_t AddLambda( TCallable &&lambda, TPayload &&...payload )
	{
		return Add( SingleDelegate_t( Forward< TCallable >( lambda ), Forward< TPayload >( payload )... ) );
	}

	template < typename T, typename ...TPayload >
	DelegateHandle_t AddMember( T *pObject, const MemberFunction_t< T, TPayload... > &pFunction, TPayload &&...payload )
	{
		return Add( SingleDelegate_t( pObject, pFunction, Forward< TPayload >( payload )... ) );
	}

	template < typename T, typename ...TPayload >
	DelegateHandle_t AddMember( T *pObject, const ConstMemberFunction_t< T, TPayload... > &pFunction, TPayload &&...payload )
	{
		return Add( SingleDelegate_t( pObject, pFunction, Forward< TPayload >( payload )... ) );
	}

	void RemoveObject( void *pObject ) noexcept
	{
		if ( pObject == nullptr )
			return;

		Index_t i = 0;

		while ( i < m_Events.Count() )
		{
			if ( Callback( i ).GetOwner() != pObject )
			{
				++i;
				continue;
			}

			Handle( i ).Reset();
			Callback( i ).Clear();

			if ( IsLocked() )
			{
				++i;
				continue;
			}

			const Index_t nLast = m_Events.Count() - 1;

			if ( i != nLast )
			{
				Swap( Handle( i ), Handle( nLast ) );
				Swap( Callback( i ), Callback( nLast ) );
			}

			m_Events.Remove( nLast );
		}
	}

	bool Remove( DelegateHandle_t &handle ) noexcept
	{
		if ( !handle.IsValid() )
			return false;

		for ( Index_t i = 0; i < m_Events.Count(); ++i )
		{
			if ( Handle( i ) != handle )
				continue;

			Handle( i ).Reset();
			Callback( i ).Clear();

			if ( !IsLocked() )
			{
				const Index_t nLast = m_Events.Count() - 1;

				if ( i != nLast )
				{
					Swap( Handle( i ), Handle( nLast ) );
					Swap( Callback( i ), Callback( nLast ) );
				}

				m_Events.Remove( nLast );
			}

			handle.Reset();

			return true;
		}

		return false;
	}

	bool IsBoundTo( const DelegateHandle_t &handle ) const noexcept
	{
		if ( !handle.IsValid() )
			return false;

		return m_Events.Find( handle ) != m_Events.INVALID_INDEX;
	}

	void RemoveAll() noexcept
	{
		if ( IsLocked() )
		{
			for ( Index_t i = 0; i < m_Events.Count(); ++i )
			{
				Handle( i ).Reset();
				Callback( i ).Clear();
			}
		}
		else
		{
			m_Events.RemoveAll();
		}
	}

	void Compress( size_t nMaxSpace = 0 ) noexcept
	{
		if ( IsLocked() )
			return;

		Index_t iWrite = 0;

		for ( Index_t i = 0; i < m_Events.Count(); ++i )
		{
			if ( iWrite != i )
			{
				Swap( Handle( iWrite ), Handle( i ) );
				Swap( Callback( iWrite ), Callback( i ) );
			}

			++iWrite;
		}

		while ( m_Events.Count() > iWrite + static_cast< Index_t >( nMaxSpace ) )
			m_Events.Remove( m_Events.Count() - 1 );
	}

	void Broadcast( Ts... args )
	{
		Lock();

		for ( Index_t i = 0; i < m_Events.Count(); ++i )
		{
			if ( Handle( i ).IsValid() )
				Callback( i ).Execute( args... );
		}

		Unlock();
	}

	size32_t GetSize() const noexcept
	{
		size32_t nBound = 0;

		for ( Index_t i = 0; i < m_Events.Count(); ++i )
		{
			if ( Handle( i ).IsValid() )
				++nBound;
		}

		return nBound;
	}

protected:
	void Lock() noexcept { ++m_nLocks; }
	void Unlock()
	{
		BALL_ASSERT( m_nLocks > 0 );

		--m_nLocks;
	}
	bool IsLocked() const noexcept { return m_nLocks > 0; }
	DelegateHandle_t &Handle( Index_t i ) { return m_Events.template Get< DelegateHandle_t >( i ); }
	const DelegateHandle_t &Handle( Index_t i ) const { return m_Events.template Get< DelegateHandle_t >( i ); }
	SingleDelegate_t &Callback( Index_t i ) { return m_Events.template Get< SingleDelegate_t >( i ); }
	const SingleDelegate_t &Callback( Index_t i ) const { return m_Events.template Get< SingleDelegate_t >( i ); }

private:
	size32_t m_nLocks; //TODO: Make atomic family.
	MultiVector32_t< DelegateHandle_t, SingleDelegate_t > m_Events; //TODO: Make ordered slotvector -> map.
};

template < typename TSignature >
struct MDelegate;

template < typename R, typename ...Ts >
struct MDelegate< R ( Ts... ) >
{
	using Type = CDelegate< R, Ts... >;
};

template < typename TSignature >
struct MMulticastDelegate;

template < typename ...Ts >
struct MMulticastDelegate< void ( Ts... ) >
{
	using Type = CMulticastDelegate< Ts... >;
};

template < typename TSignature > using Delegate_t = typename MDelegate< TSignature >::Type;
template < typename TSignature > using MulticastDelegate_t = typename MMulticastDelegate< TSignature >::Type;

#endif // !defined( _INCLUDE_BALL_TYPES_DELEGATES_HPP_ )
