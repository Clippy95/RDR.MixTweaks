module;

#include "common.hxx"
#include <Windows.h>
#include <MemoryMgr.h>
#include <safetyhook.hpp>
#include <cstddef>
#include <cstring>
export module mousefix;
import common;
import comvars;

volatile float cover_sens_fix_multiplier = 2.f;
uintptr_t camera_pointer = NULL;

uintptr_t game_action_input_global = NULL;
uintptr_t get_current_binding_for_action = NULL;
uintptr_t game_string_assign = NULL;

SafetyHookInline mouse_prompt_info_from_tag_hook{};

void AddSideButtonMask(SafetyHookContext& ctx)
{
	auto mouse = reinterpret_cast<unsigned char*>(ctx.r8);
	if (!mouse)
		return;

	if (mouse[0x10])
		ctx.rdx |= 1 << 5;
	if (mouse[0x11])
		ctx.rdx |= 1 << 6;
}

class UIPromptInfo
{
public:
	GameString texture_name;
	uint16_t arrangement;
	uint16_t pad_22;
	float scale;
	uint8_t mirrored;
	uint8_t pad_29[3];
	uint32_t width;
	uint32_t height;
};

static_assert(offsetof(UIPromptInfo, texture_name) == 0x00);
static_assert(offsetof(UIPromptInfo, arrangement) == 0x20);
static_assert(offsetof(UIPromptInfo, scale) == 0x24);
static_assert(offsetof(UIPromptInfo, mirrored) == 0x28);
static_assert(offsetof(UIPromptInfo, width) == 0x2C);
static_assert(offsetof(UIPromptInfo, height) == 0x30);

void ApplySideButtonPrompt(UIPromptInfo* prompt, uint64_t binding)
{
	const auto binding_type = static_cast<uint32_t>(binding);
	const auto input = static_cast<uint32_t>(binding >> 32);
	if ((binding_type != 3 && binding_type != 4) || (input != 5 && input != 6))
		return;

	if (!prompt)
		return;

	auto name = input == 5 ? "Mouse_4th_Button" : "Mouse_5th_Button";
	fastcall_call<void*>(game_string_assign, &prompt->texture_name, name, 16);

	prompt->arrangement = 0;
	prompt->scale = 1.0f;
	prompt->mirrored = 0;
	prompt->width = 0;
	prompt->height = 0;
}

UIPromptInfo* __fastcall MousePromptInfoFromTagHook(uintptr_t input_method, UIPromptInfo* prompt_info, const char* tag)
{
	auto result = mouse_prompt_info_from_tag_hook.unsafe_fastcall<UIPromptInfo*>(input_method, prompt_info, tag);
	if (!tag || !std::strchr(tag, '@'))
		return result;

	GameString action;
	fastcall_call<void*>(game_string_assign, &action, tag, std::strlen(tag));

	uint64_t binding = input_method;
	auto game_action_input = *reinterpret_cast<uintptr_t*>(game_action_input_global);
	if (game_action_input)
	{
		fastcall_call<void>(get_current_binding_for_action, game_action_input, &binding, &action);
		ApplySideButtonPrompt(prompt_info, binding);
	}

	action.Free();
	return result;
}

uintptr_t ReadPointer(uintptr_t baseAddress, const std::vector<uintptr_t>& offsets) {
	uintptr_t address = baseAddress;

	if (address == 0) {
		return 0;
	}

	for (size_t i = 0; i < offsets.size(); ++i) {
		uintptr_t* nextAddress = reinterpret_cast<uintptr_t*>(address);
		if (nextAddress == nullptr || *nextAddress == 0) {
			return 0;
		}
		address = *nextAddress + offsets[i];
	}

	return address;
}

