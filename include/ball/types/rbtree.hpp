#ifndef _INCLUDE_BALL_TYPES_RBTREE_HPP_
#	define _INCLUDE_BALL_TYPES_RBTREE_HPP_

#	pragma once

#	include "base/arch.h"
#	include "c/assert.h"
#	include "elements.hpp"
#	include "fixed.hpp"
#	include "meta/conditional.hpp"
#	include "meta/fixed.hpp"
#	include "meta/get.hpp"
#	include "meta/indexsequence.hpp"
#	include "meta/indextype.hpp"
#	include "meta/reflectvalue.hpp"
#	include "meta/xvalue.hpp"
#	include "multivector.hpp"
#	include "pair.hpp"
#	include "vector.hpp"

enum class ERBTreeColor : uint1_t
{
	BLACK = 0,
	RED = 1
};
BALL_FIXED_UNSIGNED_ENUM_TRAIT( ERBTreeColor, 1 );

///-----------------------------------------------------------------------------
/// @brief Iterates @p tree in ascending key order, binding each slot index to @p it.
/// 
/// @details Forward in-order walk from `FirstIndex()` to `EndIndex()` (`NIL_INDEX`). 
/// @p it is declared by the macro and holds the current node's `Index_t`; access 
/// payload columns with `Get< TN >( it )`. Safe on an empty tree.
/// 
/// @complexity O(n) for the whole loop: n inorder steps, each O(1) amortized.
///-----------------------------------------------------------------------------
#	define BALL_RBTREE_FOREACH( tree, it ) \
		for ( auto it = ( tree ).FirstIndex(); it != ( tree ).EndIndex(); it = ( tree ).NextIndex( it ) )

///-----------------------------------------------------------------------------
/// @brief Iterates @p tree in descending key order, binding each slot index to @p it.
/// 
/// @details Reverse in-order walk: `PrevIndex( EndIndex() )` yields the rightmost 
/// node, and each step walks to the inorder predecessor until it returns 
/// `EndIndex()` again, so the body always sees a valid node index. Safe on an 
/// empty tree (the body never runs).
/// 
/// @complexity O(n) for the whole loop: n reverse-inorder steps, each O(1) amortized.
///-----------------------------------------------------------------------------
#	define BALL_RBTREE_FOREACH_REVERSE( tree, it ) \
		for ( auto it = ( tree ).PrevIndex( ( tree ).EndIndex() ); it != ( tree ).EndIndex(); it = ( tree ).PrevIndex( it ) )

///-----------------------------------------------------------------------------
/// @brief Iterates @p tree in storage (slot) order from `FIRST_INDEX`, not key order.
/// 
/// @details Storage is dense (compact-on-erase leaves no holes), so every slot in 
/// [`FIRST_INDEX`, `Count()`) is a live node: this is a linear, cache-friendly sweep 
/// with no link chasing, at the cost of an arbitrary visiting order -- rows are 
/// permuted by insertion history and compaction moves. @p it holds the slot 
/// `Index_t`, same as the ordered macros. Safe on an empty tree.
/// 
/// @complexity O(n) for the whole loop: one O(1) step per slot.
///-----------------------------------------------------------------------------
#	define BALL_RBTREE_FOREACH_UNORDERED( tree, it ) \
		for ( auto it = ( tree ).FIRST_INDEX; it != ( tree ).Count(); ++it )

///-----------------------------------------------------------------------------
/// @brief Iterates @p tree in reverse storage (slot) order, from the last slot 
/// down to `FIRST_INDEX`; not key order.
/// 
/// @details Mirror of `BALL_RBTREE_FOREACH_UNORDERED` with the same dense-storage 
/// guarantees. The downward direction additionally tolerates removing the current 
/// node inside the body: compact-on-erase relocates the last row into the vacated 
/// slot, and in a downward sweep that row was already visited, so every node is 
/// still visited exactly once. Safe on an empty tree.
/// 
/// @complexity O(n) for the whole loop: one O(1) step per slot.
///-----------------------------------------------------------------------------
#	define BALL_RBTREE_FOREACH_UNORDERED_REVERSE( tree, it ) \
		for ( auto it = ( tree ).Count(); it-- != ( tree ).FIRST_INDEX; )

///-----------------------------------------------------------------------------
/// @brief Default strict-weak-order comparator for tree keys.
/// 
/// @details Red-black tree search, insertion and balancing logic assume that 
/// this predicate induces a strict weak ordering over the key domain.
/// 
/// @complexity operator() is O(1) for the built-in `<` it wraps.
///-----------------------------------------------------------------------------
template < typename T = void >
struct CRBTreeLess
{
	constexpr bool operator()( const T &left, const T &right ) const noexcept { return left < right; }
};

/// @brief Transparent specialization deducing both operands, used as the default 
/// comparator when the key type is not yet known at the point of declaration.
template <>
struct CRBTreeLess< void >
{
	template < typename L, typename R > constexpr bool operator()( const L &left, const R &right ) const noexcept { return left < right; }
};

// Red-black tree link columns: gives each of the tree's 
// left/right/parent columns a distinct CReflect type, with the index type supplied per 
// use as RBTreeLeftColumn_t< I > etc. Declared outside the tree so the base-clause 
// below can name the storage type, without a wrapper struct to thread I.
BALL_REFLECT_TAGGED_TEMPLATE( RBTreeLeftColumn );
BALL_REFLECT_TAGGED_TEMPLATE( RBTreeRightColumn );
BALL_REFLECT_TAGGED_TEMPLATE( RBTreeParentColumn );

///-----------------------------------------------------------------------------
/// @brief Structure-of-arrays red-black tree core with an out-of-band NIL.
/// 
/// @details `NIL_INDEX` is the out-of-band invalid index (-1): a virtual black 
/// `end()`/no-child marker that owns no physical row. Real nodes occupy slots 
/// `FIRST_INDEX` (0) and up, so slot 0 is an ordinary black node (the first 
/// root), not a reserved sentinel. The accessors map this -1 to a black, 
/// self-linked virtual leaf so the balancing code can treat "no child" like a 
/// black leaf, mirroring the classic CLRS sentinel model without spending a slot 
/// on it. The root is reachable in O(1) without any dedicated member: slot 0's 
/// parent cell always stores the root's index encoded as `~R`, while slot 0's 
/// real parent is displaced into the root's own (by definition NIL) parent cell 
/// -- ParentOf/SetParent remap the two cells and no element ever moves for it. 
/// The balancing rules and complexity guarantees follow the standard 
/// red-black tree model: 
/// https://www.nist.gov/dads/HTML/redblack.html 
/// https://tildesites.bowdoin.edu/~ltoma/teaching/cs231/fall07/MIT/rbtrees-MIT.pdf
///-----------------------------------------------------------------------------
///-----------------------------------------------------------------------------
/// @brief Multi-column (SoA) red-black tree core.
/// 
/// @details The payload is the key column @p K followed by an arbitrary pack 
/// @p Ts...: each is stored in its own column alongside the tree metadata 
/// columns (color, left, right, parent). The ordering key is column 0 (type 
/// @p K) and is compared with @p C. @p TI is the type used to index columns in 
/// the underlying SoA storage. Payload columns are reachable by compile-time 
/// index through `Column_t< TN >` / `Get< TN >`, mirroring `CMultiVector` 
/// (column 0 is the key, columns 1.. are @p Ts).
template < typename I, I N, typename TI, typename C, typename K, typename... Ts >
class CMultiRBTreeBase : public C, public CBufferMultiVector< I, N, TI, ERBTreeColor, RBTreeLeftColumn_t< I >, RBTreeRightColumn_t< I >, RBTreeParentColumn_t< I >, K, Ts... >
{
public:
	using TagColumn_t = ERBTreeColor;

	// Link columns, tagged by the RBTree*Column_t alias templates above the class.
	using LeftColumn_t = RBTreeLeftColumn_t< I >;
	using RightColumn_t = RBTreeRightColumn_t< I >;
	using ParentColumn_t = RBTreeParentColumn_t< I >;

	/// @brief Type of the @p TN -th payload column (column 0 is the key @p K).
	template < TI TN > using Column_t = typename MIndexType< TI, TN, K, Ts... >::Type;

	// The SoA node storage is the protected base; Nodes() exposes it as Base_t.
	using Base_t = CBufferMultiVector< I, N, TI, TagColumn_t, LeftColumn_t, RightColumn_t, ParentColumn_t, K, Ts... >;
	using Tree_t = Base_t;
	using Value_t = Column_t< 0 >;
	using Key_t = K;
	using Compare_t = C;
	using Color_t = TagColumn_t;
	using Index_t = I;
	using Fixed_t = MFixed< Index_t >;

	/// @brief Type used to index columns of the underlying SoA storage.
	using TypeIndex_t = TI;

	/// @brief Total payload columns: the key column plus @p Ts.
	static constexpr size_t COLUMN_COUNT = 1 + sizeof...( Ts );

public:
	static constexpr I COLUMN_NIL_INDEX = MFixed< I >::INVALID;

	// Access to the comparator empty-base subobject. Routing every use through this method 
	// keeps the call sites (Less, cross-capacity CopyFrom / MoveFrom) from naming the base or 
	// static_cast-ing to it, and gives a single place to read or carry the comparator across 
	// sibling instantiations.
	constexpr Compare_t &Comparator() noexcept { return *this; }
	constexpr const Compare_t &Comparator() const noexcept { return *this; }

	constexpr Tree_t &Nodes() noexcept { return *this; }
	constexpr const Tree_t &Nodes() const noexcept { return *this; }

	// Metadata columns (color, left, right, parent) precede the payload columns 
	// (key + values); their count is whatever Tree_t carries beyond the payload.
	static constexpr TI PAYLOAD_COLUMN_OFFSET = Tree_t::NUM_TYPES - COLUMN_COUNT;

	using ColumnSequence_t = MakeIndexSequence_t< size_t, 1 + sizeof...( Ts ) >;

	template < TI TN > constexpr Column_t< TN > *PayloadColumn() noexcept { return Get< PAYLOAD_COLUMN_OFFSET + TN >( Nodes() ); }
	template < TI TN > constexpr const Column_t< TN > *PayloadColumn() const noexcept { return Get< PAYLOAD_COLUMN_OFFSET + TN >( Nodes() ); }

public:
	// "No node" / end / no-child is the out-of-band invalid index (-1): it owns no 
	// physical row and fails IsValidIndex, so all control flow tests against 
	// NIL_INDEX. Real nodes occupy slots FIRST_INDEX (0) and up; slot 0 is an 
	// ordinary black node, not a reserved sentinel.
	static constexpr Index_t NIL_INDEX = COLUMN_NIL_INDEX;
	static constexpr Index_t FIRST_INDEX = 0;
	// Alias of NIL_INDEX kept for diagnostics: the never-valid (-1) index.
	static constexpr Index_t INVALID_INDEX = COLUMN_NIL_INDEX;

	///-----------------------------------------------------------------------------
	/// @brief Minimal bidirectional iterator over occupied tree slots.
	/// 
	/// @details `end()` is represented by the virtual NIL sentinel and is decrementable, 
	/// which matches the standard bidirectional iterator requirement 
	/// that a past-the-end iterator may still be decrementable when a predecessor exists: 
	/// https://cppreference.com/w/cpp/iterator/bidirectional_iterator.html
	/// 
	/// @complexity Construction, comparison, dereference and SlotIndex are O(1). The 
	/// increment/decrement operators are O(log n) worst case (one inorder step), but 
	/// O(1) amortized, so a full begin()->end() traversal is O(n) overall.
	///-----------------------------------------------------------------------------
	template < bool IS_CONST >
	class CIterator
	{
	private:
		using TreeOwner_t = Conditional_t< IS_CONST, const CMultiRBTreeBase, CMultiRBTreeBase >;
		using Reference_t = Conditional_t< IS_CONST, const Value_t &, Value_t & >;
		using Pointer_t = Conditional_t< IS_CONST, const Value_t *, Value_t * >;

