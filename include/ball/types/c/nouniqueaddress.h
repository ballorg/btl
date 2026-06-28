#ifndef _INCLUDE_BALL_TYPES_C_NOUNIQUEADDRESS_H_
#	define _INCLUDE_BALL_TYPES_C_NOUNIQUEADDRESS_H_

// Empty-member elision: lets a potentially-empty member (e.g. a stateless comparator or
// allocator) occupy zero bytes, the way an empty base would under a working EBO. MSVC keeps
// the standard [[no_unique_address]] a no-op for ABI reasons and spells its own as
// [[msvc::no_unique_address]] (clang-cl follows MSVC here), so probe that first and fall
// back to the standard attribute, or to nothing when neither is available.
#	ifdef __has_cpp_attribute
#		if __has_cpp_attribute( msvc::no_unique_address )
#			define BALL_NO_UNIQUE_ADDRESS [[ msvc::no_unique_address ]]
#		elif __has_cpp_attribute( no_unique_address )
#			define BALL_NO_UNIQUE_ADDRESS [[ no_unique_address ]]
#		else
#			define BALL_NO_UNIQUE_ADDRESS
#		endif
#	else
#		define BALL_NO_UNIQUE_ADDRESS
#	endif

#endif // !defined( _INCLUDE_BALL_TYPES_C_NOUNIQUEADDRESS_H_ )
