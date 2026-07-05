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

SafetyHookInline rage_UIStringTable_GetStringD;
const char* __fastcall  hk_rage_UIStringTable_GetString(void* a1, __int64 a2)
{
	static auto snow = rage::atStringHash("GameOption_Snow");

	if (snow == a2)
	{
		return "Snow at high heights";
	}

	static auto mixtweaks = rage::atStringHash("OptionsHeader_Mix");
	if (mixtweaks == a2)
	{
		return "MIXTWEAKS";
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

			pattern = hook::pattern("48 85 C0 74 ? 40 38 B7");
			if (!pattern.empty())
			{
				static auto sync_to_options = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& ctx) {

					uintptr_t object = rage::UIFactory::GetUIObjectByIdName(*rage::UIFactory::sm_Instance, "GameOption_Snow");

					if (object)
					{
						vftable_call<void>((void*)object, 1128, !falltrees_snow.IsApplied(), 0);
					}

					printf("object %p !\n", object);

					});
			}

			pattern = hook::pattern("48 85 C0 74 ? ? ? ? 33 D2 4C 8B 81 ? ? ? ? 48 8B C8 41 FF D0 85 C0 0F 95 C0 88 83 ? ? ? ? 48 8B 5C 24");
			if (!pattern.empty())
			{
				static auto sync_from_options = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& ctx) {

					uintptr_t object = rage::UIFactory::GetUIObjectByIdName(*rage::UIFactory::sm_Instance, "GameOption_Snow");

					if (object)
					{
						auto result = vftable_call<int>((void*)object, 1040, 0);

						if (result == 0)
							falltrees_snow.Apply();
						else
							falltrees_snow.Restore();
						printf("from ugh %d\n", result);
					}

					printf("object from %p !\n", object);

					});
			}

		};

	};
}strtbl;