	public:
		constexpr CIterator() noexcept : m_iNode( NIL_INDEX ), m_pTree( nullptr ) {}
		constexpr CIterator( TreeOwner_t *pTree, Index_t iNode ) noexcept : m_iNode( iNode ), m_pTree( pTree ) {}
		constexpr CIterator( const CIterator &other ) noexcept = default;
		constexpr CIterator &operator=( const CIterator &other ) noexcept = default;
		constexpr CIterator( const CIterator< false > &other ) noexcept requires ( IS_CONST ) : m_iNode( other.m_iNode ), m_pTree( other.m_pTree ) {}

		constexpr Reference_t operator*() const
		{
			BALL_ASSERT_MESSAGE( m_iNode != NIL_INDEX, "Iterator dereference cannot target end()/NIL_INDEX" );
			BALL_ASSERT_MESSAGE( m_pTree, "Iterator dereference requires a valid tree owner" );
			BALL_ASSERT_MESSAGE( m_pTree->IsOccupied( m_iNode ), "Iterator dereference requires an occupied tree slot" );

			return m_pTree->Key( m_iNode );
		}

		constexpr Pointer_t operator->() const { return &operator*(); }

		constexpr CIterator &operator++()
		{
			BALL_ASSERT_MESSAGE( m_iNode != NIL_INDEX, "Iterator increment cannot start from end()/NIL_INDEX" );
			BALL_ASSERT_MESSAGE( m_pTree, "Iterator increment requires a valid tree owner" );
			BALL_ASSERT_MESSAGE( m_pTree->IsOccupied( m_iNode ), "Iterator increment requires an occupied tree slot" );

			m_iNode = m_pTree->NextIndex( m_iNode );

			return *this;
		}

		constexpr CIterator operator++( int )
		{
			CIterator copy( *this );

			++( *this );

			return copy;
		}

		constexpr CIterator &operator--()
		{
			BALL_ASSERT_MESSAGE( m_iNode == NIL_INDEX || m_pTree->IsOccupied( m_iNode ), "Iterator decrement requires end() or an occupied tree slot" );
			BALL_ASSERT_MESSAGE( m_pTree, "Iterator decrement requires a valid tree owner" );

			m_iNode = m_pTree->PrevIndex( m_iNode );

			return *this;
		}

		constexpr CIterator operator--( int )
		{
			CIterator copy( *this );

			--( *this );

			return copy;
		}

		template < bool RHS_CONST > constexpr bool operator==( const CIterator< RHS_CONST > &rhs ) const noexcept { return m_pTree == rhs.m_pTree && m_iNode == rhs.m_iNode; }
		template < bool RHS_CONST > constexpr bool operator!=( const CIterator< RHS_CONST > &rhs ) const noexcept { return !( *this == rhs ); }

		constexpr Index_t SlotIndex() const noexcept { return m_iNode; }

	private:
		template < bool > friend class CIterator;

		Index_t m_iNode;
		TreeOwner_t *m_pTree;
	};

	using iterator = CIterator< false >;
	using const_iterator = CIterator< true >;

	/// @complexity O(1).
	constexpr CMultiRBTreeBase() noexcept : CMultiRBTreeBase( Compare_t() ) {}

	/// No reserved sentinel row: NIL is the out-of-band -1, so storage starts empty 
	/// and the 0th BLACK node is used for real data only once the first insert has 
	/// values to fill it, never seeded with a placeholder up front.
	/// 
	/// @complexity O(1).
	constexpr explicit CMultiRBTreeBase( const Compare_t &compare ) noexcept :
		Compare_t( compare ),
		Base_t()
	{
		BALL_ASSERT_MESSAGE( TreeCount() == FIRST_INDEX, "Tree storage starts empty without a reserved sentinel" );
	}

	/// Erase compacts the array (the last node fills the vacated slot), so storage is always 
	/// dense -- every row in [0, TreeCount()) is a live node. The storage base can destruct 
	/// the whole range directly; there are no raw holes to revive first.
	/// 
	/// @complexity O(n): the storage base destructs every live row.
	~CMultiRBTreeBase() noexcept = default;

protected:
	// Bring the inherited SoA storage helper into scope so the tree calls it directly 
	// instead of qualifying through Nodes(). Count() is not pulled in: it would collide 
	// with the occupied-node Count() below, so storage row count is reached through 
	// TreeCount() / Tree_t::Count(). Member templates taking explicit type args 
	// (At<>, PackedBaseBy<>) still need the this->template form, not a using.
	using Tree_t::PackedBaseBy;
	using Tree_t::AddToTail;

	/// Storage is kept dense by compact-on-erase, so the live-node count is exactly the 
	/// storage row count -- no occupancy scan needed.
	/// 
	/// @complexity O(1).
	constexpr I Count() const noexcept { return TreeCount(); }

	/// Climb parent links from a known occupied node to the root (parent == NIL). Kept as 
	/// the encoding-independent way to reach the root: Validate uses it to cross-check the 
	/// root cell (see RootIndex below), and asserts can call it from any occupied node.
	/// 
	/// @complexity O(log n): one climb up the tree height.
	constexpr Index_t RootIndex( Index_t iFrom ) const noexcept
	{
		BALL_ASSERT_MESSAGE( IsOccupied( iFrom ), "Climb requires an occupied start node" );

		while ( !IsNil( ParentOf( iFrom ) ) )
			iFrom = ParentOf( iFrom );

		return iFrom;
	}

	/// The root's physical index R is encoded in the parent cell of slot FIRST_INDEX as 
	/// `~R` (== -(R + 1)): the root needs no parent value of its own (it is NIL by 
	/// definition), so that cell is free to act as the root pointer, and slot 0's real 
	/// parent is displaced into the root's parent cell instead (see ParentOf/SetParent). 
	/// With the root at slot 0 the cell holds `~0 == -1` -- exactly the plain "parent is 
	/// NIL" value, so the indirection only kicks in once the root moves away from slot 0. 
	/// No elements are ever relocated to maintain this. Empty tree reports NIL.
	/// 
	/// @complexity O(1): decode the root cell, no climb and no scan.
	constexpr Index_t RootIndex() const noexcept
	{
		return TreeCount() != FIRST_INDEX ? static_cast< Index_t >( ~static_cast< Index_t >( ParentColumn()[ FIRST_INDEX ] ) ) : NIL_INDEX;
	}

	/// Smallest key = leftmost descendant of the root; NIL when the tree is empty.
	/// 
	/// @complexity O(log n): resolve the root in O(1), then descend the left spine.
	constexpr Index_t LeftmostIndex() const noexcept
	{
		const Index_t iRoot = RootIndex();

		return IsNil( iRoot ) ? NIL_INDEX : Minimum( iRoot );
	}

	/// Largest key = rightmost descendant of the root; NIL when the tree is empty.
	/// 
	/// @complexity O(log n): resolve the root in O(1), then descend the right spine.
	constexpr Index_t RightmostIndex() const noexcept
	{
		const Index_t iRoot = RootIndex();

		return IsNil( iRoot ) ? NIL_INDEX : Maximum( iRoot );
	}

	/// @complexity The following index/predicate helpers are all O(1): NilIndex, TreeCount, 
	/// IsFirst, IsNil, IsOccupied, IsValidIndex, IsNilOrValid, IsNilOrOccupied, 
	/// LeftIndex, RightIndex, ParentIndex and Color each do a constant amount of work.
	static constexpr Index_t NilIndex() noexcept { return NIL_INDEX; }

	constexpr Index_t TreeCount() const noexcept { return Tree_t::Count(); }

	constexpr bool IsFirst( Index_t iNode ) const noexcept { return iNode == FIRST_INDEX; }
	constexpr bool IsNil( Index_t iNode ) const noexcept { return iNode == NIL_INDEX; }

	// Storage is dense (compact-on-erase leaves no holes), so an in-range slot is exactly an 
	// occupied node: occupancy is just the range test, with no free flag to consult.
	constexpr bool IsOccupied( Index_t iNode ) const noexcept { return IsValidIndex( iNode ); }
	constexpr bool IsValidIndex( Index_t iNode ) const noexcept { return FIRST_INDEX <= iNode && iNode < TreeCount(); }

	// Local assertion predicates centralizing the "-1 (NIL) or real slot" guards used 
	// across the link/validity asserts below.
	constexpr bool IsNilOrValid( Index_t iNode ) const noexcept { return IsNil( iNode ) || IsValidIndex( iNode ); }
	constexpr bool IsNilOrOccupied( Index_t iNode ) const noexcept { return IsNil( iNode ) || IsOccupied( iNode ); }

	constexpr Index_t LeftIndex( Index_t iNode ) const noexcept { return LeftOf( iNode ); }
	constexpr Index_t RightIndex( Index_t iNode ) const noexcept { return RightOf( iNode ); }
	constexpr Index_t ParentIndex( Index_t iNode ) const noexcept { return ParentOf( iNode ); }
	constexpr Color_t Color( Index_t iNode ) const noexcept { return ColorOf( iNode ); }

	/// @brief Accesses the @p TN -th payload column value of an occupied node.
	/// 
	/// @complexity O(1).
	template < TI TN > constexpr Column_t< TN > &Column( Index_t iNode )
	{
		BALL_ASSERT_MESSAGE( !IsNil( iNode ), "Access cannot target the sentinel slot" );
		BALL_ASSERT_MESSAGE( IsValidIndex( iNode ), "Access requires an in-range slot index" );
		BALL_ASSERT_MESSAGE( IsOccupied( iNode ), "Access requires an occupied tree slot" );

		return PayloadColumn< TN >()[ iNode ];
	}

	/// @complexity O(1).
	template < TI TN > constexpr const Column_t< TN > &Column( Index_t iNode ) const
	{
		BALL_ASSERT_MESSAGE( !IsNil( iNode ), "Access cannot target the sentinel slot" );
		BALL_ASSERT_MESSAGE( IsValidIndex( iNode ), "Access requires an in-range slot index" );
		BALL_ASSERT_MESSAGE( IsOccupied( iNode ), "Access requires an occupied tree slot" );

		return PayloadColumn< TN >()[ iNode ];
	}

	// Key and EndIndex are O(1). FirstIndex is O(log n) (leftmost node). NextIndex and 
	// PrevIndex are O(log n) worst case / O(1) amortized (one inorder step).
	constexpr Value_t &Key( Index_t iNode ) { return Column< 0 >( iNode ); }
	constexpr const Value_t &Key( Index_t iNode ) const { return Column< 0 >( iNode ); }
	constexpr Index_t FirstIndex() const noexcept { return LeftmostIndex(); }
	constexpr Index_t EndIndex() const noexcept { return NIL_INDEX; }
	constexpr Index_t NextIndex( Index_t iNode ) const noexcept
	{
		BALL_ASSERT_MESSAGE( !IsNil( iNode ), "Non-end iterator index" );

		return Successor( iNode );
	}
	constexpr Index_t PrevIndex( Index_t iNode ) const noexcept
	{
		if ( IsNil( iNode ) )
			return RightmostIndex();

		return Predecessor( iNode );
	}

	// Iterator() wraps an index in O(1). begin()/cbegin() are O(log n) (leftmost node); 
	// end()/cend() are O(1) (the NIL sentinel).
	constexpr iterator Iterator( Index_t iNode ) noexcept { return iterator( this, iNode ); }
	constexpr const_iterator Iterator( Index_t iNode ) const noexcept { return const_iterator( this, iNode ); }

