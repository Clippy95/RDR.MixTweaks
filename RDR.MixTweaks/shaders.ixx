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
					static auto pattern = hook::pattern("33 D2 48 8B CB E8 ? ? ? ? 48 8B 0D ? ? ? ? E8");
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
		};
	}
}shaders;
