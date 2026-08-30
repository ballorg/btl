module;

#include <ball/types/memory.h>

export module Ball.Types:Core;

import Ball.New;
export namespace BTL
{
#include <ball/types/base.h>
#include <ball/types/meta.hpp>
#include <ball/types/memoryaligned.h>
#include <ball/types/meta/constevaluated.hpp>
#include <ball/types/meta/decay.hpp>
#include <ball/types/meta/indexsequence.hpp>
#include <ball/types/meta/indextype.hpp>
#include <ball/types/meta/isclass.hpp>
#include <ball/types/meta/isclassorunion.hpp>
#include <ball/types/meta/iscopyassignable.hpp>
#include <ball/types/meta/iscopyconstructible.hpp>
#include <ball/types/meta/isdefaultconstructible.hpp>
#include <ball/types/meta/ismemmovesafe.hpp>
#include <ball/types/meta/ismoveassignable.hpp>
#include <ball/types/meta/ismoveconstructible.hpp>
#include <ball/types/meta/isstandardlayout.hpp>
#include <ball/types/meta/istrivial.hpp>
#include <ball/types/meta/istriviallyconstructible.hpp>
#include <ball/types/meta/istriviallycopyable.hpp>
#include <ball/types/meta/istriviallydestructible.hpp>
#include <ball/types/meta/isunion.hpp>
#include <ball/types/meta/reflectcommon.hpp>
#include <ball/types/meta/reflectdescriptor.hpp>
#include <ball/types/meta/reflectfield.hpp>
#include <ball/types/meta/reflectforeach.hpp>
#include <ball/types/meta/reflecttraits.hpp>
#include <ball/types/meta/reflectvalue.hpp>
#include <ball/types/meta/return.hpp>
#include <ball/types/meta/size.hpp>
#include <ball/types/meta/tuple.hpp>
#include <ball/types/meta/typeinfo.hpp>
#include <ball/types/meta/variant.hpp>
}
