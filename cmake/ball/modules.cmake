ball_generate_public_module(
	NAME Ball.New
	HEADER new.hpp
	GLOBAL_HEADERS types/base/arch/size.h types/c/macros.h
)

ball_generate_public_module(
	NAME Ball.Time
	HEADER time.hpp
	GLOBAL_HEADERS types/base/arch/size.h types/c/macros.h
)

ball_generate_public_module(
	NAME Ball.Types
	EXPORT_IMPORTS
		Ball.New
		:Meta
		:Bits
		:Elements
		:Fixed
		:Math
		:Number
		:Pair
		:Prefetch
		:Allocator
		:Array
		:ElementsPack
		:VectorIterator
		:SlotIterator
		:Reflect
		:ViewBase
		:View
		:StringView
		:Hash
		:Vector
		:String
		:RBTree
		:HashMap
		:Delegate
)