	constexpr iterator begin() noexcept { return Iterator( FirstIndex() ); }
	constexpr iterator end() noexcept { return Iterator( EndIndex() ); }
	constexpr const_iterator begin() const noexcept { return Iterator( FirstIndex() ); }
	constexpr const_iterator end() const noexcept { return Iterator( EndIndex() ); }
	constexpr const_iterator cbegin() const noexcept { return Iterator( FirstIndex() ); }
	constexpr const_iterator cend() const noexcept { return Iterator( EndIndex() ); }

	// Column base-pointer accessors are all O(1).
	constexpr LeftColumn_t *LeftColumn() noexcept { return Get< LeftColumn_t >( Nodes() ); }
	constexpr const LeftColumn_t *LeftColumn() const noexcept { return Get< LeftColumn_t >( Nodes() ); }
	constexpr RightColumn_t *RightColumn() noexcept { return Get< RightColumn_t >( Nodes() ); }
	constexpr const RightColumn_t *RightColumn() const noexcept { return Get< RightColumn_t >( Nodes() ); }
	constexpr ParentColumn_t *ParentColumn() noexcept { return Get< ParentColumn_t >( Nodes() ); }
	constexpr const ParentColumn_t *ParentColumn() const noexcept { return Get< ParentColumn_t >( Nodes() ); }
	constexpr uchar_t *TagColumn() noexcept { return Tree_t::template PackedBaseBy< TagColumn_t >(); }
	constexpr const uchar_t *TagColumn() const noexcept { return Tree_t::template PackedBaseBy< TagColumn_t >(); }

	///-----------------------------------------------------------------------------
	/// @brief Validates the full RB-tree topology and all SoA payload links.
	/// 
	/// @complexity O(n): one recursive walk of every node plus a linear visited-slot 
	/// sweep; allocates an O(n) visited bitmap.
	///-----------------------------------------------------------------------------
	constexpr bool Validate() const
	{
		const Index_t iRoot = RootIndex();
		const I nCount = Count();

		if ( !nCount || !TreeCount() )
			return IsNil( iRoot ) && IsNil( LeftmostIndex() ) && IsNil( RightmostIndex() );

		if ( IsNil( iRoot ) || !IsNil( ParentOf( iRoot ) ) || ColorOf( iRoot ) != Color_t::BLACK )
			return false;

		// The encoded root cell must agree with the encoding-independent parent climb.
		if ( RootIndex( FIRST_INDEX ) != iRoot )
			return false;

		CVector< I, bool > vecVisited;

		vecVisited.Grow( TreeCount() );

		for ( I i = 0; i < TreeCount(); ++i )
			vecVisited.PackedSetValue( i, false );

		// NIL (-1) anchors "no in-order predecessor yet" for the ordering check below.
		Index_t iPrev = NIL_INDEX;

		I nVisited = 0;
		I nBlackHeight = 0;

		if ( !ValidateNode( iRoot, NIL_INDEX, nullptr, nullptr, vecVisited, iPrev, nVisited, nBlackHeight ) )
			return false;

		if ( nVisited != nCount )
			return false;

		// Storage is dense: every slot in [0, TreeCount()) is a live node and must have been 
		// reached by the structural walk above -- there are no free holes to account for.
		for ( Index_t i = FIRST_INDEX; i < TreeCount(); ++i )
		{
			if ( !vecVisited[ i ] )
				return false;
		}

		if ( LeftmostIndex() != Minimum( iRoot ) || RightmostIndex() != Maximum( iRoot ) )
			return false;

		return true;
	}

protected:
	///-----------------------------------------------------------------------------
	/// @brief Compares two keys using the configured comparison functor.
	/// 
	/// @complexity O(1) (assuming an O(1) comparator).
	///-----------------------------------------------------------------------------
	constexpr bool Less( const Key_t &left, const Key_t &right ) const noexcept { return Comparator()( left, right ); }

	///-----------------------------------------------------------------------------
	/// @brief Allocates a tree slot and initializes RB metadata eagerly.
	/// 
	/// @see RB-INSERT initializes the new node as red before the fix-up pass:
	/// 
	/// @details Always constructs a fresh node at the tail via `AddToTail`. Storage stays 
	/// dense because erase compacts (the last node fills the vacated slot), so there are 
	/// never holes to reclaim -- insert simply appends and the new index is the old count.
	/// 
	/// @complexity O(1) amortized; O(n) on the appends that grow and reallocate storage.
	///-----------------------------------------------------------------------------
	template < typename... Us >
	constexpr Index_t AddNode( Index_t iParent, Us &&...values )
	{
		BALL_ASSERT_MESSAGE( IsNilOrOccupied( iParent ), "Requires the parent to be sentinel or occupied" );

		const Index_t iAppended = TreeCount();

		// Seed the parent link in the appended row directly instead of a follow-up 
		// SetParent: the new node has no children yet, so left/right stay NIL. The raw 
		// write is encoding-correct: the first node stores NIL (-1 == ~0), which is 
		// exactly the root cell saying "the root is slot 0"; any later node is neither 
		// slot 0 nor the root, so its cell holds its own parent verbatim.
		AddToTail( Color_t::RED, NIL_INDEX, NIL_INDEX, iParent, Forward< Us >( values )... );

		return iAppended;
	}

	/// @complexity O(log n): a single root-to-leaf BST descent.
	constexpr Index_t LowerBoundIndex( const Key_t &key ) const noexcept
	{
		Index_t iNode = RootIndex();
		Index_t iCandidate = NIL_INDEX;

		while ( !IsNil( iNode ) )
		{
			const Key_t &nodeKey = Key( iNode );

			if ( Less( nodeKey, key ) )
			{
				iNode = RightOf( iNode );
			}
			else
			{
				iCandidate = iNode;
				iNode = LeftOf( iNode );
			}
		}

		return iCandidate;
	}

	/// @complexity O(log n): a single root-to-leaf BST descent.
	constexpr Index_t UpperBoundIndex( const Key_t &key ) const noexcept
	{
		Index_t iNode = RootIndex();
		Index_t iCandidate = NIL_INDEX;

		while ( !IsNil( iNode ) )
		{
			const Key_t &nodeKey = Key( iNode );

			if ( Less( key, nodeKey ) )
			{
				iCandidate = iNode;
				iNode = LeftOf( iNode );
			}
			else
			{
				iNode = RightOf( iNode );
			}
		}

		return iCandidate;
	}

	/// @complexity O(log n): a single root-to-leaf BST descent.
	constexpr Index_t FindIndex( const Key_t &key ) const noexcept
	{
		Index_t iNode = RootIndex();

		while ( !IsNil( iNode ) )
		{
			const Key_t &nodeKey = Key( iNode );

			if ( Less( key, nodeKey ) )
				iNode = LeftOf( iNode );
			else if ( Less( nodeKey, key ) )
				iNode = RightOf( iNode );
			else
				return iNode;
		}

		return NIL_INDEX;
	}

	///-----------------------------------------------------------------------------
	/// @brief Destructs the @p TN -th payload element of @p iNode, leaving the slot raw.
	/// 
	/// @details Called by `CompactAfterRemoval` to release a removed node before the last 
	/// row is relocated into its slot. The slot is left raw only transiently: compaction 
	/// either move-constructs the relocated payload into it or drops it via the storage 
	/// shrink, so storage stays dense with no lingering holes.
	/// 
	/// @complexity O(1) per column; DestructPayload folds over the fixed column count, O(1).
	///-----------------------------------------------------------------------------
	template < TI TN >
	constexpr void DestructPayloadColumn( Index_t iNode ) noexcept
	{
		DestructElement( &PayloadColumn< TN >()[ iNode ] );
	}

	template < size_t... TNs >
	constexpr void DestructPayload( Index_t iNode, MIndexSequence< size_t, TNs... > ) noexcept
	{
		( DestructPayloadColumn< static_cast< TI >( TNs ) >( iNode ), ... );
	}

	///-----------------------------------------------------------------------------
	/// @brief Move-constructs the @p TN -th payload element of @p iTo from @p iFrom.
	/// 
	/// @details Used by compaction to relocate the last row into a vacated slot. @p iTo 
	/// must be raw (its old payload already destructed); @p iFrom is left in a moved-from 
	/// state and is destructed by the subsequent storage shrink.
	/// 
	/// @complexity O(1) per column; ConstructPayload folds over the fixed column count, O(1).
	///-----------------------------------------------------------------------------
	template < TI TN >
	constexpr void ConstructPayloadColumn( Index_t iTo, Index_t iFrom ) noexcept
	{
		ConstructElement( &PayloadColumn< TN >()[ iTo ], Move( PayloadColumn< TN >()[ iFrom ] ) );
	}

	template < size_t... TNs >
	constexpr void ConstructPayload( Index_t iTo, Index_t iFrom, MIndexSequence< size_t, TNs... > ) noexcept
	{
		( ConstructPayloadColumn< static_cast< TI >( TNs ) >( iTo, iFrom ), ... );
	}

	///-----------------------------------------------------------------------------
	/// @brief Removes the node at @p iHole physically by compacting the storage dense.
	/// 
	/// @details The node must already be spliced out of the tree (no links point to it). 
	/// Its payload is released, then the last row is relocated into the vacated slot and 
	/// every link that referenced the moved node is repointed to its new index, so the 
	/// array stays hole-free in [0, TreeCount()). The single relocation moves one node's 
	/// data; everything else is index rewiring. Returns the slot the relocated node came 
	/// from (the old last index), or NIL when @p iHole was already the last slot, so the 
	/// caller can remap any index it still holds into the moved node's new home.
	/// 
	/// @complexity O(1) amortized -- one payload relocation plus constant link rewiring; 
	/// O(n) only on the storage shrink that reallocates across a capacity boundary.
	///-----------------------------------------------------------------------------
	constexpr Index_t CompactAfterRemoval( Index_t iHole ) noexcept
	{
		BALL_ASSERT_MESSAGE( IsValidIndex( iHole ), "Requires an in-range slot index" );

		const Index_t iLast = TreeCount() - 1;

		if ( iHole != iLast )
		{
			// Release the removed node, then move the last node into its slot. The shrink 
			// below destructs the moved-from last row exactly once.
			DestructPayload( iHole, ColumnSequence_t() );
			ConstructPayload( iHole, iLast, ColumnSequence_t() );

			const Index_t iParent = ParentOf( iLast );
			const Index_t iLeft = LeftOf( iLast );
			const Index_t iRight = RightOf( iLast );

			SetColor( iHole, ColorOf( iLast ) );
			SetParent( iHole, iParent );
			SetLeft( iHole, iLeft );
			SetRight( iHole, iRight );

			// Repoint the moved node's neighbours (parent's child link, children's parent 
			// links) to its new slot. A NIL parent means the moved node was the root: the 
			// SetParent above then performed a root transfer, re-encoding the root cell 
			// onto the hole, so no follow-up fix is needed here.
			if ( !IsNil( iParent ) )
			{
				if ( LeftOf( iParent ) == iLast )
					SetLeft( iParent, iHole );
				else
					SetRight( iParent, iHole );
			}

			if ( !IsNil( iLeft ) )
				SetParent( iLeft, iHole );

			if ( !IsNil( iRight ) )
				SetParent( iRight, iHole );
		}

		// Drop the last row: storage destructs the removed payload (iHole == iLast) or the 
		// moved-from row (iHole != iLast).
		Nodes().SetCount( iLast );

		return iHole != iLast ? iLast : NIL_INDEX;
	}

