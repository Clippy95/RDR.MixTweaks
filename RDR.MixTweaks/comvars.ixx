module;

#include "common.hxx"
#include <cstddef>

export module comvars;

import common;

#define VALIDATE_SIZE(struc, size) static_assert(sizeof(struc) == size, "Invalid structure size of " #struc)


class rdrPostFX;

uintptr_t game_free = NULL;

export class GameString
{
public:
	char text[16]{};
	size_t size{};
	size_t capacity{15};

	void Free()
	{
		if (capacity < 16)
			return;

		fastcall_call<void>(game_free, *reinterpret_cast<void**>(this));
	}
};

static_assert(offsetof(GameString, text) == 0x00);
static_assert(offsetof(GameString, size) == 0x10);
static_assert(offsetof(GameString, capacity) == 0x18);
static_assert(sizeof(GameString) == 0x20);

export namespace rage
{
	namespace UIFactory
	{
		GAME_FN(
			GetUIObjectByIdName_func,
			"? ? ? ? ? ? ? ? ? ? ? ? 0F 84 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 74 ? 0F 1F 40 ? 0F 1F 84 00",
			uintptr_t,
			(uintptr_t instance, const char* name),
			-0xA
		);

		uintptr_t GetUIObjectByIdName(uintptr_t instance, const char* name)
		{
			return GetUIObjectByIdName_func(instance, name);
		}

		uintptr_t* sm_Instance = nullptr;

	}


	GAME_FN(
		atPartialStringHash,
		"75 ? 33 C0 C3 ? ? ? 75 ? 0F B6 41",
		uint32_t,
		(const char* string, int starter),
		-6
	);

	__declspec(noinline) uint32_t atStringHash(const char* string, int starter = 0)
	{
		auto result = atPartialStringHash(string, starter);
		return 32769 * ((9 * result) ^ ((unsigned int)(9 * result) >> 11));
	}

	typedef uintptr_t grcTexture;

	namespace grPostFX
	{
		using BuildHudlessRTFn = void*(__fastcall*)(rdrPostFX*);

		rdrPostFX** sm_InstanceStorage = nullptr;
		BuildHudlessRTFn BuildHudlessRT = nullptr;

		rdrPostFX* GetInstance()
		{
			return sm_InstanceStorage ? *sm_InstanceStorage : nullptr;
		}
	}

	struct grcTextureFactory;

	struct grcTextureFactoryVTable {
		void* pad_0000[7];                                                          // 0x0000
		grcTexture* (__fastcall* GetBackBuffer)(grcTextureFactory* factory, bool);  // 0x0038
	};
	VALIDATE_SIZE(grcTextureFactoryVTable, 0x40);

	struct grcTextureFactory {
		grcTextureFactoryVTable* vtable;        // 0x0000
		grcTexture* BackBufferColour_Surface_0; // 0x0008
		grcTexture* BackBufferColour_Surface_1; // 0x0010

		grcTexture* GetBackBuffer(bool frontBuffer)
		{
			return vtable->GetBackBuffer(this, frontBuffer);
		}
	};
	VALIDATE_SIZE(grcTextureFactory, 0x18);
	grcTextureFactory** GTF_InstanceStorage = nullptr;

	grcTextureFactory* GetTextureFactory()
	{
		return GTF_InstanceStorage ? *GTF_InstanceStorage : nullptr;
	}

}

