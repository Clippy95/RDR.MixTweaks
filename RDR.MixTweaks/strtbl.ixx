module;

#include "common.hxx"
#include <Windows.h>
#include <MemoryMgr.h>
#include <safetyhook.hpp>
#include <cstddef>
#include <cstring>
export module strtbl;

import common;

import comvars;
import shaders;
import settings;

SafetyHookInline rage_UIStringTable_GetStringD;
const char* __fastcall  hk_rage_UIStringTable_GetString(void* a1, __int64 a2)
{
	if (auto fallback = RDRSettings.FindFallbackString(static_cast<uint32_t>(a2)))
	{
		return fallback;
	}

	static auto mixtweaks = rage::atStringHash("OptionsHeader_Mix");
	if (mixtweaks == a2)
	{
		return "MIXTWEAKS";
	}

	static auto Common_DOFCutscene = rage::atStringHash("Common_DOFCutscene");
		if (Common_DOFCutscene == a2)
		{
			return "Cutscene only";
		}

	return rage_UIStringTable_GetStringD.unsafe_fastcall<const char*>(a1, a2);
}

class strtbl
{
public:
	strtbl() {

		MixTweaks::onInitEvent() += []() {
			auto pattern = hook::pattern("? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? ? ? ? ? ? ? 75 ? ? ? ? 75");
			if (!pattern.empty())
			{
				rage_UIStringTable_GetStringD = safetyhook::create_inline(pattern.get_first(-0xA), hk_rage_UIStringTable_GetString);
			}

		};

	};
}strtbl;