	///-----------------------------------------------------------------------------
	/// @brief Performs the canonical left rotation around @p x using SoA columns.
	/// 
	/// @details This is the standard BST rotation that preserves inorder key 
	/// ordering while changing only local parent/child links: 
	/// The rebalance logic uses this primitive when a red child leans to the 
	/// "inside" or when deletion needs to move one extra black level from the 
	/// parent/sibling side down toward @p x. The rotation does not change key 
	/// order; it only changes which node becomes the local subtree root.
	/// 
	/// @complexity O(1): a fixed number of link rewrites.
	///-----------------------------------------------------------------------------
	constexpr void RotateLeft( Index_t x )
	{
		const Index_t y = RightOf( x );

		BALL_ASSERT_MESSAGE( !IsNil( x ), "Non-sentinel pivot" );
		BALL_ASSERT_MESSAGE( IsValidIndex( x ), "In-range pivot index" );
		BALL_ASSERT_MESSAGE( IsOccupied( x ), "Occupied pivot" );

		BALL_ASSERT_MESSAGE( !IsNil( y ), "Non-sentinel right child" );
		BALL_ASSERT_MESSAGE( IsValidIndex( y ), "In-range right child index" );
		BALL_ASSERT_MESSAGE( IsOccupied( y ), "Occupied right child" );

		// Move y's left subtree to x's right: all keys stay between x and y.
		SetRight( x, LeftOf( y ) );

		if ( !IsNil( LeftOf( y ) ) )
			SetParent( LeftOf( y ), x );

		const Index_t iParent = ParentOf( x );

		SetParent( y, iParent );

		// When x was the root, the SetParent above already made y root (parent == NIL); 
		// otherwise relink y under x's former parent on the correct side.
		if ( !IsNil( iParent ) )
		{
			if ( x == LeftOf( iParent ) )
				SetLeft( iParent, y );
			else
				SetRight( iParent, y );
		}

		// Promote y above x; x becomes the left child of the new local root.
		SetLeft( y, x );
		SetParent( x, y );
	}

	///-----------------------------------------------------------------------------
	/// @brief Performs the canonical right rotation around @p x using SoA columns.
	/// 
	/// @details Mirror of `RotateLeft`; the same local-link preservation rules 
	/// apply and inorder traversal remains unchanged. 
	/// In balancing terms this is the symmetric operation used when the heavy / 
	/// violating side is on the left. Together, left/right rotations are the only 
	/// structural edits needed; all other balancing work is recoloring.
	/// 
	/// @complexity O(1): a fixed number of link rewrites.
	///-----------------------------------------------------------------------------
	constexpr void RotateRight( Index_t x )
	{
		const Index_t y = LeftOf( x );

		BALL_ASSERT_MESSAGE( !IsNil( x ), "Non-sentinel pivot" );
		BALL_ASSERT_MESSAGE( IsValidIndex( x ), "In-range pivot index" );
		BALL_ASSERT_MESSAGE( IsOccupied( x ), "Occupied pivot" );

		BALL_ASSERT_MESSAGE( !IsNil( y ), "Non-sentinel left child" );
		BALL_ASSERT_MESSAGE( IsValidIndex( y ), "In-range left child index" );
		BALL_ASSERT_MESSAGE( IsOccupied( y ), "Occupied left child" );

		// Move y's right subtree to x's left: all keys stay between y and x.
		SetLeft( x, RightOf( y ) );

		if ( !IsNil( RightOf( y ) ) )
			SetParent( RightOf( y ), x );

		const Index_t iParent = ParentOf( x );

		SetParent( y, iParent );

		// When x was the root, the SetParent above already made y root (parent == NIL); 
		// otherwise relink y under x's former parent on the correct side.
		if ( !IsNil( iParent ) )
		{
			if ( x == RightOf( iParent ) )
				SetRight( iParent, y );
			else
				SetLeft( iParent, y );
		}

		// Promote y above x; x becomes the right child of the new local root.
		SetRight( y, x );
		SetParent( x, y );
	}

	///-----------------------------------------------------------------------------
	/// @brief Restores RB invariants after BST insertion of red node @p z.
	/// 
	/// @details This follows the textbook three-case fix-up with the symmetric 
	/// branch obtained by swapping left/right: 
	/// A fresh node is inserted as red so every root->leaf path keeps the same 
	/// black-height. The only invariant that can break immediately is a red node 
	/// having a red parent.
	/// 
	/// Case 1: uncle is red. 
	/// Recolor parent + uncle to black and grandparent to red. This repairs the 
	/// local red-red conflict without changing the black-height below the 
	/// grandparent, but it may move the violation one level upward.
	/// 
	/// Case 2 + 3: uncle is black. 
	/// First rotate to convert the "inner" configuration into an "outer" one, 
	/// then rotate around the grandparent and swap colors between parent and 
	/// grandparent. That promotes the former parent to the subtree root and makes 
	/// both children black-compatible again.
	/// 
	/// @complexity O(log n): recoloring may bubble up the height; at most two rotations.
	///-----------------------------------------------------------------------------
	constexpr void InsertFixup( Index_t z )
	{
		BALL_ASSERT_MESSAGE( !IsNil( z ), "Non-sentinel node" );
		BALL_ASSERT_MESSAGE( IsValidIndex( z ), "In-range node index" );
		BALL_ASSERT_MESSAGE( IsOccupied( z ), "Occupied node" );

		// Fresh insertions are red, so only a red-parent/red-child violation 
		// needs repair. Black-height is unchanged at insertion time
		while ( ColorOf( ParentOf( z ) ) == Color_t::RED )
		{
			const Index_t iParent = ParentOf( z );
			const Index_t iGrandParent = ParentOf( iParent );

			BALL_ASSERT_MESSAGE( !IsNil( iParent ), "Red-parent case requires a non-sentinel parent" );
			BALL_ASSERT_MESSAGE( !IsNil( iGrandParent ), "Red-parent case requires a non-sentinel grandparent" );

			if ( iParent == LeftOf( iGrandParent ) )
			{
				const Index_t y = RightOf( iGrandParent );

				if ( ColorOf( y ) == Color_t::RED )
				{
					// Case 1: red uncle. 
					// Recolor locally and continue from grandparent because the 
					// red-red conflict has been pushed one level upward
					SetColor( iParent, Color_t::BLACK );
					SetColor( y, Color_t::BLACK );
					SetColor( iGrandParent, Color_t::RED );
					z = iGrandParent;
				}
				else
				{
					if ( z == RightOf( iParent ) )
					{
						// Case 2: inner child. 
						// Rotate to convert the zig-zag shape into the outer case
						z = iParent;
						RotateLeft( z );
					}

					// Case 3: outer child. 
					// Promote the parent and recolor so the local subtree root is 
					// black and the former grandparent becomes its red child
					SetColor( ParentOf( z ), Color_t::BLACK );
					SetColor( ParentOf( ParentOf( z ) ), Color_t::RED );
					RotateRight( ParentOf( ParentOf( z ) ) );
				}
			}
			else
			{
				const Index_t y = LeftOf( iGrandParent );

				if ( ColorOf( y ) == Color_t::RED )
				{
					// Mirror case 1: red uncle on the left
					SetColor( iParent, Color_t::BLACK );
					SetColor( y, Color_t::BLACK );
					SetColor( iGrandParent, Color_t::RED );
					z = iGrandParent;
				}
				else
				{
					if ( z == LeftOf( iParent ) )
					{
						// Mirror case 2: inner child
						z = iParent;
						RotateRight( z );
					}

					// Mirror case 3: outer child
					SetColor( ParentOf( z ), Color_t::BLACK );
					SetColor( ParentOf( ParentOf( z ) ), Color_t::RED );
					RotateLeft( ParentOf( ParentOf( z ) ) );
				}
			}
		}

		// The root must remain black even if recoloring bubbled a red node upward; the 
		// root cell makes this an O(1) lookup instead of a climb from z.
		SetColor( RootIndex(), Color_t::BLACK );
	}

	///-----------------------------------------------------------------------------
	/// @brief Restores RB invariants after erasing node @p x whose sibling is @p w.
	/// 
	/// @details This is the standard bottom-up delete fix-up over the "double 
	/// black" deficiency, using the textbook sibling-color cases. 
	/// Deletion is harder than insertion because removing a black node can reduce 
	/// the black-height of exactly one root->leaf path. The loop treats @p x as 
	/// carrying one "extra black" that must be pushed upward, absorbed by a red 
	/// node, or redistributed through rotations.
	/// 
	/// Red sibling case: 
	/// Rotate so the sibling becomes black and the parent becomes red. This does 
	/// not solve the deficit yet; it rewrites the neighborhood into one of the 
	/// black-sibling cases below.
	/// 
	/// Black sibling with two black children: 
	/// Recolor the sibling red and move the extra black to the parent.
	/// 
	/// Black sibling with at least one red outer child: 
	/// Rotate around the parent and recolor so the path through @p x gains one 
	/// black node back, eliminating the deficit locally.
	/// 
	/// @complexity O(log n): the deficit may propagate up the height; at most three 
	/// rotations, and each loop iteration is O(1).
	///-----------------------------------------------------------------------------
	constexpr void EraseFixup( Index_t x, Index_t xParent )
	{
		BALL_ASSERT_MESSAGE( IsNilOrValid( x ), "Sentinel or in-range node index" );
		BALL_ASSERT_MESSAGE( IsNilOrValid( xParent ), "Sentinel or in-range parent node index" );

		// If a black node was removed, one path can become "short" by one black. 
		// Treat x as carrying that extra black until it is absorbed or rotated away. 
		// x sits at the root exactly when it has no parent; while x is the virtual NIL that 
		// role falls to its tracked xParent. x never becomes NIL again inside the loop, so 
		// xParent is consulted only on the first iteration; resolving iParent once per step 
		// keeps each iteration O(1) and avoids a second ParentOf( x ) read in the condition.
		while ( ColorOf( x ) == Color_t::BLACK )
		{
			const Index_t iParent = IsNil( x ) ? xParent : ParentOf( x );

			// x reached the root: the loop's second terminating condition.
			if ( IsNil( iParent ) )
				break;

			BALL_ASSERT_MESSAGE( IsValidIndex( iParent ), "Delete fixup requires a parent while x is not root" );
			BALL_ASSERT_MESSAGE( IsOccupied( iParent ), "Current node parent is occupied" );

			if ( x == LeftOf( iParent ) )
			{
				Index_t w = RightOf( iParent );

				if ( ColorOf( w ) == Color_t::RED )
				{
					// Case 1: red sibling. 
					// Rotate to turn the configuration into one with a black sibling
					SetColor( w, Color_t::BLACK );
					SetColor( iParent, Color_t::RED );
					RotateLeft( iParent );
					w = RightOf( iParent );
				}

				if ( ColorOf( LeftOf( w ) ) == Color_t::BLACK && ColorOf( RightOf( w ) ) == Color_t::BLACK )
				{
					// Case 2: black sibling with two black children. 
					// Recolor sibling red and move the black deficit up to parent.
					SetColor( w, Color_t::RED );
					x = iParent;
				}
				else
				{
					if ( ColorOf( RightOf( w ) ) == Color_t::BLACK )
					{
						// Case 3: black sibling whose outer child is black but inner 
						// child is red. Rotate sibling first to obtain case 4
						SetColor( LeftOf( w ), Color_t::BLACK );
						SetColor( w, Color_t::RED );
						RotateRight( w );
						w = RightOf( iParent );
					}

					// Case 4: black sibling with red outer child. One rotation around parent 
					// restores black-height locally and terminates the fix-up: break out 
					// instead of climbing to the root just to stop the loop.
					SetColor( w, ColorOf( iParent ) );
					SetColor( iParent, Color_t::BLACK );
					SetColor( RightOf( w ), Color_t::BLACK );
					RotateLeft( iParent );

					break;
				}
			}
			else
			{
				Index_t w = LeftOf( iParent );

				if ( ColorOf( w ) == Color_t::RED )
				{
					// Mirror case 1: red sibling.
					SetColor( w, Color_t::BLACK );
					SetColor( iParent, Color_t::RED );
					RotateRight( iParent );
					w = LeftOf( iParent );
				}

				if ( ColorOf( RightOf( w ) ) == Color_t::BLACK && ColorOf( LeftOf( w ) ) == Color_t::BLACK )
				{
					// Mirror case 2: black sibling with two black children
					SetColor( w, Color_t::RED );
					x = iParent;
				}
				else
				{
					if ( ColorOf( LeftOf( w ) ) == Color_t::BLACK )
					{
						// Mirror case 3: convert inner-red to outer-red
						SetColor( RightOf( w ), Color_t::BLACK );
						SetColor( w, Color_t::RED );
						RotateLeft( w );
						w = LeftOf( iParent );
					}

					// Mirror case 4: rotate around parent, absorb the deficit, then end the 
					// fix-up by breaking instead of climbing to the root.
					SetColor( w, ColorOf( iParent ) );
					SetColor( iParent, Color_t::BLACK );
					SetColor( LeftOf( w ), Color_t::BLACK );
					RotateRight( iParent );

					break;
				}
			}
		}

		// Any remaining extra black is discharged by painting x black (a no-op when the 
		// loop ended on case 4, where x is already black).
		SetColor( x, Color_t::BLACK );
	}

