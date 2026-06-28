module;

#include "common.hxx"

export module comvars;

import common;

#define VALIDATE_SIZE(struc, size) static_assert(sizeof(struc) == size, "Invalid structure size of " #struc)
namespace rage
{

}
class Common
{
public:
	Common()
	{

	}
}Common;