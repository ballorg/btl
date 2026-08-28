#ifndef _INCLUDE_BALL_TYPES_SLOTITERATOR_HPP_
#	define _INCLUDE_BALL_TYPES_SLOTITERATOR_HPP_

#	pragma once

#	if !defined( BALL_ENABLE_MODULE )
#		include "c/assert.h"
#		include "meta/conditional.hpp"
#	endif

///-----------------------------------------------------------------------------
/// @brief Slot iterator shared by the (SoA) map.
///
/// @details Both containers iterate by walking slot indices of a node-owning
/// container and dereferencing to the key, differing only in how a step advances
/// (an occupied-slot scan for the hash map, an in-order successor for the tree).
/// This factors out the common machinery: it holds an @p Owner pointer plus a slot
/// index and delegates every traversal decision to the owner, which must expose the
/// contract
///   - `Index_t` / `Key_t` member types and a `NIL_INDEX` static (the end sentinel),
///   - `Key( Index_t )` returning the key reference,
///   - `IsOccupied( Index_t )` for the debug asserts,
///   - `NextIndex( Index_t )` (advance) and, only where `operator--` is used,
///     `PrevIndex( Index_t )` (retreat).
///
/// `operator--` is a member template body, so it is instantiated lazily: a
/// forward-only owner (the hash map) that never decrements needs no `PrevIndex`,
/// while a bidirectional owner (the tree) gets a full bidirectional iterator.
///
/// @tparam Owner The node-owning container providing the contract above.
/// @tparam IS_CONST Whether this is a `const_iterator` (const owner, const key).
///
/// @complexity Construction, comparison, dereference and SlotIndex are O(1);
/// increment/decrement inherit the owner's step cost.
///-----------------------------------------------------------------------------
template < typename Owner, bool IS_CONST >
class CSlotIterator
{
private:
	using Owner_t = Conditional_t< IS_CONST, const Owner, Owner >;
	using Index_t = typename Owner::Index_t;
	using Key_t = typename Owner::Key_t;
	using Reference_t = Conditional_t< IS_CONST, const Key_t &, Key_t & >;
	using Pointer_t = Conditional_t< IS_CONST, const Key_t *, Key_t * >;

public:
	constexpr CSlotIterator() noexcept : m_iSlot( Owner::NIL_INDEX ), m_pOwner( nullptr ) {}
	constexpr CSlotIterator( Index_t iSlot, Owner_t *pOwner ) noexcept : m_iSlot( iSlot ), m_pOwner( pOwner ) {}
	constexpr CSlotIterator( const CSlotIterator & ) noexcept = default;
	constexpr CSlotIterator &operator=( const CSlotIterator & ) noexcept = default;
	constexpr CSlotIterator( const CSlotIterator< Owner, false > &other ) noexcept requires ( IS_CONST ) : m_iSlot( other.m_iSlot ), m_pOwner( other.m_pOwner ) {}

	constexpr Reference_t operator*() const
	{
		BALL_ASSERT_MESSAGE( m_iSlot != Owner::NIL_INDEX, "Iterator dereference cannot target end()/NIL_INDEX" );
		BALL_ASSERT_MESSAGE( m_pOwner, "Iterator dereference requires a valid owner" );
		BALL_ASSERT_MESSAGE( m_pOwner->IsOccupied( m_iSlot ), "Iterator dereference requires an occupied slot" );

		return m_pOwner->Key( m_iSlot );
	}

	constexpr Pointer_t operator->() const { return &operator*(); }

	constexpr CSlotIterator &operator++()
	{
		BALL_ASSERT_MESSAGE( m_iSlot != Owner::NIL_INDEX, "Iterator increment cannot start from end()/NIL_INDEX" );
		BALL_ASSERT_MESSAGE( m_pOwner, "Iterator increment requires a valid owner" );
		BALL_ASSERT_MESSAGE( m_pOwner->IsOccupied( m_iSlot ), "Iterator increment requires an occupied slot" );

		m_iSlot = m_pOwner->NextIndex( m_iSlot );

		return *this;
	}

	constexpr CSlotIterator operator++( int )
	{
		CSlotIterator copy( *this );

		++( *this );

		return copy;
	}

	// Bidirectional only: instantiated on use, so a forward-only owner never
	// requires a `PrevIndex`. `--end()` (m_iSlot == NIL_INDEX) yields the last slot.
	constexpr CSlotIterator &operator--()
	{
		BALL_ASSERT_MESSAGE( m_iSlot == Owner::NIL_INDEX || m_pOwner->IsOccupied( m_iSlot ), "Iterator decrement requires end() or an occupied slot" );
		BALL_ASSERT_MESSAGE( m_pOwner, "Iterator decrement requires a valid owner" );

		m_iSlot = m_pOwner->PrevIndex( m_iSlot );

		return *this;
	}

	constexpr CSlotIterator operator--( int )
	{
		CSlotIterator copy( *this );

		--( *this );

		return copy;
	}

	template < bool RHS_CONST > constexpr bool operator==( const CSlotIterator< Owner, RHS_CONST > &rhs ) const noexcept { return m_pOwner == rhs.m_pOwner && m_iSlot == rhs.m_iSlot; }
	template < bool RHS_CONST > constexpr bool operator!=( const CSlotIterator< Owner, RHS_CONST > &rhs ) const noexcept { return !( *this == rhs ); }

	constexpr Index_t SlotIndex() const noexcept { return m_iSlot; }

private:
	template < typename, bool > friend class CSlotIterator;

	Index_t m_iSlot;
	Owner_t *m_pOwner;
};

#endif // !defined( _INCLUDE_BALL_TYPES_SLOTITERATOR_HPP_ )