	///-----------------------------------------------------------------------------
	/// @brief Re-links one subtree in place of another without touching payloads.
	/// 
	/// @details Equivalent to CLRS `RB-TRANSPLANT`, except that `NIL_INDEX` 
	/// plays the role of the shared NIL sentinel.
	/// 
	/// @complexity O(1): a fixed number of link rewrites.
	///-----------------------------------------------------------------------------
	constexpr void Transplant( Index_t u, Index_t v )
	{
		BALL_ASSERT_MESSAGE( !IsNil( u ), "Non-sentinel source node" );
		BALL_ASSERT_MESSAGE( IsValidIndex( u ), "In-range source node index" );
		BALL_ASSERT_MESSAGE( IsOccupied( u ), "Occupied source node" );
		BALL_ASSERT_MESSAGE( IsNilOrValid( v ), "Sentinel or in-range replacement node index" );
		BALL_ASSERT_MESSAGE( IsNilOrOccupied( v ), "Replacement must be sentinel or occupied" );

		const Index_t iParent = ParentOf( u );

		// When u was the root, the SetParent below makes v root (parent == NIL); 
		// otherwise relink v under u's former parent on the correct side.
		if ( !IsNil( iParent ) )
		{
			if ( u == LeftOf( iParent ) )
				SetLeft( iParent, v );
			else
				SetRight( iParent, v );
		}

		if ( !IsNil( v ) )
			SetParent( v, iParent );
	}

	///-----------------------------------------------------------------------------
	/// @brief Returns the leftmost occupied node in the subtree rooted at @p iNode.
	/// 
	/// @complexity O(log n): descend the left spine.
	///-----------------------------------------------------------------------------
	constexpr Index_t Minimum( Index_t iNode ) const noexcept
	{
		BALL_ASSERT_MESSAGE( !IsNil( iNode ), "Non-sentinel subtree root" );
		BALL_ASSERT_MESSAGE( IsValidIndex( iNode ), "In-range subtree root index" );
		BALL_ASSERT_MESSAGE( IsOccupied( iNode ), "Occupied subtree root" );

		while ( !IsNil( LeftOf( iNode ) ) )
			iNode = LeftOf( iNode );

		return iNode;
	}

	///-----------------------------------------------------------------------------
	/// @brief Returns the rightmost occupied node in the subtree rooted at @p iNode.
	/// 
	/// @complexity O(log n): descend the right spine.
	///-----------------------------------------------------------------------------
	constexpr Index_t Maximum( Index_t iNode ) const noexcept
	{
		BALL_ASSERT_MESSAGE( !IsNil( iNode ), "Non-sentinel subtree root" );
		BALL_ASSERT_MESSAGE( IsValidIndex( iNode ), "In-range subtree root index" );
		BALL_ASSERT_MESSAGE( IsOccupied( iNode ), "Occupied subtree root" );

		while ( !IsNil( RightOf( iNode ) ) )
			iNode = RightOf( iNode );

		return iNode;
	}

	///-----------------------------------------------------------------------------
	/// @brief Returns the inorder successor of @p iNode or `NIL_INDEX` at end.
	/// 
	/// @complexity O(log n) worst case, O(1) amortized over a full inorder traversal.
	///-----------------------------------------------------------------------------
	constexpr Index_t Successor( Index_t iNode ) const noexcept
	{
		BALL_ASSERT_MESSAGE( !IsNil( iNode ), "Non-sentinel node" );
		BALL_ASSERT_MESSAGE( IsValidIndex( iNode ), "Requires an in-range node index" );
		BALL_ASSERT_MESSAGE( IsOccupied( iNode ), "Requires an occupied node" );

		if ( !IsNil( RightOf( iNode ) ) )
			return Minimum( RightOf( iNode ) );

		Index_t iParent = ParentOf( iNode );

		while ( !IsNil( iParent ) && iNode == RightOf( iParent ) )
		{
			iNode = iParent;
			iParent = ParentOf( iParent );
		}

		return iParent;
	}

	///-----------------------------------------------------------------------------
	/// @brief Returns the inorder predecessor of @p iNode or the rightmost node for `end()`.
	/// 
	/// @complexity O(log n) worst case, O(1) amortized over a full reverse traversal.
	///-----------------------------------------------------------------------------
	constexpr Index_t Predecessor( Index_t iNode ) const noexcept
	{
		BALL_ASSERT_MESSAGE( !IsNil( iNode ), "Non-sentinel node" );
		BALL_ASSERT_MESSAGE( IsValidIndex( iNode ), "Requires an in-range node index" );
		BALL_ASSERT_MESSAGE( IsOccupied( iNode ), "Requires an occupied node" );

		if ( !IsNil( LeftOf( iNode ) ) )
			return Maximum( LeftOf( iNode ) );

		Index_t iParent = ParentOf( iNode );

		while ( !IsNil( iParent ) && iNode == LeftOf( iParent ) )
		{
			iNode = iParent;
			iParent = ParentOf( iParent );
		}

		return iParent;
	}

	///-----------------------------------------------------------------------------
	/// @brief Rebuilds sentinel slot invariants after any structural mutation.
	/// 
	/// @details The sentinel is the shared NIL leaf and also the public `end()` 
	/// iterator state. Keeping it permanently black with self-links lets the tree 
	/// balancing code treat "missing child" like an ordinary black leaf, which 
	/// removes many null special-cases from insert/delete fix-up logic. Boundary 
	/// nodes are no longer cached: LeftmostIndex()/RightmostIndex() derive them.
	/// 
	/// @complexity Both overloads are O(1): the no-arg form resolves the root from the 
	/// encoded root cell, not a climb.
	///-----------------------------------------------------------------------------
	// Normalize from an already-resolved root (NIL when the tree is empty), so a caller 
	// that climbed to the root after a mutation does not trigger another slot scan.
	constexpr bool NormalizeAfterMutation( Index_t iRoot ) noexcept
	{
		if ( IsNil( iRoot ) )
			return false;

		SetParent( iRoot, NIL_INDEX );
		SetColor( iRoot, Color_t::BLACK );

		return true;
	}

	constexpr bool NormalizeAfterMutation() noexcept { return NormalizeAfterMutation( RootIndex() ); }

	/// @complexity O(k) where k is the subtree size: recursively visits every node once. 
	/// At the root this is O(n) and underlies Validate.
	constexpr bool ValidateNode( Index_t iNode, Index_t iExpectedParent, const Key_t *pMin, const Key_t *pMax, CVector< I, bool > &vecVisited, Index_t &iPrev, I &nVisited, I &nBlackHeight ) const
	{
		if ( IsNil( iNode ) )
		{
			nBlackHeight = 1;

			return true;
		}

		if ( iNode >= TreeCount() )
			return false;

		if ( vecVisited[ iNode ] )
			return false;

		vecVisited.PackedSetValue( iNode, true );

		if ( ParentOf( iNode ) != iExpectedParent )
			return false;

		const Index_t iLeft = LeftOf( iNode );
		const Index_t iRight = RightOf( iNode );

		if ( ( !IsNil( iLeft ) && iLeft >= TreeCount() ) || ( !IsNil( iRight ) && iRight >= TreeCount() ) )
			return false;

		if ( !IsNil( iLeft ) && ParentOf( iLeft ) != iNode )
			return false;

		if ( !IsNil( iRight ) && ParentOf( iRight ) != iNode )
			return false;

		const Key_t &key = Key( iNode );

		if ( pMin && !Less( *pMin, key ) )
			return false;

		if ( pMax && !Less( key, *pMax ) )
			return false;

		if ( ColorOf( iNode ) == Color_t::RED )
		{
			if ( ColorOf( iLeft ) != Color_t::BLACK || ColorOf( iRight ) != Color_t::BLACK )
				return false;
		}

		I nLeftBlackHeight = 0;
		I nRightBlackHeight = 0;

		if ( !ValidateNode( iLeft, iNode, pMin, &key, vecVisited, iPrev, nVisited, nLeftBlackHeight ) )
			return false;

		if ( !IsNil( iPrev ) && !Less( Key( iPrev ), key ) )
			return false;

		iPrev = iNode;
		++nVisited;

		if ( !ValidateNode( iRight, iNode, &key, pMax, vecVisited, iPrev, nVisited, nRightBlackHeight ) )
			return false;

		if ( nLeftBlackHeight != nRightBlackHeight )
			return false;

		nBlackHeight = nLeftBlackHeight + ( ColorOf( iNode ) == Color_t::BLACK );

		return true;
	}

	/// LeftOf/RightOf/ParentOf are only ever asked about real nodes: a NIL child is fed 
	/// to ColorOf (which keeps the virtual-black rule), never to these link readers. The 
	/// in-range assert stands in for the dropped "not NIL" runtime branch.
	/// 
	/// @complexity LeftOf/RightOf/ParentOf/ColorOf and SetLeft/SetRight/SetParent/SetColor 
	/// are all O(1) single-column reads or writes; ParentOf/SetParent add one read of the 
	/// always-hot root cell (the parent cell of slot FIRST_INDEX).
	constexpr Index_t LeftOf( Index_t iNode ) const noexcept
	{
		BALL_ASSERT_MESSAGE( IsValidIndex( iNode ), "Requires an in-range non-sentinel node index" );

		return LeftColumn()[ iNode ];
	}

	constexpr Index_t RightOf( Index_t iNode ) const noexcept
	{
		BALL_ASSERT_MESSAGE( IsValidIndex( iNode ), "Requires an in-range non-sentinel node index" );

		return RightColumn()[ iNode ];
	}

	/// Parent links live in the parent column with two repurposed cells (see RootIndex): 
	/// the cell of slot FIRST_INDEX always stores the encoded root pointer `~R`, and the 
	/// cell of the root R stores slot 0's displaced real parent. Every other cell holds 
	/// its own node's parent verbatim, so the remap below is two compares plus one read 
	/// of the always-cached root cell -- no climbs and no relocations anywhere.
	constexpr Index_t ParentOf( Index_t iNode ) const noexcept
	{
		BALL_ASSERT_MESSAGE( IsValidIndex( iNode ), "Requires an in-range non-sentinel node index" );

		const ParentColumn_t *pParents = ParentColumn();
		const Index_t iRoot = static_cast< Index_t >( ~static_cast< Index_t >( pParents[ FIRST_INDEX ] ) );

		// The root has no parent by definition; slot 0 reads its displaced parent from 
		// the root's cell.
		if ( iNode == iRoot )
			return NIL_INDEX;

		return pParents[ IsFirst( iNode ) ? iRoot : iNode ];
	}

