module;

#include "memory.h"

export module Ball.Types:Core;

import Ball.New;
export namespace BTL
{
#include "base.h"
#include "meta.hpp"
#include "memoryaligned.h"
#include "meta/constevaluated.hpp"
#include "meta/decay.hpp"
#include "meta/indexsequence.hpp"
#include "meta/indextype.hpp"
#include "meta/isclass.hpp"
#include "meta/isclassorunion.hpp"
#include "meta/iscopyassignable.hpp"
#include "meta/iscopyconstructible.hpp"
#include "meta/isdefaultconstructible.hpp"
#include "meta/ismemmovesafe.hpp"
#include "meta/ismoveassignable.hpp"
#include "meta/ismoveconstructible.hpp"
#include "meta/isstandardlayout.hpp"
#include "meta/istrivial.hpp"
#include "meta/istriviallyconstructible.hpp"
#include "meta/istriviallycopyable.hpp"
#include "meta/istriviallydestructible.hpp"
#include "meta/isunion.hpp"
#include "meta/reflectcommon.hpp"
#include "meta/reflectdescriptor.hpp"
#include "meta/reflectfield.hpp"
#include "meta/reflectforeach.hpp"
#include "meta/reflecttraits.hpp"
#include "meta/reflectvalue.hpp"
#include "meta/return.hpp"
#include "meta/size.hpp"
#include "meta/tuple.hpp"
#include "meta/typeinfo.hpp"
#include "meta/variant.hpp"
}
