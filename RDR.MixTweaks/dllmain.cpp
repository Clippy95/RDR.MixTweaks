// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include "common.hxx"

import common;

void OpenConsole()
{
    AllocConsole();

    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    SetConsoleTitleA("Debug MixTweaks Console");

}

void Init()
{



    CIniReader ini{};
    if (ini.ReadInteger("DEBUG", "DisableCrashLog", 1) != 0)
    {
        auto pattern = hook::pattern("48 89 4C 24 ? 55 53 56 57 41 54 41 55 48 8D AC 24 ? ? ? ? 48 81 EC");
        Memory::VP::Patch(pattern.get_first(), 0xC3);
    }

    if (ini.ReadInteger("DEUB", "OpenConsole", 0))
        OpenConsole();

    MixTweaks::onInitEvent().executeAll();
}

extern "C"
{
    void __declspec(dllexport) InitializeASI()
    {
        std::call_once(CallbackHandler::flag, []()
            {
                CallbackHandler::RegisterCallback(Init, hook::pattern("E8 ? ? ? ? 4D 85 F6 74 ? 49 8B D6"));
            });
    }
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        MixTweaks::onShutdownEvent().executeAll();
    }
    return TRUE;
}

