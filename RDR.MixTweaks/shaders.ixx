module;

#include "common.hxx"
#include <safetyhook.hpp>

export module shaders;

import common;

import comvars;

namespace
{
	rage::grcTexture* GetDrawRainDropsTextureSource(rage::grcTexture* originalSource)
	{
		auto* const postFx = rage::grPostFX::GetInstance();
		if (postFx && postFx->HudlessRT)
			return postFx->HudlessRT;

		auto* const textureFactory = rage::GetTextureFactory();
		if (!textureFactory)
			return originalSource;

		auto* const backBuffer0 = textureFactory->BackBufferColour_Surface_0;
		auto* const backBuffer1 = textureFactory->BackBufferColour_Surface_1;
		auto* const currentBackBuffer = textureFactory->GetBackBuffer(false);

		if (currentBackBuffer == backBuffer0 && backBuffer1)
			return backBuffer1;

		if (currentBackBuffer == backBuffer1 && backBuffer0)
			return backBuffer0;

		return backBuffer0 ? backBuffer0 : originalSource;
	}
}
export CMPatch falltrees_snow;
class shaders
{
public:

	shaders()
	{
		MixTweaks::onInitEvent() += []()
		{
				CIniReader ini{};
				if (ini.ReadInteger("FIXES", "FixRainDropletRefraction", 1) != 0)
				{
					// 0x1405C9154
					auto pattern = hook::pattern("33 D2 48 8B CB E8 ? ? ? ? 48 8B 0D ? ? ? ? E8");
					if (!pattern.empty())
					{
						static auto rain_droplet_hudless_prepare_hook = safetyhook::create_mid(
							pattern.get_first(),
							[](SafetyHookContext& ctx)
							{
								auto* const postFx = rage::grPostFX::GetInstance();
								if (postFx && rage::grPostFX::BuildHudlessRT)
									rage::grPostFX::BuildHudlessRT(postFx);
							});
					}
					// 
					pattern = hook::pattern("48 63 43 ? 48 85 C9 74 ? 0F B6 51 ? EB ? 32 D2 85 C0 74 ? ? ? ? 44 8D 50 ? 48 03 C0");
					if (!pattern.empty())
					{
						static auto rain_droplet_source_hook = safetyhook::create_mid(
							pattern.get_first(),
							[](SafetyHookContext& ctx)
							{
								ctx.rcx = reinterpret_cast<uintptr_t>(
									GetDrawRainDropsTextureSource(reinterpret_cast<rage::grcTexture*>(ctx.rcx)));
							});
					}
				}

				auto pattern = hook::pattern("0F 84 ? ? ? ? 44 0F 2F C6");

				if (!pattern.empty()) {
					falltrees_snow.AddRaw<uint8_t>(pattern.get_first(0), 0xE9);
					falltrees_snow.AddRaw<int32_t>(pattern.get_first(1), 0x931);
					falltrees_snow.AddRaw<uint8_t>(pattern.get_first(5), 0x90);
				}
				if (!pattern.empty() && ini.ReadInteger("GRAPHICS","RemoveSnowFromHeights",0))
				{
					falltrees_snow.Apply();
				}
				static CMPatch cutscene_patch{};
				cutscene_patch.Add<uint8_t>("C6 05 ? ? ? ? ? 74 ? C6 05 ? ? ? ? 00 48 8B 15", 0, 6);
				if (ini.ReadInteger("GRAPHICS", "DisableCutsceneBars", 1))
				{
					cutscene_patch.Apply();
				}

				pattern = hook::pattern("0F 2F 05 ? ? ? ? 0F 82 ? ? ? ? 0F 10 57");



				if (!pattern.empty())
				{
					auto displacement = resolve_displacement(pattern.get_first());
					if (displacement.has_value()) {
						auto offset = make_pattern_rip_relative_offset("F3 0F 5E 1D ? ? ? ? 74", displacement.value(), 0);
						if (offset.has_value())
						{
							static CMPatch dithering_workaround;
							pattern = hook::pattern("F3 0F 5E 1D ? ? ? ? 74");
							if (!pattern.empty()) {
								dithering_workaround.Add<int>((uintptr_t)pattern.get_first(4), offset.value());
								dithering_workaround.Apply();
							}
						}
					}

				}

		};
	}
}shaders;
