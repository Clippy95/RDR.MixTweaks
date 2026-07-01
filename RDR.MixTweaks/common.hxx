#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <subauth.h>
#include "IniReader.h"
#include "Hooking.Patterns.h"
#include "ModuleList.hpp"
#include <thread>
#include <mutex>
#include <set>
#include <map>
#include <iomanip>
#include <array>
#include <future>
#include <d3d9.h>
#include "maths.hxx"
#include <MemoryMgr.h>
#define GAME_FN(name, signature, ret, args, get_first_arg) \
    inline GamePatternFn<ret(__cdecl*) args> name{ signature, get_first_arg }

#define GAME_FN_DEFAULT(name, signature, ret, args) \
    inline GamePatternFn<ret(__cdecl*) args> name{ signature, 0 }