#ifndef _INCLUDE_BALL_TYPES_HASHMAP_H_
#	define _INCLUDE_BALL_TYPES_HASHMAP_H_

/// Iterates a hash map over occupied slots in storage order.
#	define BALL_HASHMAP_FOREACH( map, it ) \
		for ( auto it = ( map ).FirstIndex(); it != ( map ).EndIndex(); it = ( map ).NextIndex( it ) )

#endif // !defined( _INCLUDE_BALL_TYPES_HASHMAP_H_ )
