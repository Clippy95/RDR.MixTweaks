module;

#include "common.hxx"
#include <MemoryMgr.h>
#include <safetyhook.hpp>
import common;
export module mousefix;

volatile float cover_sens_fix_multiplier = 2.f;
uintptr_t camera_pointer = NULL;

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
				auto pattern = hook::pattern("F3 0F 5C C1 41 0F 2F C1 72 ? 0F 28 D7");
				if (!pattern.empty()) {
					Memory::VP::Nop(pattern.get_first<void>(0), 0x2C);
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