	// ColorOf keeps the NIL runtime branch (unlike the link readers above): the fix-up 
	// logic reads the colour of NIL children constantly and relies on NIL being black.
	constexpr Color_t ColorOf( Index_t iNode ) const noexcept
	{
		if ( IsNil( iNode ) )
			return Color_t::BLACK;

		BALL_ASSERT_MESSAGE( IsValidIndex( iNode ), "Requires an in-range node index" );

		return Get< TagColumn_t >( Nodes(), iNode );
	}

	constexpr void SetLeft( Index_t iNode, Index_t iLeft ) noexcept
	{
		BALL_ASSERT_MESSAGE( IsValidIndex( iNode ), "Requires an in-range node index" );
		BALL_ASSERT_MESSAGE( IsNilOrValid( iLeft ), "Child must be sentinel or an in-range node index" );

		LeftColumn()[ iNode ] = iLeft;
	}

	constexpr void SetRight( Index_t iNode, Index_t iRight ) noexcept
	{
		BALL_ASSERT_MESSAGE( IsValidIndex( iNode ), "Requires an in-range node index" );
		BALL_ASSERT_MESSAGE( IsNilOrValid( iRight ), "Child must be sentinel or an in-range node index" );

		RightColumn()[ iNode ] = iRight;
	}

	/// Writing NIL declares @p iNode the new root: the root pointer cell is re-encoded and 
	/// slot 0's displaced parent follows it from the old root's cell into the new root's 
	/// cell -- pure index bookkeeping, no element is constructed or moved. Every rotation 
	/// and transplant already promotes the new root (SetParent with NIL) before demoting 
	/// the old one, so a plain write never targets the current root (asserted below).
	constexpr void SetParent( Index_t iNode, Index_t iParent ) noexcept
	{
		BALL_ASSERT_MESSAGE( IsValidIndex( iNode ), "Requires an in-range non-sentinel node index" );
		BALL_ASSERT_MESSAGE( IsNilOrValid( iParent ), "Parent must be sentinel or an in-range node index" );

		ParentColumn_t *pParents = ParentColumn();
		const Index_t iRoot = static_cast< Index_t >( ~static_cast< Index_t >( pParents[ FIRST_INDEX ] ) );

		if ( IsNil( iParent ) )
		{
			// Root transfer to iNode (a no-op when it already is the root).
			if ( iNode == iRoot )
				return;

			// Slot 0's displaced parent moves from the old root's cell to the new root's 
			// cell; while the old root was slot 0 itself there is nothing displaced yet.
			const Index_t iFirstParent = IsFirst( iRoot ) ? NIL_INDEX : static_cast< Index_t >( pParents[ iRoot ] );

			// The old root's cell reverts to holding its own parent: transiently NIL until 
			// the mutation relinks it (or drops it, when the old root is being removed).
			if ( !IsFirst( iRoot ) )
				pParents[ iRoot ] = NIL_INDEX;

			pParents[ FIRST_INDEX ] = static_cast< Index_t >( ~iNode );

			if ( !IsFirst( iNode ) )
				pParents[ iNode ] = iFirstParent;

			return;
		}

		BALL_ASSERT_MESSAGE( iNode != iRoot, "Demoting the root requires a prior root transfer (SetParent with NIL)" );

		pParents[ IsFirst( iNode ) ? iRoot : iNode ] = iParent;
	}

	// SetColor keeps the NIL no-op (unlike SetParent above): EraseFixup's final 
	// SetColor( x, BLACK ) is reached with x == NIL when the last node is removed.
	constexpr void SetColor( Index_t iNode, Color_t eColor ) noexcept
	{
		if ( IsNil( iNode ) )
		{
			BALL_ASSERT_MESSAGE( eColor == Color_t::BLACK, "NIL is permanently black" );

			return;
		}

		BALL_ASSERT_MESSAGE( IsValidIndex( iNode ), "SetColor requires an in-range node index" );

		Get< TagColumn_t >( Nodes(), iNode ) = eColor;
	}

	// Nothing is cached and no extra memory is spent: the node storage itself is the 
	// protected base (Nodes()), the comparator is an empty base reached via Comparator(), 
	// and the root pointer lives inside the existing parent column -- slot 0's parent cell 
	// stores ~R while slot 0's real parent is displaced into the root's own (otherwise 
	// NIL) cell. That makes RootIndex() O(1) with zero element relocations; compact-on- 
	// erase keeps storage dense so slot 0 always exists to host the encoding.
};

template < typename I, I N, typename TI, typename C, typename K, typename... Ts >
class CMultiRBTreeImpl : public CMultiRBTreeBase< I, N, TI, C, K, Ts... >
{
private:
	using Base_t = CMultiRBTreeBase< I, N, TI, C, K, Ts... >;
	using ColumnSequence_t = MakeIndexSequence_t< size_t, 1 + sizeof...( Ts ) >;

	using Base_t::AddNode;
	using Base_t::ColorOf;
	using Base_t::EraseFixup;
	using Base_t::Key;
	using Base_t::FindIndex;
	using Base_t::LeftColumn;
	using Base_t::LeftOf;
	using Base_t::Less;
	using Base_t::LowerBoundIndex;
	using Base_t::Nodes;
	using Base_t::CompactAfterRemoval;
	using Base_t::Maximum;
	using Base_t::NormalizeAfterMutation;
	using Base_t::ParentColumn;
	using Base_t::ParentOf;
	using Base_t::RightOf;
	using Base_t::SetCount; using Base_t::SetColor; using Base_t::SetLeft; using Base_t::SetParent; using Base_t::SetRight;
	using Base_t::Predecessor;
	using Base_t::Successor;
	using Base_t::TagColumn;
	using Base_t::Transplant;
	using Base_t::Minimum;
	using Base_t::UpperBoundIndex;

public:
	using typename Base_t::Value_t;
	using typename Base_t::Key_t;
	using typename Base_t::Compare_t;
	using typename Base_t::Color_t;
	using typename Base_t::Index_t;
	using typename Base_t::iterator; using typename Base_t::const_iterator;
	using Base_t::Base_t;
	using Base_t::FIRST_INDEX; using Base_t::INVALID_INDEX; using Base_t::NIL_INDEX;
	using Base_t::Count;
	using Base_t::FirstIndex;
	using Base_t::NilIndex;
	using Base_t::TreeCount;
	using Base_t::Validate;
	using Base_t::LeftIndex;
	using Base_t::RightIndex;
	using Base_t::ParentIndex;
	using Base_t::RootIndex;
	using Base_t::IsOccupied;
	using Base_t::IsNil;
	using Base_t::Color;
	using Base_t::EndIndex;
	using Base_t::NextIndex;
	using Base_t::PrevIndex;
	using Base_t::Iterator;
	using Base_t::begin;
	using Base_t::end;
	using Base_t::cbegin;
	using Base_t::cend;
	using Base_t::Column;

	template < TI TN > using Column_t = typename Base_t::template Column_t< TN >;
	using Base_t::COLUMN_COUNT;

	/// @brief Per-column row access by column index, mirroring `CMultiVector::Get< TN >`.
	/// 
	/// @complexity O(1) (all Get overloads, by index or by column type).
	template < TI TN > constexpr Column_t< TN > &Get( Index_t iNode ) { return Base_t::template Column< TN >( iNode ); }
	template < TI TN > constexpr const Column_t< TN > &Get( Index_t iNode ) const { return Base_t::template Column< TN >( iNode ); }
	template < TI TN > constexpr Column_t< TN > &Get( iterator it ) { return Base_t::template Column< TN >( it.SlotIndex() ); }
	template < TI TN > constexpr const Column_t< TN > &Get( const_iterator it ) const { return Base_t::template Column< TN >( it.SlotIndex() ); }

	/// @brief Per-column row access by column type (requires a unique column type).
	template < typename T > constexpr T &Get( Index_t iNode ) { return this->template At< T >( iNode ); }
	template < typename T > constexpr const T &Get( Index_t iNode ) const { return this->template At< T >( iNode ); }
	template < typename T > constexpr T &Get( iterator it ) { return this->template At< T >( it.SlotIndex() ); }
	template < typename T > constexpr const T &Get( const_iterator it ) const { return this->template At< T >( it.SlotIndex() ); }

private:
	/// @complexity O(log n): a single Insert of one source row (CopyRowFrom and MoveRowFrom).
	template < typename Other, size_t... Is >
	constexpr void CopyRowFrom( const Other &other, Index_t i, MIndexSequence< size_t, Is... > )
	{
		Insert( other.template Get< Is >( i )... );
	}

	template < typename Other, size_t... Is >
	constexpr void MoveRowFrom( Other &other, Index_t i, MIndexSequence< size_t, Is... > )
	{
		Insert( Move( other.template Get< Is >( i ) )... );
	}

public:
	/// @complexity O(m + n log n): clear m current rows, then insert the n source rows.
	template < I ON >
	constexpr CMultiRBTreeImpl &CopyFrom( const CMultiRBTreeImpl< I, ON, TI, C, K, Ts... > &other )
	{
		if ( reinterpret_cast< const void * >( this ) == reinterpret_cast< const void * >( &other ) )
			return *this;

		Base_t::Comparator() = other.Comparator();

		// Reduce to empty storage (FIRST_INDEX == 0). Storage is dense, so the shrink 
		// destructs every live row directly; root/boundary values derive from storage and 
		// reset with it.
		SetCount( FIRST_INDEX );

		for ( Index_t i = other.FirstIndex(); i != other.EndIndex(); i = other.NextIndex( i ) )
			CopyRowFrom( other, i, ColumnSequence_t() );

		return *this;
	}

	/// @complexity O(m + n log n): clear m current rows, then re-insert the n source rows 
	/// (this is a structural rebuild, not a pointer steal, so it is not O(1)).
	template < I ON >
	constexpr CMultiRBTreeImpl &MoveFrom( CMultiRBTreeImpl< I, ON, TI, C, K, Ts... > &&other )
	{
		if ( reinterpret_cast< const void * >( this ) == reinterpret_cast< const void * >( &other ) )
			return *this;

		Base_t::Comparator() = Move( other.Comparator() );

		// Reduce to empty storage (FIRST_INDEX == 0). Storage is dense, so the shrink 
		// destructs every live row directly; root/boundary values derive from storage and 
		// reset with it.
		SetCount( FIRST_INDEX );

		for ( Index_t i = other.FirstIndex(); i != other.EndIndex(); )
		{
			const Index_t iNext = other.NextIndex( i );

			MoveRowFrom( other, i, ColumnSequence_t() );
			i = iNext;
		}

		other.Clear();

		return *this;
	}

	/// @complexity O(m + n log n): the copy/move constructors and assignment operators all 
	/// delegate to CopyFrom/MoveFrom, which rebuild the tree row by row.
	template < I ON > constexpr CMultiRBTreeImpl( const CMultiRBTreeImpl< I, ON, TI, C, K, Ts... > &other ) { CopyFrom( other ); }
	template < I ON > constexpr CMultiRBTreeImpl &operator=( const CMultiRBTreeImpl< I, ON, TI, C, K, Ts... > &other ) { return CopyFrom( other ); }
	template < I ON > constexpr CMultiRBTreeImpl( CMultiRBTreeImpl< I, ON, TI, C, K, Ts... > &&other ) { MoveFrom( Move( other ) ); }
	template < I ON > constexpr CMultiRBTreeImpl &operator=( CMultiRBTreeImpl< I, ON, TI, C, K, Ts... > &&other ) { return MoveFrom( Move( other ) ); }

