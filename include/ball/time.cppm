module;

#include "types/base/arch/size.h"
#include "types/c/macros.h"

export module Ball.Time;

#define BALL_ENABLE_MODULE
#undef BALL_EXPORT
#define BALL_EXPORT export
#include "time.hpp"
