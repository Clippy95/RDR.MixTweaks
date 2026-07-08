module;

#include "common.hxx"
#include <Windows.h>
#include <MemoryMgr.h>
#include <safetyhook.hpp>
#include <cstddef>
#include <cstring>
export module fixes;

import common;

import comvars;

static bool showme = true;

//static float GetFixedClothTaskDelta()
//{
//	constexpr float original_cloth_task_delta = 1.0f / 120.0f;
//
//	if (!rdr_frametime)
//		return original_cloth_task_delta;
//
//	float delta = *rdr_frametime * 0.25f;
//
//	if (delta < 0.0f)
//		return 0.0f;
//
//	if (delta > original_cloth_task_delta)
//		return original_cloth_task_delta;
//
//	return delta;
//}

class fixes
{
public:
	fixes()
	{
		MixTweaks::onInitEvent() += [] {
			CIniReader ini{};
			//static auto cloth_task_delta_pattern = hook::pattern("44 38 BF ? ? ? ? 0F 84 ? ? ? ? B8");
			//if (ini.ReadInteger("FIXES","FixClothSim",1) && !cloth_task_delta_pattern.empty())
			//{
			//	static auto cloth_task_delta_fix = safetyhook::create_mid(cloth_task_delta_pattern.get_first(), [](SafetyHookContext& ctx) {
			//		if (showme)
			//			ctx.xmm0.f32[0] = GetFixedClothTaskDelta();
			//		});
			//}

			};
	}
}fixes;
