#ifndef _INCLUDE_BALL_TYPES_META_VARIANT_HPP_
#	define _INCLUDE_BALL_TYPES_META_VARIANT_HPP_

#	pragma once

union Variant_t
{
	unsigned char m_Byte;
	unsigned short m_UShort;
	unsigned int m_UInt;
	unsigned long long int m_ULLInt;
	float m_Float;
	double m_Double;
	long double m_LDouble;
	void *m_Ptr;
	char *m_pCString;
	void ( *m_pFunction )();
};

#endif // !defined( _INCLUDE_BALL_TYPES_META_VARIANT_HPP_ )