	///-----------------------------------------------------------------------------
	/// @brief Public unique-key tree facade over `CMultiRBTreeBase`.
	/// 
	/// @details Insert performs a standard BST descent followed by red-black 
	/// fix-up; erase uses successor transplant plus delete fix-up. Search, 
	/// lower_bound, upper_bound, successor and predecessor remain logarithmic in 
	/// tree size because the underlying red-black height is O(log n): 
	/// The balancing strategy is the usual one: 
	/// insert tries to avoid black-height changes by adding a red node first, 
	/// while erase may need to repair a lost black level after the physical 
	/// removal/transplant step.
	///-----------------------------------------------------------------------------
	/// @complexity O(n): destruct every live row when shrinking storage to empty.
	constexpr void Clear()
	{
		// Drop every node back to empty storage (FIRST_INDEX == 0). Storage is dense, so the 
		// shrink destructs every live row directly; NIL is the out-of-band -1, so there is no 
		// sentinel row to retain and derived root/boundary values reset with the storage.
		SetCount( FIRST_INDEX );
	}

	/// @complexity O(log n): a BST descent to the insertion point plus an InsertFixup pass, 
	/// amortized over the appends that grow storage.
	constexpr Index_t Insert( K key, Ts... values )
	{
		// Phase 1: plain BST insertion by key. 
		// This chooses the unique search position but does not yet guarantee the 
		// red-black color invariants. Column 0 is the ordering key. On an empty tree 
		// the search starts at NIL and the first node fills slot 0 as the root.

		Index_t iParent = NIL_INDEX;
		Index_t iNode = RootIndex();
		bool bLinkLeft = false;

		while ( !IsNil( iNode ) )
		{
			iParent = iNode;

			const Key_t &nodeKey = Key( iNode );

			if ( Less( key, nodeKey ) )
			{
				bLinkLeft = true;
				iNode = LeftOf( iNode );
			}
			else if ( Less( nodeKey, key ) )
			{
				bLinkLeft = false;
				iNode = RightOf( iNode );
			}
			else
				return NIL_INDEX;
		}

		// New nodes start red to preserve the black-height of all leaf paths
		const Index_t z = AddNode( iParent, Move( key ), Move( values )... );

		// The first node is already the root: AddNode set its parent to NIL. 
		// Otherwise link it under the located parent on the search side. Boundary 
		// nodes are derived (LeftmostIndex/RightmostIndex), so nothing to update.
		if ( !IsNil( iParent ) )
		{
			if ( bLinkLeft )
				SetLeft( iParent, z );
			else
				SetRight( iParent, z );
		}

		// Phase 2: recolor / rotate until any local red-red violation is removed
		InsertFixup( z );

		return z;
	}
	constexpr iterator InsertIterator( K key, Ts... values ) { return Iterator( Insert( Move( key ), Move( values )... ) ); }

	/// @complexity O(log n) for Find/LowerBound/UpperBound/Contains and their iterator 
	/// variants -- each is a single root-to-leaf BST descent. InsertIterator is O(log n).
	constexpr Index_t Find( const Key_t &key ) const noexcept { return FindIndex( key ); }
	constexpr Index_t LowerBound( const Key_t &key ) const noexcept { return LowerBoundIndex( key ); }
	constexpr Index_t UpperBound( const Key_t &key ) const noexcept { return UpperBoundIndex( key ); }
	constexpr iterator FindIterator( const Key_t &key ) noexcept { return Iterator( Find( key ) ); }
	constexpr const_iterator FindIterator( const Key_t &key ) const noexcept { return Iterator( Find( key ) ); }
	constexpr iterator LowerBoundIterator( const Key_t &key ) noexcept { return Iterator( LowerBound( key ) ); }
	constexpr const_iterator LowerBoundIterator( const Key_t &key ) const noexcept { return Iterator( LowerBound( key ) ); }
	constexpr iterator UpperBoundIterator( const Key_t &key ) noexcept { return Iterator( UpperBound( key ) ); }
	constexpr const_iterator UpperBoundIterator( const Key_t &key ) const noexcept { return Iterator( UpperBound( key ) ); }
	constexpr bool Contains( const Key_t &key ) const noexcept { return !IsNil( Find( key ) ); }

	/// @complexity O(log n): a Find descent plus RemoveNode (both O(log n)).
	constexpr Index_t FindAndRemove( const Key_t &key )
	{
		const Index_t iNode = Find( key );

		if ( !IsNil( iNode ) )
			RemoveNode( iNode );

		return iNode;
	}

	/// @complexity O(log n): compute the successor, then RemoveNode (both O(log n)). The 
	/// iterator overload is likewise O(log n).
	constexpr Index_t Remove( Index_t iNode )
	{
		BALL_ASSERT_MESSAGE( !IsNil( iNode ), "Non-end node index" );
		BALL_ASSERT_MESSAGE( IsOccupied( iNode ), "Occupied node index" );

		const Index_t iNext = NextIndex( iNode );
		const Index_t iMovedFrom = RemoveNode( iNode );

		// Compaction may have relocated the successor's node from the old last slot into 
		// iNode; remap the cached next index onto its new home so the returned iterator 
		// stays valid.
		return ( !IsNil( iMovedFrom ) && iNext == iMovedFrom ) ? iNode : iNext;
	}
	constexpr iterator Remove( iterator it ) { return Iterator( Remove( it.SlotIndex() ) ); }

	/// @brief Performs the canonical left rotation around @p x using SoA columns.
	/// 
	/// @details Public-tree mirror of the base helper; kept here because member 
	/// lookup inside `CRBTree` currently binds to this overload set. 
	/// This rotation is selected by insertion/deletion fix-up when the violating 
	/// subtree must be re-rooted through the right child of @p x.
	/// 
	/// @complexity O(1): a fixed number of link rewrites.
	///-----------------------------------------------------------------------------
	constexpr void RotateLeft( Index_t x )
	{
		const Index_t y = RightOf( x );

		BALL_ASSERT_MESSAGE( !IsNil( x ), "Non-sentinel pivot" );
		BALL_ASSERT_MESSAGE( IsOccupied( x ), "Occupied pivot" );

		BALL_ASSERT_MESSAGE( !IsNil( y ), "Non-sentinel right child" );
		BALL_ASSERT_MESSAGE( IsOccupied( y ), "Occupied right child" );

		// Detach the middle subtree and move it under x before promoting y.
		SetRight( x, LeftOf( y ) );

		if ( !IsNil( LeftOf( y ) ) )
			SetParent( LeftOf( y ), x );

		const Index_t iParent = ParentOf( x );

		SetParent( y, iParent );

		// When x was the root, the SetParent above already made y root (parent == NIL); 
		// otherwise relink y under x's former parent on the correct side.
		if ( !IsNil( iParent ) )
		{
			if ( x == LeftOf( iParent ) )
				SetLeft( iParent, y );
			else
				SetRight( iParent, y );
		}

		// Finish the rotation by making x the left child of y.
		SetLeft( y, x );
		SetParent( x, y );
	}

	///-----------------------------------------------------------------------------
	/// @brief Performs the canonical right rotation around @p x using SoA columns.
	/// 
	/// @details Symmetric counterpart of `RotateLeft`; it preserves inorder 
	/// ordering and only rewires the local neighborhood around @p x. 
	/// This is the mirror-image primitive used when the violating subtree must be 
	/// re-rooted through the left child of @p x.
	/// 
	/// @complexity O(1): a fixed number of link rewrites.
	///-----------------------------------------------------------------------------
	constexpr void RotateRight( Index_t x )
	{
		const Index_t y = LeftOf( x );

		BALL_ASSERT_MESSAGE( !IsNil( x ), "Non-sentinel pivot" );
		BALL_ASSERT_MESSAGE( IsOccupied( x ), "Occupied pivot" );

		BALL_ASSERT_MESSAGE( !IsNil( y ), "Non-sentinel left child" );
		BALL_ASSERT_MESSAGE( IsOccupied( y ), "Occupied left child" );

		// Detach the middle subtree and move it under x before promoting y.
		SetLeft( x, RightOf( y ) );

		if ( !IsNil( RightOf( y ) ) )
			SetParent( RightOf( y ), x );

		const Index_t iParent = ParentOf( x );

		SetParent( y, iParent );

		// When x was the root, the SetParent above already made y root (parent == NIL); 
		// otherwise relink y under x's former parent on the correct side.
		if ( !IsNil( iParent ) )
		{
			if ( x == RightOf( iParent ) )
				SetRight( iParent, y );
			else
				SetLeft( iParent, y );
		}

		// Finish the rotation by making x the right child of y.
		SetRight( y, x );
		SetParent( x, y );
	}

	///-----------------------------------------------------------------------------
	/// @brief Restores RB invariants after BST insertion of red node @p z.
	/// 
	/// @details Standard red-black insertion fix-up over the parent/uncle/grand- 
	/// parent triad, with the second branch being the left/right mirror image. 
	/// The algorithm keeps black-height stable by never inserting a black node. 
	/// Rebalancing therefore focuses on removing a possible red parent + red 
	/// child pair. Recoloring pushes the problem upward; rotations terminate it.
	/// 
	/// @complexity O(log n): recoloring may bubble up the height; at most two rotations.
	///-----------------------------------------------------------------------------
	constexpr void InsertFixup( Index_t z )
	{
		BALL_ASSERT_MESSAGE( !IsNil( z ), "Non-sentinel node" );
		BALL_ASSERT_MESSAGE( IsOccupied( z ), "Occupied node" );

		// Insert starts red, so fix-up only needs to remove a red-red edge.
		while ( ColorOf( ParentOf( z ) ) == Color_t::RED )
		{
			const Index_t iParent = ParentOf( z );
			const Index_t iGrandParent = ParentOf( iParent );

			BALL_ASSERT_MESSAGE( !IsNil( iParent ), "Red-parent case requires a non-sentinel parent" );
			BALL_ASSERT_MESSAGE( !IsNil( iGrandParent ), "Red-parent case requires a non-sentinel grandparent" );

			if ( iParent == LeftOf( iGrandParent ) )
			{
				const Index_t y = RightOf( iGrandParent );

				if ( ColorOf( y ) == Color_t::RED )
				{
					// Case 1: recolor and continue upward.
					SetColor( iParent, Color_t::BLACK );
					SetColor( y, Color_t::BLACK );
					SetColor( iGrandParent, Color_t::RED );
					z = iGrandParent;
				}
				else
				{
					if ( z == RightOf( iParent ) )
					{
						// Case 2: rotate parent to expose the outer case.
						z = iParent;
						RotateLeft( z );
					}

					// Case 3: rotate grandparent and swap colors to terminate locally.
					SetColor( ParentOf( z ), Color_t::BLACK );
					SetColor( ParentOf( ParentOf( z ) ), Color_t::RED );
					RotateRight( ParentOf( ParentOf( z ) ) );
				}
			}
			else
			{
				const Index_t y = LeftOf( iGrandParent );

				if ( ColorOf( y ) == Color_t::RED )
				{
					// Mirror case 1.
					SetColor( iParent, Color_t::BLACK );
					SetColor( y, Color_t::BLACK );
					SetColor( iGrandParent, Color_t::RED );
					z = iGrandParent;
				}
				else
				{
					if ( z == LeftOf( iParent ) )
					{
						// Mirror case 2.
						z = iParent;
						RotateRight( z );
					}

					// Mirror case 3.
					SetColor( ParentOf( z ), Color_t::BLACK );
					SetColor( ParentOf( ParentOf( z ) ), Color_t::RED );
					RotateLeft( ParentOf( ParentOf( z ) ) );
				}
			}
		}

		// The fix-up may bubble to the top; the root is always forced black. The root 
		// cell makes this an O(1) lookup instead of a climb from z.
		SetColor( RootIndex(), Color_t::BLACK );
	}

