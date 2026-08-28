#ifndef _INCLUDE_BALL_TYPES_RBTREE_H_
#	define _INCLUDE_BALL_TYPES_RBTREE_H_

/// Iterates a tree in ascending key order.
#	define BALL_RBTREE_FOREACH( tree, it ) \
		for ( auto it = ( tree ).FirstIndex(); it != ( tree ).EndIndex(); it = ( tree ).NextIndex( it ) )

/// Iterates a tree in descending key order.
#	define BALL_RBTREE_FOREACH_REVERSE( tree, it ) \
		for ( auto it = ( tree ).PrevIndex( ( tree ).EndIndex() ); it != ( tree ).EndIndex(); it = ( tree ).PrevIndex( it ) )

/// Iterates a tree in dense storage order.
#	define BALL_RBTREE_FOREACH_UNORDERED( tree, it ) \
		for ( auto it = ( tree ).FIRST_INDEX; it != ( tree ).Count(); ++it )

/// Iterates a tree in reverse dense storage order.
#	define BALL_RBTREE_FOREACH_UNORDERED_REVERSE( tree, it ) \
		for ( auto it = ( tree ).Count(); it-- != ( tree ).FIRST_INDEX; )

#endif // !defined( _INCLUDE_BALL_TYPES_RBTREE_H_ )
