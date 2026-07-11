module;

#include "common.hxx"
#include <safetyhook.hpp>

export module shaders;

import common;

import comvars;
import settings;

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
SafetyHookInline GetSnowCoverageAtPointD1;
SafetyHookInline GetSnowCoverageAtPointD2;
float __fastcall GetSnowCoverageAtPoint(void* renLightPrePassLightsMgr)
{
	if(falltrees_snow.IsApplied())
	return 0.f;
	return GetSnowCoverageAtPointD1.unsafe_fastcall<float>(renLightPrePassLightsMgr);
}

float __fastcall GetSnowCoverageAtPoint2(void* unk,void* renLightPrePassLightsMgr)
{
	if (falltrees_snow.IsApplied())
		return 0.f;
	return GetSnowCoverageAtPointD2.unsafe_fastcall<float>(unk,renLightPrePassLightsMgr);
}

void DoASnowThing(float& value)
{
	if (falltrees_snow.IsApplied())
		value = 0.f;
}

static int __fastcall SyncSnowToOptions()
{
	return falltrees_snow.IsApplied();
}

static void __fastcall SyncSnowFromOptions(int value)
{
	if (value == 1)
		falltrees_snow.Apply();
	else
		falltrees_snow.Restore();
	CIniReader ini;
	ini.WriteInteger("GRAPHICS", "RemoveSnowFromHeights", falltrees_snow.IsApplied());
}