	///-----------------------------------------------------------------------------
	/// @brief Erases an occupied node and repairs red-black invariants in-place.
	/// 
	/// @details This follows the standard transplant-with-successor deletion 
	/// scheme, then runs the bottom-up delete fix-up when a black node was 
	/// physically removed from the search tree. 
	/// If @p z has two children, the inorder successor is transplanted into its 
	/// place so the BST ordering stays valid while the actual removed node has at 
	/// most one non-sentinel child. That reduction is what makes the later 
	/// color-fixup manageable.
	/// 
	/// @complexity O(log n): successor lookup + transplant + EraseFixup + an O(1) 
	/// amortized compaction.
	///-----------------------------------------------------------------------------
	constexpr Index_t RemoveNode( Index_t z )
	{
		BALL_ASSERT_MESSAGE( !IsNil( z ), "Non-sentinel node" );
		BALL_ASSERT_MESSAGE( IsOccupied( z ), "Occupied node" );

		Index_t y = z;
		Color_t yOriginalColor = ColorOf( y );
		Index_t x = NIL_INDEX;
		Index_t xParent = NIL_INDEX;

		if ( IsNil( LeftOf( z ) ) )
		{
			// Zero or one child on the right: splice z out directly
			x = RightOf( z );
			xParent = ParentOf( z );
			Transplant( z, RightOf( z ) );
		}
		else if ( IsNil( RightOf( z ) ) )
		{
			// Zero or one child on the left: splice z out directly
			x = LeftOf( z );
			xParent = ParentOf( z );
			Transplant( z, LeftOf( z ) );
		}
		else
		{
			// Two children: swap the structural position with the inorder 
			// successor, so the physically removed node has at most one child
			y = Minimum( RightOf( z ) );
			yOriginalColor = ColorOf( y );
			x = RightOf( y );

			if ( ParentOf( y ) == z )
			{
				// Successor is z's direct child, so x now hangs under y
				xParent = y;

				if ( !IsNil( x ) )
					SetParent( x, y );
			}
			else
			{
				// First detach successor from its old position
				xParent = ParentOf( y );
				Transplant( y, RightOf( y ) );
				SetRight( y, RightOf( z ) );
				SetParent( RightOf( y ), y );
			}

			// Then move successor into z's former position
			Transplant( z, y );
			SetLeft( y, LeftOf( z ) );
			SetParent( LeftOf( y ), y );
			SetColor( y, ColorOf( z ) );
		}

		// Only removing a black node can break black-height equivalence. Run the fix-up 
		// while z is spliced out but still physically present; nothing references its slot, 
		// and its payload is released later by the compaction step.
		if ( yOriginalColor == Color_t::BLACK )
			EraseFixup( x, xParent );

		// The root cell tracked every transplant/rotation above, so the post-splice root 
		// is an O(1) read -- no climb from an anchor node. When the last node was removed 
		// the stale self-root normalizes to a harmless no-op before the row is dropped.
		NormalizeAfterMutation( RootIndex() );

		// Physically drop z by compacting the last node into its slot so storage stays 
		// dense. Returns the slot the relocated node came from (or NIL) so the caller can 
		// remap any index it still holds onto the moved node's new home.
		return CompactAfterRemoval( z );
	}
};

template < typename I, I N, typename K, typename C, typename... Ts > class CBufferMultiRBTree;

///-----------------------------------------------------------------------------
/// @brief Heap-backed multi-column red-black tree: ordering key column @p K plus 
/// value columns @p Ts (a set when @p Ts is empty, an SoA multi-map otherwise).
///-----------------------------------------------------------------------------
template < typename I = size32_t, typename C = CRBTreeLess<>, typename K = I, typename... Ts >
class CMultiRBTree : public CMultiRBTreeImpl< I, 0, size8_t, C, K, Ts... >
{
public:
	using Base_t = CMultiRBTreeImpl< I, 0, size8_t, C, K, Ts... >;
	using Base_t::Base_t;
	using Base_t::CopyFrom; using Base_t::MoveFrom;

	/// @complexity O(m + n log n): cross-capacity copy/move conversions rebuild the tree via 
	/// CopyFrom/MoveFrom.
	template < I N > constexpr CMultiRBTree( const CBufferMultiRBTree< I, N, C, K, Ts... > &other ) { CopyFrom( other ); }
	template < I N > constexpr CMultiRBTree &operator=( const CBufferMultiRBTree< I, N, C, K, Ts... > &other ) { CopyFrom( other ); return *this; }
	template < I N > constexpr CMultiRBTree( CBufferMultiRBTree< I, N, C, K, Ts... > &&other ) { MoveFrom( Move( other ) ); }
	template < I N > constexpr CMultiRBTree &operator=( CBufferMultiRBTree< I, N, C, K, Ts... > &&other ) { MoveFrom( Move( other ) ); return *this; }
};

/// @brief Fixed-capacity (inline buffer) counterpart of `CMultiRBTree`.
template < typename I, I N, typename C = CRBTreeLess<>, typename K = I, typename... Ts >
class CBufferMultiRBTree : public CMultiRBTreeImpl< I, N, size8_t, C, K, Ts... >
{
public:
	using Base_t = CMultiRBTreeImpl< I, N, size8_t, C, K, Ts... >;
	using Base_t::Base_t;
	using Base_t::CopyFrom; using Base_t::MoveFrom;

	/// @complexity O(m + n log n): cross-form copy/move conversions rebuild via CopyFrom/MoveFrom.
	constexpr CBufferMultiRBTree( const CMultiRBTree< I, C, K, Ts... > &other ) { CopyFrom( other ); }
	constexpr CBufferMultiRBTree &operator=( const CMultiRBTree< I, C, K, Ts... > &other ) { CopyFrom( other ); return *this; }
	constexpr CBufferMultiRBTree( CMultiRBTree< I, C, K, Ts... > &&other ) { MoveFrom( Move( other ) ); }
	constexpr CBufferMultiRBTree &operator=( CMultiRBTree< I, C, K, Ts... > &&other ) { MoveFrom( Move( other ) ); return *this; }
};

///-----------------------------------------------------------------------------
/// @brief Single-value map: ordering key @p K and value @p V. A thin spelling of 
/// the multi-column core with exactly one value column. @p N is the inline-buffer 
/// capacity: `0` (the default) is heap-backed, a positive value keeps up to @p N 
/// nodes inline before spilling to the heap.
///-----------------------------------------------------------------------------
template < typename I = size32_t, typename K = I, typename V = K, typename C = CRBTreeLess< K >, I N = 0 >
class CRBTree : public CMultiRBTreeImpl< I, N, size8_t, C, K, V >
{
public:
	using Base_t = CMultiRBTreeImpl< I, N, size8_t, C, K, V >;
	using Base_t::Base_t;
	using Base_t::Get;
	using Base_t::CopyFrom; using Base_t::MoveFrom;

	using typename Base_t::Index_t;
	using typename Base_t::Key_t;
	using typename Base_t::iterator; using typename Base_t::const_iterator;

	/// @brief Accesses the ordering key (column 0) of a node by index or iterator.
	/// 
	/// @complexity O(1) (all Key and Value accessors).
	constexpr Key_t &Key( Index_t iNode ) { return Base_t::template Get< Key_t >( iNode ); }
	constexpr const Key_t &Key( Index_t iNode ) const { return Base_t::template Get< Key_t >( iNode ); }
	constexpr Key_t &Key( iterator it ) { return Base_t::template Get< Key_t >( it ); }
	constexpr const Key_t &Key( const_iterator it ) const { return Base_t::template Get< Key_t >( it ); }

	/// @brief Accesses the mapped value (column 1) of a node by index or iterator.
	constexpr V &Value( Index_t iNode ) { return Base_t::template Get< V >( iNode ); }
	constexpr const V &Value( Index_t iNode ) const { return Base_t::template Get< V >( iNode ); }
	constexpr V &Value( iterator it ) { return Base_t::template Get< V >( it ); }
	constexpr const V &Value( const_iterator it ) const { return Base_t::template Get< V >( it ); }

	/// Cross-capacity conversions (e.g. heap <-> inline buffer of any size).
	/// 
	/// @complexity O(m + n log n): each rebuilds the tree via CopyFrom/MoveFrom.
	template < I ON > constexpr CRBTree( const CRBTree< I, K, V, C, ON > &other ) { CopyFrom( other ); }
	template < I ON > constexpr CRBTree &operator=( const CRBTree< I, K, V, C, ON > &other ) { CopyFrom( other ); return *this; }
	template < I ON > constexpr CRBTree( CRBTree< I, K, V, C, ON > &&other ) { MoveFrom( Move( other ) ); }
	template < I ON > constexpr CRBTree &operator=( CRBTree< I, K, V, C, ON > &&other ) { MoveFrom( Move( other ) ); return *this; }
};

/// Convenience spellings with the comparator fixed to `CRBTreeLess< K >` (use the 
/// classes directly for a custom comparator).
/// 
/// Single-value map: key @p K + value @p V.
template < typename I = size32_t, typename K = I, typename V = K > using RBTree_t = CRBTree< I, K, V, CRBTreeLess< K > >;
template < typename K, typename V = K > using RBTree8_t = CRBTree< size8_t, K, V, CRBTreeLess< K > >;
template < typename K, typename V = K > using RBTree16_t = CRBTree< size16_t, K, V, CRBTreeLess< K > >;
template < typename K, typename V = K > using RBTree32_t = CRBTree< size32_t, K, V, CRBTreeLess< K > >;
template < typename K, typename V = K > using RBTree64_t = CRBTree< size64_t, K, V, CRBTreeLess< K > >;

template < size_t N, typename K = size_t, typename V = K > using BufferRBTree_t = CRBTree< size_t, K, V, CRBTreeLess< K >, N >;
template < size8_t N, typename K = size8_t, typename V = K > using BufferRBTree8_t = CRBTree< size8_t, K, V, CRBTreeLess< K >, N >;
template < size16_t N, typename K = size16_t, typename V = K > using BufferRBTree16_t = CRBTree< size16_t, K, V, CRBTreeLess< K >, N >;
template < size32_t N, typename K = size32_t, typename V = K > using BufferRBTree32_t = CRBTree< size32_t, K, V, CRBTreeLess< K >, N >;
template < size64_t N, typename K = size64_t, typename V = K > using BufferRBTree64_t = CRBTree< size64_t, K, V, CRBTreeLess< K >, N >;

/// Multi-column: key @p K + value columns @p Ts (no @p Ts is a set).
template < typename K, typename... Ts > using MultiRBTree_t = CMultiRBTree< size32_t, CRBTreeLess< K >, K, Ts... >;
template < typename K, typename... Ts > using MultiRBTree8_t = CMultiRBTree< size8_t, CRBTreeLess< K >, K, Ts... >;
template < typename K, typename... Ts > using MultiRBTree16_t = CMultiRBTree< size16_t, CRBTreeLess< K >, K, Ts... >;
template < typename K, typename... Ts > using MultiRBTree32_t = CMultiRBTree< size32_t, CRBTreeLess< K >, K, Ts... >;
template < typename K, typename... Ts > using MultiRBTree64_t = CMultiRBTree< size64_t, CRBTreeLess< K >, K, Ts... >;

template < size_t N, typename K = size_t, typename... Ts > using BufferMultiRBTree_t = CBufferMultiRBTree< size_t, N, CRBTreeLess< K >, K, Ts... >;
template < size8_t N, typename K = size8_t, typename... Ts > using BufferMultiRBTree8_t = CBufferMultiRBTree< size8_t, N, CRBTreeLess< K >, K, Ts... >;
template < size16_t N, typename K = size16_t, typename... Ts > using BufferMultiRBTree16_t = CBufferMultiRBTree< size16_t, N, CRBTreeLess< K >, K, Ts... >;
template < size32_t N, typename K = size32_t, typename... Ts > using BufferMultiRBTree32_t = CBufferMultiRBTree< size32_t, N, CRBTreeLess< K >, K, Ts... >;
template < size64_t N, typename K = size64_t, typename... Ts > using BufferMultiRBTree64_t = CBufferMultiRBTree< size64_t, N, CRBTreeLess< K >, K, Ts... >;

#endif // !defined( _INCLUDE_BALL_TYPES_RBTREE_HPP_ )