export class rdrPostFX
{
public:
	void* vftable;                                      // 0x0000
	char pad_0008[0x10 - 0x08];                        // 0x0008
	rage::grcTexture* FullScreenCopy;                        // 0x0010
	char pad_0018[0x90 - 0x18];                        // 0x0018
	rage::grcTexture* BrightPass;                            // 0x0090
	rage::grcTexture* BrightPass2;                           // 0x0098
	char pad_00A0[0xD0 - 0xA0];                        // 0x00A0
	rage::grcTexture* GaussRT;                               // 0x00D0
	char pad_00D8[0x3F0 - 0xD8];                       // 0x00D8
	rage::grcTexture* VelocityRT;                            // 0x03F0
	rage::grcTexture* ReactivityRT;                          // 0x03F8
	rage::grcTexture* FSRLinearOutput;                       // 0x0400
	rage::grcTexture* FullScreenCopyRT;                      // 0x0408
	char pad_0410[0x540 - 0x410];                      // 0x0410
	rage::grcTexture* HudlessRT;                             // 0x0540
	char pad_0548[0x880 - 0x548];                      // 0x0548
	rage::grcTexture* FXAATarget;                            // 0x0880
	char pad_0888[0x898 - 0x888];                      // 0x0888
	rage::grcTexture* PostFXAATarget;                        // 0x0898
	char** PPPNames;                                   // 0x08A0
	uint16_t PPPCount;                                 // 0x08A8
	char pad_08AA[0x8B0 - 0x8AA];                      // 0x08AA
	void* PPPData;                                     // 0x08B0
	char pad_08B8[0xDF0 - 0x8B8];                      // 0x08B8
	rage::grcTexture* MiniBrightPass;                        // 0x0DF0
	rage::grcTexture* MiniBrightPassScratch;                 // 0x0DF8
	char pad_0E00[0xE10 - 0xE00];                      // 0x0E00
	bool InitializedRenderTargets;                     // 0x0E10
	char pad_0E11[0x1000 - 0xE11];                     // 0x0E11
};
VALIDATE_SIZE(rdrPostFX, 0x1000);

export namespace UAL
{
	bool (WINAPI* GetOverloadPathW)(wchar_t* out, size_t out_size) = nullptr;
	bool (WINAPI* AddVirtualFileForOverloadW)(const wchar_t* virtualPath, const uint8_t* data, size_t size, int priority) = nullptr;
}

class Common
{
public:
	Common()
	{
		// from fusafix
		ModuleList dlls;
		dlls.Enumerate(ModuleList::SearchLocation::LocalOnly);
		for (auto& e : dlls.m_moduleList)
		{
			auto m = std::get<HMODULE>(e);
			if (IsModuleUAL(m))
			{
				UAL::GetOverloadPathW = (decltype(UAL::GetOverloadPathW))GetProcAddress(m, "GetOverloadPathW");
				UAL::AddVirtualFileForOverloadW = (decltype(UAL::AddVirtualFileForOverloadW))GetProcAddress(m, "AddVirtualFileForOverloadW");
				break;
			}
		}

		auto pattern = hook::pattern("48 8B 0D ? ? ? ? 48 89 91");
		if (!pattern.empty())
		{
			auto displacement = resolve_displacement(pattern.get_first());
			if (displacement.has_value())
				rage::grPostFX::sm_InstanceStorage = reinterpret_cast<rdrPostFX**>(displacement.value());
		}

		pattern = hook::pattern("48 8B 0D ? ? ? ? 41 B9 ? ? ? ? 45 33 C0 33 D2 ? ? ? FF 90 ? ? ? ? 41 8B D6");
		if (!pattern.empty())
		{
			auto displacement = resolve_displacement(pattern.get_first());
			if (displacement.has_value())
				rage::GTF_InstanceStorage = reinterpret_cast<rage::grcTextureFactory**>(displacement.value());
		}

		pattern = hook::pattern("48 89 74 24 ? 57 48 83 EC ? 48 8B F9 33 C9 E8");
		if(!pattern.empty())
		rage::grPostFX::BuildHudlessRT = reinterpret_cast<rage::grPostFX::BuildHudlessRTFn>(
			pattern.get_first(-0xA));

		pattern = hook::pattern("53 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 8B D3");
		if (!pattern.empty())
			game_free = (uintptr_t)pattern.get_first(-5);

		pattern = hook::pattern("48 8B 0D ? ? ? ? 48 8D 15 ? ? ? ? E8 ? ? ? ? 48 85 C0 74 ? 48 8D 88 ? ? ? ? ? ? ? FF 90 ? ? ? ? 48 8B 0D ? ? ? ? 48 85 C9");
		if (!pattern.empty())
		{
			auto displacement = resolve_displacement(pattern.get_first());
			if (displacement.has_value())
				rage::UIFactory::sm_Instance = reinterpret_cast<uintptr_t*>(displacement.value());
		}

	}
}Common;