class mousefix
{
public:
	mousefix()
	{
		MixTweaks::onInitEvent() += []() 
			{
				CIniReader ini{};
				auto pattern = hook::pattern("F3 0F 5C C1 41 0F 2F C1 72 ? 0F 28 D7");
				if (!pattern.empty()) {
					Memory::VP::Nop(pattern.get_first<void>(0), 0x2C);
				}



				if (ini.ReadInteger("CONTROLS", "EnableMouseSideHooks", 1) != 0)
				{
					resolve_pattern_displacement("48 8B 3D ? ? ? ? 44 38 A7 ? ? ? ? 74", game_action_input_global);

					pattern = hook::pattern("48 89 54 24 ? 48 89 4C 24 ? 55 56 57 41 54 41 56 41 57 48 8B EC");
					if (!pattern.empty())
						get_current_binding_for_action = (uintptr_t)pattern.get_first(-5);

					pattern = hook::pattern("48 89 6C 24 ? 56 57 41 57 48 83 EC ? 48 8B 69");

					if (!pattern.empty())
						game_string_assign = (uintptr_t)pattern.get_first(-5);

					pattern = hook::pattern("F3 0F 11 47 ? F3 0F 59 05");


					if (!pattern.empty())
						static auto extra_mouse_buttons_hook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& ctx) {
						auto mouse = reinterpret_cast<unsigned char*>(ctx.rdi);
						if (!mouse)
							return;

						mouse[0x10] = (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0;
						mouse[0x11] = (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0;
							});

					pattern = hook::pattern("? ? ? ? ? ? ? ? E9 ? ? ? ? ? ? 75 ? E8 ? ? ? ? ? ? ? ? ? ? ? 74 ? ? ? ? ? 75 ? ? ? ? ? ? ? ? 75 ? ? ? ? EB");
					if (!pattern.empty())
						static auto mouse_button_all_mask_hook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& ctx) {
						AddSideButtonMask(ctx);
							});
					pattern = hook::pattern("? ? ? ? ? ? ? ? ? ? E9 ? ? ? ? E8 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? E9");
					static auto mouse_button_any_mask_hook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& ctx) {
						AddSideButtonMask(ctx);
						});

					pattern = hook::pattern("83 FE ? 7C ? 32 C0");
					if (!pattern.empty())
						Memory::VP::Patch<uint8_t>(pattern.get_first(2), 7);

					pattern = hook::pattern("48 89 7C 24 ? 48 89 4C 24 ? 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 45 33 FF");
					if (!pattern.empty())
						mouse_prompt_info_from_tag_hook = safetyhook::create_inline(pattern.get_first(-5), MousePromptInfoFromTagHook);
				}
				pattern = hook::pattern("F3 0F 59 3D ? ? ? ? F3 0F 10 0D");

				if (!pattern.empty())
				{
					static auto cover_sens_hook = safetyhook::create_mid(pattern.get_first<void>(0), [](SafetyHookContext& ctx) {
						ctx.xmm7.f32[0] *= cover_sens_fix_multiplier;
						});
				}

				pattern = hook::pattern("48 8B 05 ? ? ? ? F3 0F 10 05 ? ? ? ? 48 8B 48 ? F3 0F 10 89 ? ? ? ? E8 ? ? ? ? 8B 05 ? ? ? ? 0F 28 F0 F3 0F 59 35 ? ? ? ? F7 D8 66 0F 6E F8");
				if (!pattern.empty()) {
					uint32_t camera_ptr_offset = *pattern.get_first<uint32_t>(3);

					camera_pointer = (uintptr_t)pattern.get_first<uintptr_t>(7);
					camera_pointer += camera_ptr_offset;
				}
				// TBF I can put this in the cover_sens_MID and do bools but I'd rather be all seperated because what if the above fails?
				pattern = hook::pattern("E8 ? ? ? ? 44 8B C0 48 8D 55 ? 48 8B CB E8 ? ? ? ? 8B 18 81 E3 ? ? ? ? 74");
				if (!pattern.empty())
					static auto cover_invert_fix = safetyhook::create_mid(pattern.get_first<void>(0), [](SafetyHookContext& ctx) {

					float& y_delta = ctx.xmm8.f32[0];
					float& x_delta = ctx.xmm7.f32[0];

					//printf("x_delta %f y_delta %f \n", y_delta, x_delta);
					if (camera_pointer) {
						uintptr_t bool_ptr = ReadPointer(camera_pointer, { 0x28,0x620,0x30,0x8 });
						bool_ptr = *(uintptr_t*)(bool_ptr);

						bool** invert_x = (bool**)(bool_ptr + 0x2f0);
						bool** invert_y = (bool**)(bool_ptr + 0x2f8);
						if (**invert_x)
							x_delta = -x_delta;
						if (**invert_y)
							y_delta = -y_delta;
					}

						});

			};
	}
}mousefix;
