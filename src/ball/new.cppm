module;

#include <ball/types/base/arch/size.h>
#include <ball/types/c/macros.h>

export module Ball.New;

#define BALL_ENABLE_MODULE
#undef BALL_EXPORT
#define BALL_EXPORT export
#include "ball/new.hpp"