SafetyHookInline SetDofParmsD;
int8_t DOFoption = 0;
void __fastcall SetDofParms_hook(__int64 a1, int a2, float* vector4)
{
	if (DOFoption == 0 || (DOFoption == 2 && rage::CutsceneManagerIsCutscenePlaying()))
	{
		SetDofParmsD.unsafe_fastcall(a1, a2, vector4);
		return;
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
				bool first_empty = pattern.empty();

				pattern = hook::pattern("F3 0F 10 41 ? 0F 57 C9 F3 0F 5C 05");

				if (!pattern.empty())
					GetSnowCoverageAtPointD1 = safetyhook::create_inline(pattern.get_first(), GetSnowCoverageAtPoint);

				pattern = hook::pattern("F3 0F 10 42 ? 0F 57 C9 F3 0F 5C 05");

				if (!pattern.empty())
					GetSnowCoverageAtPointD2 = safetyhook::create_inline(pattern.get_first(), GetSnowCoverageAtPoint2);

				static auto snow_inlinemid1 = hooks::pattern_mid("F3 0F 5C FC 0F 28 C7", [](SafetyHookContext& ctx) {
					DoASnowThing(ctx.xmm4.f32[0]);
					});

				static auto snow_inlinemid2 = hooks::pattern_mid("F3 0F 5C DF 48 89 B4 24", [](SafetyHookContext& ctx) {
					DoASnowThing(ctx.xmm3.f32[0]);

					});

				static auto snow_inlinemid3 = hooks::pattern_mid("F3 0F 10 05 ? ? ? ? 48 8D 4D ? F3 0F 10 15 ? ? ? ? 33 DB", [](SafetyHookContext& ctx) {
					DoASnowThing(ctx.xmm9.f32[0]);

					});

				static auto snow_inlinemid4 = hooks::pattern_mid("F3 41 0F 5C C1 0F 2F C6 72 ? 41 0F 28 D1 EB", [](SafetyHookContext& ctx) {
					DoASnowThing(ctx.xmm0.f32[0]);

					});

				static auto snow_inlinemid5 = hooks::pattern_mid("4D 8B 99 ? ? ? ? 4D 85 DB 74 ? 4D 8B 9B ? ? ? ? 4D 85 DB 74 ? 45 0F B7 53 ? 41 8B D4 4D 8B C4 45 85 D2 74 ? ? ? ? 0F 1F 40 ? 8B C2", [](SafetyHookContext& ctx) {
					DoASnowThing(ctx.xmm0.f32[0]);

					});

				static auto snow_inlinemid6 = hooks::pattern_mid("F3 0F 5C 1D ? ? ? ? 0F 2F DF", [](SafetyHookContext& ctx) {
					DoASnowThing(ctx.xmm3.f32[0]);

					});

				static auto snow_inlinemid7 = hooks::pattern_mid("74 ? 84 D2 74 ? 40 B7", [](SafetyHookContext& ctx) {
					DoASnowThing(ctx.xmm2.f32[0]);
					});

				static auto snow_inlinemid8 = hooks::pattern_mid("F3 0F 10 0D ? ? ? ? 8B D3", [](SafetyHookContext& ctx) {
					DoASnowThing(ctx.xmm15.f32[0]);

					});

				static auto snow_inlinemid9 = hooks::pattern_mid("0F 5A C1 66 0F 2F 05 ? ? ? ? 76 ? 48 8B 42", [](SafetyHookContext& ctx) {
					DoASnowThing(ctx.xmm1.f32[0]);

					});

				if (!first_empty && ini.ReadInteger("GRAPHICS","RemoveSnowFromHeights",0))
				{
					falltrees_snow.Apply();
				}
				RDRSettings.AddOptionBool(
					"SnowTree",
					"Snow",
					"Remove snow",
					SyncSnowToOptions,
					SyncSnowFromOptions, RDRSettingsRegistry::OptionsList::Gameplay
				);

				static CMPatch cutscene_patch{};
				cutscene_patch.Add<uint8_t>("C6 05 ? ? ? ? ? 74 ? C6 05 ? ? ? ? 00 48 8B 15", 0, 6);
				if (ini.ReadInteger("GRAPHICS", "DisableCutsceneBars", 1))
				{
					cutscene_patch.Apply();
				}

				RDRSettings.AddOptionBool(
					"DisableCutsceneBars",
					"CutsceneBars",
					"Disable Cutscene Bars",
					[]() { int result = cutscene_patch.IsApplied();
				return result;
					},
					[](int value) {

						if (value == 1)
							cutscene_patch.Apply();
						else
							cutscene_patch.Restore();

						CIniReader ini;
						ini.WriteInteger("GRAPHICS", "DisableCutsceneBars", value);
					}, RDRSettingsRegistry::OptionsList::Display
				);

				static bool remove_snow_height_check = ini.ReadInteger("GRAPHICS","RemoveSnowHeightCheck",1);
				static auto show_snow = hooks::pattern_mid("0F 2F C6 0F 86 ? ? ? ? 33 C9", [](SafetyHookContext& ctx) {
					if(remove_snow_height_check)
					ctx.xmm0.f32[0] = 1.f;

					});

				static auto show_snow2 = hooks::pattern_mid("0F 2F C6 F3 0F 10 3D ? ? ? ? 0F 86", [](SafetyHookContext& ctx) {
					if(remove_snow_height_check)
					ctx.xmm0.f32[0] = 1.f;

					});

				static auto show_snow3 = hooks::pattern_mid("48 8B D1 44 8B 87 ? ? ? ? C7 44 24 ? ? ? ? ? 48 8B 49 ? F3 0F 11 BD", [](SafetyHookContext& ctx) {
					if(remove_snow_height_check)
					ctx.xmm0.f32[0] = 1.f;

					});

				RDRSettings.AddOptionBool(
					"RemoveSnowHeightCheck",
					"SnowHeightCheck",
					"Disable Snow height check",
					[]() { int result = remove_snow_height_check;
				return result;
					},
					[](int value) {

						remove_snow_height_check = value != 0;

						CIniReader ini;
						ini.WriteInteger("GRAPHICS", "RemoveSnowHeightCheck", value);
					}, RDRSettingsRegistry::OptionsList::Display
				);

				pattern = hook::pattern("0F 2F 05 ? ? ? ? 0F 82 ? ? ? ? 0F 10 57");


				if (ini.ReadInteger("GRAPHICS", "DitheringFixWorkAround", 1) != 0) {
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
									dithering_workaround.AddRaw<int>(pattern.get_first(4), offset.value());
									dithering_workaround.Apply();
								}
							}
						}

					}
				}

				DOFoption = std::clamp(ini.ReadInteger("GRAPHICS", "DepthOfField", 0),0,2);
				SetDofParmsD = hooks::pattern_inline("48 83 EC ? 48 63 EA 49 8B F8 48 8B D9 83 FD", SetDofParms_hook, -0xB);
				RDRSettings.AddOptionInt(
					RDRSettingsRegistry::OptionsList::Display,
					"DepthOfFieldTree",
					"DepthOfField",
					"Depth of Field",
					{ "Common_On", "Common_Off", "Common_DOFCutscene" },
					[]() -> int {
						return DOFoption;
					},
					[](int value) {
						DOFoption = value;
						CIniReader ini{};
						ini.WriteInteger("GRAPHICS", "DepthOfField", DOFoption);
					}
				);

		};
	}
}shaders;
