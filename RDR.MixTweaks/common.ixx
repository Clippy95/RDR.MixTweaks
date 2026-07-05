module;

#include "common.hxx"
#include <Zydis.h>
#include <cstring>
#include <limits>
#include <safetyhook.hpp>

export module common;

import <stacktrace>;
import <optional>;

// common file from FusaFix https://github.com/ThirteenAG/GTAIV.EFLC.FusionFix/blob/master/source/common.ixx

export class MixTweaks
{
public:
    template<typename... Args>
    class Event : public std::function<void(Args...)>
    {
    public:
        using std::function<void(Args...)>::function;

    private:
        std::list<std::function<void(Args...)>> handlers;

    public:
        auto operator+=(std::function<void(Args...)>&& handler) -> std::function<void()>
        {
            auto it = handlers.insert(handlers.end(), std::move(handler));
            return [this, it]() { handlers.erase(it); };
        }

        void executeAll(Args... args) const
        {
            if (!handlers.empty())
            {
                for (auto& handler : handlers)
                {
                    handler(args...);
                }
            }
        }

        std::reference_wrapper<std::vector<std::future<void>>> executeAllAsync(Args... args) const
        {
            static std::vector<std::future<void>> pendingFutures;
            if (!handlers.empty())
            {
                for (auto& handler : handlers)
                {
                    pendingFutures.emplace_back(std::async(std::launch::async, std::cref(handler), args...));
                }
            }
            return std::ref(pendingFutures);
        }
    };

public:
    static Event<>& onInitEvent()
    {
        static Event<> InitEvent;
        return InitEvent;
    }
    //static Event<>& onInitEventAsync()
    //{
    //    static Event<> InitEventAsync;
    //    return InitEventAsync;
    //}
    static Event<>& onShutdownEvent()
    {
        static Event<> ShutdownEvent;
        return ShutdownEvent;
    }
    //static Event<>& onGameInitEvent()
    //{
    //    static Event<> GameInitEvent;
    //    return GameInitEvent;
    //}
    //static Event<>& onGameProcessEvent()
    //{
    //    static Event<> GameProcessEvent;
    //    return GameProcessEvent;
    //}
    //static Event<>& onMenuDrawingEvent()
    //{
    //    static Event<> MenuDrawingEvent;
    //    return MenuDrawingEvent;
    //}
    //static Event<>& onMenuEnterEvent()
    //{
    //    static Event<> MenuEnterEvent;
    //    return MenuEnterEvent;
    //}
    //static Event<>& onMenuExitEvent()
    //{
    //    static Event<> MenuExitEvent;
    //    return MenuExitEvent;
    //}
    //static Event<bool>& onActivateApp()
    //{
    //    static Event<bool> ActivateApp;
    //    return ActivateApp;
    //}
    //static Event<>& onBeforeReset()
    //{
    //    static Event<> BeforeReset;
    //    return BeforeReset;
    //}
    //static Event<>& onEndScene()
    //{
    //    static Event<> EndScene;
    //    return EndScene;
    //}
    //static Event<>& onReadGameConfig()
    //{
    //    static Event<> ReadGameConfig;
    //    return ReadGameConfig;
    //}
    //static Event<>& onAfterReset() {
    //    static Event<> AfterReset;
    //    return AfterReset;
    //}
    //static Event<>& onBeforePostFX() {
    //    static Event<> BeforePostFX;
    //    return BeforePostFX;
    //}
    //static Event<>& onAfterPostFX() {
    //    static Event<> AfterPostFX;
    //    return AfterPostFX;
    //}
};

export template<typename Ret, typename... Args>
inline Ret cdecl_call(uintptr_t addr, Args... args)
{
    return reinterpret_cast<Ret(__cdecl*)(Args...)>(addr)(args...);
}

export template<typename Ret, typename... Args>
inline Ret stdcall_call(uintptr_t addr, Args... args)
{
    return reinterpret_cast<Ret(__stdcall*)(Args...)>(addr)(args...);
}

export template<typename Ret, typename... Args>
inline Ret fastcall_call(uintptr_t addr, Args... args)
{
    return reinterpret_cast<Ret(__fastcall*)(Args...)>(addr)(args...);
}

export template<typename Ret, typename... Args>
inline Ret thiscall_call(uintptr_t addr, Args... args)
{
    return reinterpret_cast<Ret(__thiscall*)(Args...)>(addr)(args...);
}

export template<typename Ret, typename Obj, typename... Args>
inline Ret vftable_call(Obj* object, ptrdiff_t vftable_offset, Args... args)
{
    using Fn = Ret(__fastcall*)(Obj*, Args...);

    auto vftable = *reinterpret_cast<uintptr_t**>(object);
    auto fn = reinterpret_cast<Fn>(*reinterpret_cast<uintptr_t*>(
        reinterpret_cast<uintptr_t>(vftable) + vftable_offset
        ));

    return fn(object, args...);
}

export constexpr uintptr_t rdr_exe_base = 0x140000000;

export void* RdrAddress(uintptr_t address)
{
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)) + (address - rdr_exe_base));
}

export template<class T = std::filesystem::path>
T GetModulePath(HMODULE hModule)
{
    static constexpr auto INITIAL_BUFFER_SIZE = MAX_PATH;
    static constexpr auto MAX_ITERATIONS = 7;

    if constexpr (std::is_same_v<T, std::filesystem::path>)
    {
        std::u16string ret;
        std::filesystem::path pathret;
        auto bufferSize = INITIAL_BUFFER_SIZE;
        for (size_t iterations = 0; iterations < MAX_ITERATIONS; ++iterations)
        {
            ret.resize(bufferSize);
            size_t charsReturned = 0;
            charsReturned = GetModuleFileNameW(hModule, (LPWSTR)&ret[0], bufferSize);
            if (charsReturned < ret.length())
            {
                ret.resize(charsReturned);
                pathret = ret;
                return pathret;
            }
            else
            {
                bufferSize *= 2;
            }
        }
    }
    else
    {
        T ret;
        auto bufferSize = INITIAL_BUFFER_SIZE;
        for (size_t iterations = 0; iterations < MAX_ITERATIONS; ++iterations)
        {
            ret.resize(bufferSize);
            size_t charsReturned = 0;
            if constexpr (std::is_same_v<T, std::string>)
                charsReturned = GetModuleFileNameA(hModule, &ret[0], bufferSize);
            else
                charsReturned = GetModuleFileNameW(hModule, &ret[0], bufferSize);
            if (charsReturned < ret.length())
            {
                ret.resize(charsReturned);
                return ret;
            }
            else
            {
                bufferSize *= 2;
            }
        }
    }
    return T();
}

export template<class T = std::filesystem::path>
T GetThisModulePath()
{
    HMODULE hm = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&MixTweaks::onInitEvent, &hm);
    T r = GetModulePath<T>(hm);
    if constexpr (std::is_same_v<T, std::filesystem::path>)
        return r.parent_path();
    else if constexpr (std::is_same_v<T, std::string>)
        r = r.substr(0, r.find_last_of("/\\") + 1);
    else
        r = r.substr(0, r.find_last_of(L"/\\") + 1);
    return r;
}

export template<class T = std::filesystem::path>
T GetThisModuleName()
{
    HMODULE hm = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&MixTweaks::onInitEvent, &hm);
    const T moduleFileName = GetModulePath<T>(hm);

    if constexpr (std::is_same_v<T, std::filesystem::path>)
        return moduleFileName.filename();
    else if constexpr (std::is_same_v<T, std::string>)
        return moduleFileName.substr(moduleFileName.find_last_of("/\\") + 1);
    else
        return moduleFileName.substr(moduleFileName.find_last_of(L"/\\") + 1);
}

export template<class T = std::filesystem::path>
T GetExeModulePath()
{
    T r = GetModulePath<T>(NULL);

    if constexpr (std::is_same_v<T, std::filesystem::path>)
        return r.parent_path();
    else if constexpr (std::is_same_v<T, std::string>)
        r = r.substr(0, r.find_last_of("/\\") + 1);
    else
        r = r.substr(0, r.find_last_of(L"/\\") + 1);
    return r;
}

export template<class T = std::filesystem::path>
T GetExeModuleName()
{
    const T moduleFileName = GetModulePath<T>(NULL);
    if constexpr (std::is_same_v<T, std::filesystem::path>)
        return moduleFileName.filename();
    else if constexpr (std::is_same_v<T, std::string>)
        return moduleFileName.substr(moduleFileName.find_last_of("/\\") + 1);
    else
        return moduleFileName.substr(moduleFileName.find_last_of(L"/\\") + 1);
}

export bool iequals(std::string_view s1, std::string_view s2)
{
    if (s1.size() != s2.size()) return false;
    return std::equal(s1.begin(), s1.end(), s2.begin(), s2.end(),
        [](char a, char b) { return ::tolower(a) == ::tolower(b); });
}

export bool iequals(std::wstring_view s1, std::wstring_view s2)
{
    if (s1.size() != s2.size()) return false;
    return std::equal(s1.begin(), s1.end(), s2.begin(), s2.end(),
        [](wchar_t a, wchar_t b) { return ::towlower(a) == ::towlower(b); });
}

export std::filesystem::path lexicallyRelativeCaseIns(const std::filesystem::path& path, const std::filesystem::path& base)
{
    class input_iterator_range
    {
    public:
        input_iterator_range(const std::filesystem::path::const_iterator& first, const std::filesystem::path::const_iterator& last)
            : _first(first)
            , _last(last)
        {
        }
        std::filesystem::path::const_iterator begin() const
        {
            return _first;
        }
        std::filesystem::path::const_iterator end() const
        {
            return _last;
        }
    private:
        std::filesystem::path::const_iterator _first;
        std::filesystem::path::const_iterator _last;
    };

    if (!iequals(path.root_name().wstring(), base.root_name().wstring()) || path.is_absolute() != base.is_absolute() || (!path.has_root_directory() && base.has_root_directory()))
    {
        return std::filesystem::path();
    }

    std::filesystem::path::const_iterator a = path.begin(), b = base.begin();

    while (a != path.end() && b != base.end() && iequals(a->wstring(), b->wstring()))
    {
        ++a;
        ++b;
    }

    if (a == path.end() && b == base.end())
    {
        return std::filesystem::path(".");
    }

    int count = 0;

    for (const auto& element : input_iterator_range(b, base.end()))
    {
        if (element != "." && element != "" && element != "..")
        {
            ++count;
        }
        else if (element == "..")
        {
            --count;
        }
    }

    if (count < 0)
    {
        return std::filesystem::path();
    }

    std::filesystem::path result;
    for (int i = 0; i < count; ++i)
    {
        result /= "..";
    }

    for (const auto& element : input_iterator_range(a, path.end()))
    {
        result /= element;
    }

    return result;
};

export inline void CreateThreadAutoClose(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId)
{
    CloseHandle(CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId));
}

export inline bool IsModuleUAL(HMODULE mod)
{
    if (GetProcAddress(mod, "IsUltimateASILoader") != NULL)
        return true;
    return false;
}

export bool IsUALPresent()
{
    for (const auto& entry : std::stacktrace::current())
    {
        HMODULE hModule = NULL;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)entry.native_handle(), &hModule))
        {
            if (IsModuleUAL(hModule))
                return true;
        }
    }
    return false;
}

export class CallbackHandler
{
public:
    static void RegisterCallback(std::function<void()>&& fn);
    static void RegisterCallback(std::wstring_view module_name, std::function<void()>&& fn);
    static void RegisterModuleLoadCallback(std::wstring_view module_name, std::function<void()>&& fn);
    static void RegisterModuleUnloadCallback(std::wstring_view module_name, std::function<void()>&& fn);
    static void RegisterAnyModuleLoadCallback(std::function<void(HMODULE)>&& fn);
    static void RegisterAnyModuleUnloadCallback(std::function<void(std::wstring_view module_name)>&& fn);

    [[deprecated("Use RegisterCallbackAtGetSystemTimeAsFileTime instead.")]]
    static void RegisterCallback(std::function<void()>&& fn, hook::pattern pattern);

    static void RegisterCallbackAtGetSystemTimeAsFileTime(std::function<void()>&& fn);
    static void RegisterCallbackAtGetSystemTimeAsFileTime(std::function<void()>&& fn, hook::pattern pattern);

    static std::once_flag flag;
};

namespace callback_handler_detail
{
    struct Comparator
    {
        bool operator() (const std::wstring& s1, const std::wstring& s2) const
        {
            std::wstring str1(s1.length(), ' ');
            std::wstring str2(s2.length(), ' ');
            std::transform(s1.begin(), s1.end(), str1.begin(), tolower);
            std::transform(s2.begin(), s2.end(), str2.begin(), tolower);
            return str1 < str2;
        }
    };

    struct ThreadParams
    {
        ThreadParams(std::function<void()> fn_, std::optional<hook::pattern> pattern_) :
            fn(std::move(fn_)),
            pattern(std::move(pattern_))
        {}

        std::function<void()> fn;
        std::optional<hook::pattern> pattern;
        bool executed{};
    };

    typedef NTSTATUS(NTAPI* _LdrRegisterDllNotification)(ULONG, PVOID, PVOID, PVOID);
    typedef NTSTATUS(NTAPI* _LdrUnregisterDllNotification)(PVOID);

    typedef struct _LDR_DLL_LOADED_NOTIFICATION_DATA
    {
        ULONG Flags;
        PUNICODE_STRING FullDllName;
        PUNICODE_STRING BaseDllName;
        PVOID DllBase;
        ULONG SizeOfImage;
    } LDR_DLL_LOADED_NOTIFICATION_DATA, LDR_DLL_UNLOADED_NOTIFICATION_DATA, * PLDR_DLL_LOADED_NOTIFICATION_DATA, * PLDR_DLL_UNLOADED_NOTIFICATION_DATA;

    typedef union _LDR_DLL_NOTIFICATION_DATA
    {
        LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
        LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
    } LDR_DLL_NOTIFICATION_DATA, * PLDR_DLL_NOTIFICATION_DATA;

    typedef NTSTATUS(NTAPI* PLDR_MANIFEST_PROBER_ROUTINE)(IN HMODULE DllBase, IN PCWSTR FullDllPath, OUT PHANDLE ActivationContext);
    typedef NTSTATUS(NTAPI* PLDR_ACTX_LANGUAGE_ROURINE)(IN HANDLE Unk, IN USHORT LangID, OUT PHANDLE ActivationContext);
    typedef void(NTAPI* PLDR_RELEASE_ACT_ROUTINE)(IN HANDLE ActivationContext);
    typedef VOID(NTAPI* fnLdrSetDllManifestProber)(IN PLDR_MANIFEST_PROBER_ROUTINE ManifestProberRoutine, IN PLDR_ACTX_LANGUAGE_ROURINE CreateActCtxLanguageRoutine, IN PLDR_RELEASE_ACT_ROUTINE ReleaseActCtxRoutine);

    auto& GetOnModuleLoadCallbackList()
    {
        static std::map<std::wstring, std::function<void()>, Comparator> onModuleLoad;
        return onModuleLoad;
    }

    auto& GetOnModuleUnloadCallbackList()
    {
        static std::map<std::wstring, std::function<void()>, Comparator> onModuleUnload;
        return onModuleUnload;
    }

    auto& GetOnAnyModuleLoadCallbackList()
    {
        static std::vector<std::function<void(HMODULE)>> onAnyModuleLoad;
        return onAnyModuleLoad;
    }

    auto& GetOnAnyModuleUnloadCallbackList()
    {
        static std::vector<std::function<void(std::wstring_view)>> onAnyModuleUnload;
        return onAnyModuleUnload;
    }

    auto& GetCallbackParamsList()
    {
        static std::vector<ThreadParams> callbackParams;
        return callbackParams;
    }

    SafetyHookInline shGetSystemTimeAsFileTime = {};
    _LdrRegisterDllNotification LdrRegisterDllNotification = nullptr;
    _LdrUnregisterDllNotification LdrUnregisterDllNotification = nullptr;
    void* cookie = nullptr;
    fnLdrSetDllManifestProber LdrSetDllManifestProber = nullptr;

    void invokeOnModuleLoad(std::wstring_view module_name)
    {
        if (GetOnModuleLoadCallbackList().count(module_name.data()))
            GetOnModuleLoadCallbackList().at(module_name.data())();
    }

    void invokeOnUnload(std::wstring_view module_name)
    {
        if (GetOnModuleUnloadCallbackList().count(module_name.data()))
            GetOnModuleUnloadCallbackList().at(module_name.data())();
    }

    void invokeOnAnyModuleLoad(HMODULE mod)
    {
        for (auto& f : GetOnAnyModuleLoadCallbackList())
            f(mod);
    }

    void invokeOnAnyModuleUnload(std::wstring_view module_name)
    {
        for (auto& f : GetOnAnyModuleUnloadCallbackList())
            f(module_name);
    }

    void CALLBACK LdrDllNotification(ULONG NotificationReason, PLDR_DLL_NOTIFICATION_DATA NotificationData, PVOID Context)
    {
        static constexpr auto LDR_DLL_NOTIFICATION_REASON_LOADED = 1;
        static constexpr auto LDR_DLL_NOTIFICATION_REASON_UNLOADED = 2;

        if (NotificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED)
        {
            invokeOnModuleLoad(NotificationData->Loaded.BaseDllName->Buffer);
            invokeOnAnyModuleLoad((HMODULE)NotificationData->Loaded.DllBase);
        }
        else if (NotificationReason == LDR_DLL_NOTIFICATION_REASON_UNLOADED)
        {
            invokeOnUnload(NotificationData->Loaded.BaseDllName->Buffer);
            invokeOnAnyModuleUnload(NotificationData->Loaded.BaseDllName->Buffer);
        }
    }

    NTSTATUS NTAPI ProbeCallback(IN HMODULE DllBase, IN PCWSTR FullDllPath, OUT PHANDLE ActivationContext)
    {
        std::wstring str(FullDllPath);
        invokeOnModuleLoad(str.substr(str.find_last_of(L"/\\") + 1));
        invokeOnAnyModuleLoad(DllBase);

        ACTCTXW act = { 0 };
        act.cbSize = sizeof(act);
        act.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
        act.lpSource = FullDllPath;
        act.hModule = DllBase;
        act.lpResourceName = ISOLATIONAWARE_MANIFEST_RESOURCE_ID;

        *ActivationContext = 0;

        HANDLE actx = CreateActCtxW(&act);
        if (actx == INVALID_HANDLE_VALUE)
            return 0xC000008B;

        *ActivationContext = actx;
        return STATUS_SUCCESS;
    }

    void RegisterDllNotification()
    {
        LdrRegisterDllNotification = (_LdrRegisterDllNotification)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "LdrRegisterDllNotification");
        if (LdrRegisterDllNotification)
        {
            if (!cookie)
                LdrRegisterDllNotification(0, LdrDllNotification, 0, &cookie);
            return;
        }

        LdrSetDllManifestProber = (fnLdrSetDllManifestProber)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "LdrSetDllManifestProber");
        if (LdrSetDllManifestProber)
            LdrSetDllManifestProber(&ProbeCallback, NULL, &ReleaseActCtx);
    }

    void UnRegisterDllNotification()
    {
        LdrUnregisterDllNotification = (_LdrUnregisterDllNotification)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "LdrUnregisterDllNotification");
        if (LdrUnregisterDllNotification && cookie)
            LdrUnregisterDllNotification(cookie);
    }

    void WINAPI GetSystemTimeAsFileTimeHook(LPFILETIME lpSystemTimeAsFileTime)
    {
        bool allExecuted = false;

        {
            auto& threadParams = GetCallbackParamsList();

            static std::mutex threadParamsMutex;
            std::lock_guard<std::mutex> lock(threadParamsMutex);

            for (auto& it : threadParams)
            {
                if (!it.executed && (!it.pattern.has_value() || !it.pattern.value().clear().empty()))
                {
                    it.executed = true;
                    it.fn();
                }
            }

            allExecuted = std::all_of(threadParams.begin(), threadParams.end(), [](const auto& params) { return params.executed; });
        }

        shGetSystemTimeAsFileTime.stdcall<void>(lpSystemTimeAsFileTime);

        if (allExecuted)
            shGetSystemTimeAsFileTime = {};
    }

    DWORD WINAPI ThreadProc(LPVOID ptr)
    {
        auto paramsPtr = (ThreadParams*)ptr;
        auto params = *paramsPtr;
        delete paramsPtr;

        LARGE_INTEGER liDueTime;
        liDueTime.QuadPart = -30 * 10000000LL;
        HANDLE hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
        SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0);

        while (params.pattern.value().clear().empty())
        {
            Sleep(0);

            if (WaitForSingleObject(hTimer, 0) == WAIT_OBJECT_0)
            {
                CloseHandle(hTimer);
                return 0;
            }
        }

        CloseHandle(hTimer);
        params.fn();

        return 0;
    }
}

std::once_flag CallbackHandler::flag;

void CallbackHandler::RegisterCallback(std::function<void()>&& fn)
{
    fn();
}

void CallbackHandler::RegisterCallback(std::wstring_view module_name, std::function<void()>&& fn)
{
    RegisterModuleLoadCallback(module_name, std::move(fn));
}

void CallbackHandler::RegisterModuleLoadCallback(std::wstring_view module_name, std::function<void()>&& fn)
{
    if (module_name.empty() || GetModuleHandleW(module_name.data()) != NULL)
    {
        fn();
        return;
    }

    callback_handler_detail::RegisterDllNotification();
    callback_handler_detail::GetOnModuleLoadCallbackList().emplace(module_name, std::move(fn));
}

void CallbackHandler::RegisterModuleUnloadCallback(std::wstring_view module_name, std::function<void()>&& fn)
{
    callback_handler_detail::RegisterDllNotification();
    callback_handler_detail::GetOnModuleUnloadCallbackList().emplace(module_name, std::move(fn));
}

void CallbackHandler::RegisterAnyModuleLoadCallback(std::function<void(HMODULE)>&& fn)
{
    callback_handler_detail::RegisterDllNotification();
    callback_handler_detail::GetOnAnyModuleLoadCallbackList().emplace_back(std::move(fn));
}

void CallbackHandler::RegisterAnyModuleUnloadCallback(std::function<void(std::wstring_view module_name)>&& fn)
{
    callback_handler_detail::RegisterDllNotification();
    callback_handler_detail::GetOnAnyModuleUnloadCallbackList().emplace_back(std::move(fn));
}

void CallbackHandler::RegisterCallback(std::function<void()>&& fn, hook::pattern pattern)
{
    if (!pattern.empty())
    {
        fn();
        return;
    }

    auto* ptr = new callback_handler_detail::ThreadParams(std::move(fn), pattern);
    CloseHandle(CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&callback_handler_detail::ThreadProc, (LPVOID)ptr, 0, NULL));
}

void CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(std::function<void()>&& fn)
{
    callback_handler_detail::GetCallbackParamsList().emplace_back(std::move(fn), std::nullopt);
    if (!callback_handler_detail::shGetSystemTimeAsFileTime)
        callback_handler_detail::shGetSystemTimeAsFileTime = safetyhook::create_inline(GetSystemTimeAsFileTime, callback_handler_detail::GetSystemTimeAsFileTimeHook);
}

void CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(std::function<void()>&& fn, hook::pattern pattern)
{
    if (!pattern.empty())
    {
        fn();
        return;
    }

    callback_handler_detail::GetCallbackParamsList().emplace_back(std::move(fn), pattern);
    if (!callback_handler_detail::shGetSystemTimeAsFileTime)
        callback_handler_detail::shGetSystemTimeAsFileTime = safetyhook::create_inline(GetSystemTimeAsFileTime, callback_handler_detail::GetSystemTimeAsFileTimeHook);
}

export template <size_t count = 1, typename... Args>
hook::pattern find_pattern(Args... args)
{
    hook::pattern pattern;
    ((pattern = hook::pattern(args), !pattern.count_hint(count).empty()) || ...);
    return pattern;
}

std::string format(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    std::vector<char> v(1024);
    while (true)
    {
        va_list args2;
        va_copy(args2, args);
        int res = vsnprintf(v.data(), v.size(), fmt, args2);
        if ((res >= 0) && (res < static_cast<int>(v.size())))
        {
            va_end(args);
            va_end(args2);
            return std::string(v.data());
        }
        size_t size;
        if (res < 0)
            size = v.size() * 2;
        else
            size = static_cast<size_t>(res) + 1;
        v.clear();
        v.resize(size);
        va_end(args2);
    }
}

export template<typename T>
std::array<uint8_t, sizeof(T)> to_bytes(const T& object)
{
    std::array<uint8_t, sizeof(T)> bytes;
    const uint8_t* begin = reinterpret_cast<const uint8_t*>(std::addressof(object));
    const uint8_t* end = begin + sizeof(T);
    std::copy(begin, end, std::begin(bytes));
    return bytes;
}

export template<typename T>
T& from_bytes(const std::array<uint8_t, sizeof(T)>& bytes, T& object)
{
    static_assert(std::is_trivially_copyable<T>::value, "not a TriviallyCopyable type");
    uint8_t* begin_object = reinterpret_cast<uint8_t*>(std::addressof(object));
    std::copy(std::begin(bytes), std::end(bytes), begin_object);
    return object;
}

export template<class T, class T1>
T from_bytes(const T1& bytes)
{
    static_assert(std::is_trivially_copyable<T>::value, "not a TriviallyCopyable type");
    T object;
    uint8_t* begin_object = reinterpret_cast<uint8_t*>(std::addressof(object));
    std::copy(std::begin(bytes), std::end(bytes) - (sizeof(T1) - sizeof(T)), begin_object);
    return object;
}

export class CMPatch
{
public:
    CMPatch() = default;

    CMPatch(std::initializer_list<std::function<void(CMPatch&)>> init)
    {
        for (auto& fn : init)
            fn(*this);
    }

    ~CMPatch()
    {
        Restore();
    }

    CMPatch(const CMPatch&) = delete;
    CMPatch& operator=(const CMPatch&) = delete;

    CMPatch(CMPatch&& other) noexcept
        : m_patches(std::move(other.m_patches))
        , m_isApplied(other.m_isApplied)
    {
        other.m_isApplied = false;
    }

    CMPatch& operator=(CMPatch&& other) noexcept
    {
        if (this != &other)
        {
            Restore();
            m_patches = std::move(other.m_patches);
            m_isApplied = other.m_isApplied;
            other.m_isApplied = false;
        }

        return *this;
    }

    template <typename T>
    CMPatch& Add(uintptr_t address, const T& value)
    {
        return AddRaw(RdrAddress(address), value);
    }

    CMPatch& Add(uintptr_t address, std::initializer_list<uint8_t> bytes)
    {
        return AddRaw(RdrAddress(address), bytes);
    }

    CMPatch& Add(uintptr_t address, const void* data, size_t len)
    {
        return AddRaw(RdrAddress(address), data, len);
    }

    template <typename T>
    CMPatch& Add(const char* signature, const T& value, ptrdiff_t get_first_arg = 0)
    {
        auto pattern = hook::pattern(signature);
        if (!pattern.empty())
            AddRaw(pattern.get_first(get_first_arg), value);

        return *this;
    }

    CMPatch& Add(const char* signature, std::initializer_list<uint8_t> bytes, ptrdiff_t get_first_arg = 0)
    {
        auto pattern = hook::pattern(signature);
        if (!pattern.empty())
            AddRaw(pattern.get_first(get_first_arg), bytes);

        return *this;
    }

    CMPatch& Add(const char* signature, const void* data, size_t len, ptrdiff_t get_first_arg = 0)
    {
        auto pattern = hook::pattern(signature);
        if (!pattern.empty())
            AddRaw(pattern.get_first(get_first_arg), data, len);

        return *this;
    }

    CMPatch& AddNop(uintptr_t address, size_t size)
    {
        return AddNopRaw(RdrAddress(address), size);
    }

    CMPatch& AddNop(const char* signature, size_t size, ptrdiff_t get_first_arg = 0)
    {
        auto pattern = hook::pattern(signature);
        if (!pattern.empty())
            AddNopRaw(pattern.get_first(get_first_arg), size);

        return *this;
    }

    template <typename T>
    CMPatch& AddRaw(uintptr_t address, const T& value)
    {
        return AddRaw(reinterpret_cast<void*>(address), value);
    }

    template <typename T>
    CMPatch& AddRaw(void* address, const T& value)
    {
        const auto bytes = to_bytes(value);
        return AddRaw(address, bytes.data(), bytes.size());
    }

    CMPatch& AddRaw(uintptr_t address, std::initializer_list<uint8_t> bytes)
    {
        return AddRaw(reinterpret_cast<void*>(address), bytes);
    }

    CMPatch& AddRaw(void* address, std::initializer_list<uint8_t> bytes)
    {
        return AddPatch(address, bytes.begin(), bytes.size());
    }

    CMPatch& AddRaw(uintptr_t address, const void* data, size_t len)
    {
        return AddRaw(reinterpret_cast<void*>(address), data, len);
    }

    CMPatch& AddRaw(void* address, const void* data, size_t len)
    {
        return AddPatch(address, static_cast<const uint8_t*>(data), len);
    }

    CMPatch& AddNopRaw(uintptr_t address, size_t size)
    {
        return AddNopRaw(reinterpret_cast<void*>(address), size);
    }

    CMPatch& AddNopRaw(void* address, size_t size)
    {
        std::vector<uint8_t> bytes(size, 0x90);
        return AddPatch(address, bytes.data(), bytes.size());
    }

    void Apply()
    {
        if (m_isApplied)
            return;

        for (auto& patch : m_patches)
            WriteBytes(patch.address, patch.patchBytes);

        m_isApplied = true;
    }

    void Restore()
    {
        if (!m_isApplied)
            return;

        for (auto& patch : m_patches)
            WriteBytes(patch.address, patch.originalBytes);

        m_isApplied = false;
    }

    void Clear()
    {
        Restore();
        m_patches.clear();
    }

    bool IsApplied() const
    {
        return m_isApplied;
    }

    bool Empty() const
    {
        return m_patches.empty();
    }

    size_t Size() const
    {
        return m_patches.size();
    }

private:
    struct PatchEntry
    {
        uintptr_t address{};
        std::vector<uint8_t> originalBytes;
        std::vector<uint8_t> patchBytes;
    };

    CMPatch& AddPatch(void* address, const uint8_t* data, size_t len)
    {
        if (!address || !data || len == 0)
            return *this;

        PatchEntry entry;
        entry.address = reinterpret_cast<uintptr_t>(address);
        entry.originalBytes.resize(len);
        entry.patchBytes.assign(data, data + len);

        std::memcpy(entry.originalBytes.data(), address, len);
        m_patches.push_back(std::move(entry));

        if (m_isApplied)
            WriteBytes(m_patches.back().address, m_patches.back().patchBytes);

        return *this;
    }

    static void WriteBytes(uintptr_t address, const std::vector<uint8_t>& bytes)
    {
        if (address == 0 || bytes.empty())
            return;

        DWORD oldProtect = 0;
        auto* ptr = reinterpret_cast<void*>(address);
        VirtualProtect(ptr, bytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect);
        std::memcpy(ptr, bytes.data(), bytes.size());
        VirtualProtect(ptr, bytes.size(), oldProtect, &oldProtect);
        FlushInstructionCache(GetCurrentProcess(), ptr, bytes.size());
    }

    std::vector<PatchEntry> m_patches;
    bool m_isApplied = false;
};

export template <size_t n>
std::string pattern_str(const std::array<uint8_t, n> bytes)
{
    std::string result;
    for (size_t i = 0; i < n; i++)
    {
        result += format("%02X ", bytes[i]);
    }
    return result;
}

export template <typename T>
std::string pattern_str(T t)
{
    return std::string((std::is_same<T, char>::value ? format("%c ", t) : format("%02X ", t)));
}

export template <typename T, typename... Rest>
std::string pattern_str(T t, Rest... rest)
{
    return std::string((std::is_same<T, char>::value ? format("%c ", t) : format("%02X ", t)) + pattern_str(rest...));
}

export std::string pattern_str(std::string_view str)
{
    std::stringstream str_stream;
    for (const auto& item : str)
    {
        str_stream << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << +uint8_t(item) << " ";
    }
    return str_stream.str();
}

export class IATHook
{
public:
    template <class... Ts>
    static auto Replace(HMODULE target_module, std::string_view dll_name, Ts&& ... inputs)
    {
        std::map<std::string, std::future<void*>> originalPtrs;

        const DWORD_PTR instance = reinterpret_cast<DWORD_PTR>(target_module);
        const PIMAGE_NT_HEADERS ntHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(instance + reinterpret_cast<PIMAGE_DOS_HEADER>(instance)->e_lfanew);
        PIMAGE_IMPORT_DESCRIPTOR pImports = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(instance + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
        DWORD dwProtect[2];

        // Regular imports
        for (; pImports->Name != 0; pImports++)
        {
            if (_stricmp(reinterpret_cast<const char*>(instance + pImports->Name), dll_name.data()) == 0)
            {
                if (pImports->OriginalFirstThunk != 0)
                {
                    const PIMAGE_THUNK_DATA pThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(instance + pImports->OriginalFirstThunk);

                    for (ptrdiff_t j = 0; pThunk[j].u1.AddressOfData != 0; j++)
                    {
                        auto pAddress = reinterpret_cast<void**>(instance + pImports->FirstThunk) + j;
                        if (!pAddress) continue;
                        VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                        ([&]
                            {
                                auto name = std::string_view(std::get<0>(inputs));
                                auto num = std::string("-1");
                                if (name.contains("@"))
                                {
                                    num = name.substr(name.find_last_of("@") + 1);
                                    name = name.substr(0, name.find_last_of("@"));
                                }

                                if (pThunk[j].u1.Ordinal & IMAGE_ORDINAL_FLAG)
                                {
                                    try
                                    {
                                        if (IMAGE_ORDINAL(pThunk[j].u1.Ordinal) == std::stoi(num.data()))
                                        {
                                            originalPtrs[std::get<0>(inputs)] = std::async(std::launch::deferred, [&]() -> void* { return *pAddress; });
                                            originalPtrs[std::get<0>(inputs)].wait();
                                            *pAddress = std::get<1>(inputs);
                                        }
                                    }
                                    catch (...) {}
                                }
                                else if ((*pAddress && *pAddress == (void*)GetProcAddress(GetModuleHandleA(dll_name.data()), name.data())) ||
                                    (strcmp(reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(instance + pThunk[j].u1.AddressOfData)->Name, name.data()) == 0))
                                {
                                    originalPtrs[std::get<0>(inputs)] = std::async(std::launch::deferred, [&]() -> void* { return *pAddress; });
                                    originalPtrs[std::get<0>(inputs)].wait();
                                    *pAddress = std::get<1>(inputs);
                                }
                            } (), ...);
                        VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                    }
                }
                else
                {
                    auto pFunctions = reinterpret_cast<void**>(instance + pImports->FirstThunk);

                    for (ptrdiff_t j = 0; pFunctions[j] != nullptr; j++)
                    {
                        auto pAddress = &pFunctions[j];
                        VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                        ([&]
                            {
                                if (*pAddress && *pAddress == (void*)GetProcAddress(GetModuleHandleA(dll_name.data()), std::get<0>(inputs)))
                                {
                                    originalPtrs[std::get<0>(inputs)] = std::async(std::launch::deferred, [&]() -> void* { return *pAddress; });
                                    originalPtrs[std::get<0>(inputs)].wait();
                                    *pAddress = std::get<1>(inputs);
                                }
                            } (), ...);
                        VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                    }
                }
            }
        }

        // Delay imports
        PIMAGE_DELAYLOAD_DESCRIPTOR pDelayed = reinterpret_cast<PIMAGE_DELAYLOAD_DESCRIPTOR>(instance + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress);
        if (pDelayed && ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress != 0 && ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].Size != 0)
        {
            for (; pDelayed->DllNameRVA != 0; pDelayed++)
            {
                if (_stricmp(reinterpret_cast<const char*>(instance + pDelayed->DllNameRVA), dll_name.data()) == 0)
                {
                    if (pDelayed->ImportAddressTableRVA != 0)
                    {
                        const PIMAGE_THUNK_DATA pThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(instance + pDelayed->ImportNameTableRVA);
                        const PIMAGE_THUNK_DATA pFThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(instance + pDelayed->ImportAddressTableRVA);

                        for (ptrdiff_t j = 0; pThunk[j].u1.AddressOfData != 0; j++)
                        {
                            auto pAddress = reinterpret_cast<void**>(&pFThunk[j].u1.Function);
                            if (!pAddress) continue;
                            VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                            ([&]
                                {
                                    auto name = std::string_view(std::get<0>(inputs));
                                    auto num = std::string("-1");
                                    if (name.contains("@"))
                                    {
                                        num = name.substr(name.find_last_of("@") + 1);
                                        name = name.substr(0, name.find_last_of("@"));
                                    }

                                    if (pThunk[j].u1.Ordinal & IMAGE_ORDINAL_FLAG)
                                    {
                                        try
                                        {
                                            if (IMAGE_ORDINAL(pThunk[j].u1.Ordinal) == std::stoi(num.data()))
                                            {
                                                originalPtrs[std::get<0>(inputs)] = std::async(std::launch::async,
                                                    [](void** pAddress, void* value, PVOID instance) -> void*
                                                    {
                                                        DWORD dwProtect[2];
                                                        VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                                                        MEMORY_BASIC_INFORMATION mbi;
                                                        mbi.AllocationBase = instance;
                                                        do
                                                        {
                                                            VirtualQuery(*pAddress, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
                                                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                                        } while (mbi.AllocationBase == instance);
                                                        auto r = *pAddress;
                                                        *pAddress = value;
                                                        VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                                                        return r;
                                                    }, pAddress, std::get<1>(inputs), (PVOID)instance);
                                            }
                                        }
                                        catch (...) {}
                                    }
                                    else if (strcmp(reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(instance + pThunk[j].u1.AddressOfData)->Name, name.data()) == 0)
                                    {
                                        originalPtrs[std::get<0>(inputs)] = std::async(std::launch::async,
                                            [](void** pAddress, void* value, PVOID instance) -> void*
                                            {
                                                DWORD dwProtect[2];
                                                VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                                                MEMORY_BASIC_INFORMATION mbi;
                                                mbi.AllocationBase = instance;
                                                do
                                                {
                                                    VirtualQuery(*pAddress, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
                                                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                                } while (mbi.AllocationBase == instance);
                                                auto r = *pAddress;
                                                *pAddress = value;
                                                VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                                                return r;
                                            }, pAddress, std::get<1>(inputs), (PVOID)instance);
                                    }
                                } (), ...);
                            VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                        }
                    }
                }
            }
        }

        // Fallback section scan (e.g. re5dx9.exe steam)
        if (originalPtrs.empty())
        {
            static auto getSection = [](const PIMAGE_NT_HEADERS nt_headers, unsigned section) -> PIMAGE_SECTION_HEADER
                {
                    return reinterpret_cast<PIMAGE_SECTION_HEADER>(
                        (UCHAR*)nt_headers->OptionalHeader.DataDirectory +
                        nt_headers->OptionalHeader.NumberOfRvaAndSizes * sizeof(IMAGE_DATA_DIRECTORY) +
                        section * sizeof(IMAGE_SECTION_HEADER));
                };

            for (auto i = 0; i < ntHeader->FileHeader.NumberOfSections; i++)
            {
                auto sec = getSection(ntHeader, i);
                auto pFunctions = reinterpret_cast<void**>(instance + std::max(sec->PointerToRawData, sec->VirtualAddress));

                for (ptrdiff_t j = 0; j < 300; j++)
                {
                    auto pAddress = &pFunctions[j];
                    VirtualProtect(pAddress, sizeof(void*), PAGE_EXECUTE_READWRITE, &dwProtect[0]);
                    ([&]
                        {
                            auto name = std::string_view(std::get<0>(inputs));
                            auto num = std::string("-1");
                            if (name.contains("@"))
                            {
                                num = name.substr(name.find_last_of("@") + 1);
                                name = name.substr(0, name.find_last_of("@"));
                            }

                            if (*pAddress && *pAddress == (void*)GetProcAddress(GetModuleHandleA(dll_name.data()), name.data()))
                            {
                                originalPtrs[std::get<0>(inputs)] = std::async(std::launch::deferred, [&]() -> void* { return *pAddress; });
                                originalPtrs[std::get<0>(inputs)].wait();
                                *pAddress = std::get<1>(inputs);
                            }
                        } (), ...);
                    VirtualProtect(pAddress, sizeof(void*), dwProtect[0], &dwProtect[1]);
                }

                if (!originalPtrs.empty())
                    return originalPtrs;
            }
        }

        return originalPtrs;
    }
};

template <class T>
static uintptr_t to_uintptr(T value)
{
    using U = std::remove_cvref_t<T>;

    if constexpr (std::is_pointer_v<U>)
        return reinterpret_cast<uintptr_t>(value);
    else
        return static_cast<uintptr_t>(value);
}

export std::optional<uintptr_t> resolve_displacement(auto ip)
{
    const uintptr_t runtime_ip = to_uintptr(ip);

    ZydisDecoder decoder{};

#if defined(_M_X64) || defined(__x86_64__)
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
#else
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);
#endif

    ZydisDecodedInstruction instruction{};
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};

    const ZyanStatus status = ZydisDecoderDecodeFull(
        &decoder,
        reinterpret_cast<const void*>(runtime_ip),
        ZYDIS_MAX_INSTRUCTION_LENGTH,
        &instruction,
        operands
    );

    if (!ZYAN_SUCCESS(status))
        return std::nullopt;

    for (uint32_t i = 0; i < instruction.operand_count_visible; ++i)
    {
        const auto& operand = operands[i];

        ZyanU64 absolute_address = 0;

        if (ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(
            &instruction,
            &operand,
            static_cast<ZyanU64>(runtime_ip),
            &absolute_address
        )))
        {
            return static_cast<uintptr_t>(absolute_address);
        }
    }

    return std::nullopt;
}

export bool resolve_pattern_displacement(const char* pattern_string, uintptr_t& value, int get_first_offset = 0)
{
    auto pattern = hook::pattern(pattern_string);
    if (pattern.empty())
        return false;

    auto displacement = resolve_displacement(pattern.get_first(get_first_offset));
    if (!displacement.has_value())
        return false;

    value = displacement.value();
    return true;
}

export struct RipRelativeDisplacement
{
    uintptr_t instruction{};
    uintptr_t target{};
    uintptr_t displacement_address{};
    int64_t displacement{};
    uint8_t instruction_length{};
    uint8_t displacement_offset{};
    uint8_t displacement_size{};
};

export std::optional<RipRelativeDisplacement> resolve_rip_relative_displacement(auto ip)
{
    const uintptr_t runtime_ip = to_uintptr(ip);

    ZydisDecoder decoder{};

#if defined(_M_X64) || defined(__x86_64__)
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
#else
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);
#endif

    ZydisDecodedInstruction instruction{};
    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT]{};

    const ZyanStatus status = ZydisDecoderDecodeFull(
        &decoder,
        reinterpret_cast<const void*>(runtime_ip),
        ZYDIS_MAX_INSTRUCTION_LENGTH,
        &instruction,
        operands
    );

    if (!ZYAN_SUCCESS(status) || instruction.raw.disp.size == 0)
        return std::nullopt;

    for (uint32_t i = 0; i < instruction.operand_count_visible; ++i)
    {
        const auto& operand = operands[i];
        if (operand.type != ZYDIS_OPERAND_TYPE_MEMORY)
            continue;

#if defined(_M_X64) || defined(__x86_64__)
        if (operand.mem.base != ZYDIS_REGISTER_RIP)
            continue;
#else
        if (operand.mem.base != ZYDIS_REGISTER_EIP)
            continue;
#endif

        ZyanU64 absolute_address = 0;
        if (!ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(
            &instruction,
            &operand,
            static_cast<ZyanU64>(runtime_ip),
            &absolute_address
        )))
        {
            return std::nullopt;
        }

        return RipRelativeDisplacement{
            runtime_ip,
            static_cast<uintptr_t>(absolute_address),
            runtime_ip + instruction.raw.disp.offset,
            static_cast<int64_t>(instruction.raw.disp.value),
            static_cast<uint8_t>(instruction.length),
            static_cast<uint8_t>(instruction.raw.disp.offset),
            static_cast<uint8_t>(instruction.raw.disp.size)
        };
    }

    return std::nullopt;
}

export std::optional<int32_t> make_rip_relative_offset(auto ip, uintptr_t target)
{
    const auto info = resolve_rip_relative_displacement(ip);
    if (!info.has_value() || info->displacement_size != 32)
        return std::nullopt;

    const auto next_instruction = info->instruction + info->instruction_length;
    const auto displacement = static_cast<int64_t>(target) - static_cast<int64_t>(next_instruction);
    if (displacement < std::numeric_limits<int32_t>::min() || displacement > std::numeric_limits<int32_t>::max())
        return std::nullopt;

    return static_cast<int32_t>(displacement);
}

export bool patch_rip_relative_target(auto ip, uintptr_t target)
{
    const auto info = resolve_rip_relative_displacement(ip);
    const auto displacement = make_rip_relative_offset(ip, target);
    if (!info.has_value() || !displacement.has_value())
        return false;

    Memory::VP::Patch<int32_t>(reinterpret_cast<void*>(info->displacement_address), displacement.value());
    return true;
}

export std::optional<int32_t> make_pattern_rip_relative_offset(const char* pattern_string, uintptr_t target, int get_first_offset = 0)
{
    auto pattern = hook::pattern(pattern_string);
    if (pattern.empty())
        return std::nullopt;

    return make_rip_relative_offset(pattern.get_first(get_first_offset), target);
}

export bool patch_pattern_rip_relative_target(const char* pattern_string, uintptr_t target, int get_first_offset = 0)
{
    auto pattern = hook::pattern(pattern_string);
    if (pattern.empty())
        return false;

    return patch_rip_relative_target(pattern.get_first(get_first_offset), target);
}

//export std::optional<uintptr_t> resolve_next_displacement(auto ip)
//{
//    ZydisDecoder decoder;
//#if defined(_M_X64) || defined(__x86_64__)
//    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
//#else
//    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);
//#endif
//
//    ZydisDecodedInstruction instruction;
//    ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
//
//    uintptr_t current_ip = (uintptr_t)ip;
//    size_t instruction_count = 0;
//
//    while (true)
//    {
//        ZyanStatus status = ZydisDecoderDecodeFull(
//            &decoder,
//            (void*)current_ip,
//            ZYDIS_MAX_INSTRUCTION_LENGTH,
//            &instruction,
//            operands
//        );
//
//        if (!ZYAN_SUCCESS(status))
//        {
//            return std::nullopt;
//        }
//
//        if (instruction.meta.category == ZYDIS_CATEGORY_COND_BR)
//        {
//            for (uint32_t i = 0; i < instruction.operand_count_visible; ++i)
//            {
//                const auto& operand = operands[i];
//
//                if (operand.type == ZYDIS_OPERAND_TYPE_MEMORY)
//                {
//                    if (operand.mem.disp.has_displacement)
//                    {
//#if defined(_M_X64) || defined(__x86_64__)
//                        if (operand.mem.is_rip_relative)
//                        {
//                            return current_ip + instruction.length + operand.mem.disp.value;
//                        }
//#else
//                        return static_cast<uintptr_t>(operand.mem.disp.value);
//#endif
//                    }
//                }
//                else if (operand.type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
//                {
//                    if (operand.imm.is_relative)
//                    {
//                        return current_ip + instruction.length + ZyanISize(operand.imm.value.s);
//                    }
//                }
//            }
//
//            if (instruction.attributes & ZYDIS_ATTRIB_IS_RELATIVE && instruction.raw.disp.size > 0)
//            {
//                return current_ip + instruction.length + ZyanISize(instruction.raw.disp.value);
//            }
//
//            return std::nullopt;
//        }
//
//        current_ip += instruction.length;
//
//        constexpr size_t MAX_INSTRUCTIONS = 20;
//        if (++instruction_count >= MAX_INSTRUCTIONS)
//        {
//            return std::nullopt;
//        }
//    }
//
//    return std::nullopt;
//}

export template <typename T>
bool pattern_to_ptr(const char* signature, T*& out)
{
    auto pattern = hook::pattern(signature);

    if (pattern.empty())
        return false;

    auto displacement = resolve_displacement(pattern.get_first());

    if (!displacement.has_value())
        return false;

    auto ptr_storage = reinterpret_cast<T**>(displacement.value());

    if (!ptr_storage || !*ptr_storage)
        return false;

    out = *ptr_storage;
    return true;
}

export template<typename T>
class GameRef
{
private:
    std::optional<T*> ptr{ std::nullopt };

public:
    GameRef() = default;

    void SetAddress(T* address)
    {
        if (address == nullptr)
            throw std::invalid_argument("GameRef::SetAddress called with null pointer");

        ptr = address;
    }

    T& get()
    {
        if (!ptr.has_value())
        {
            assert(false && "GameRef accessed before SetAddress()!");
            throw std::runtime_error("GameRef accessed before SetAddress() was called");
        }
        return **ptr;
    }

    const T& get() const
    {
        if (!ptr.has_value())
        {
            assert(false && "GameRef accessed before SetAddress()!");
            throw std::runtime_error("GameRef accessed before SetAddress() was called");
        }
        return **ptr;
    }

    bool is_initialized() const noexcept { return ptr.has_value(); }
    T* get_ptr() noexcept { return ptr.value_or(nullptr); }
    const T* get_ptr() const noexcept { return ptr.value_or(nullptr); }

    operator T& () { return get(); }
    operator const T& () const { return get(); }

    T& operator=(const T& value)
    {
        *reinterpret_cast<volatile T*>(*ptr) = value;
        return **ptr;
    }
    T& operator=(T&& value) { return get() = std::move(value); }

    template<typename U> T& operator+=(const U& v) { return get() += v; }
    template<typename U> T& operator-=(const U& v) { return get() -= v; }
    template<typename U> T& operator*=(const U& v) { return get() *= v; }
    template<typename U> T& operator/=(const U& v) { return get() /= v; }
    template<typename U> T& operator%=(const U& v) { return get() %= v; }

    template<typename U> T& operator&=(const U& v) { return get() &= v; }
    template<typename U> T& operator|=(const U& v) { return get() |= v; }
    template<typename U> T& operator^=(const U& v) { return get() ^= v; }
    template<typename U> T& operator<<=(const U& v) { return get() <<= v; }
    template<typename U> T& operator>>=(const U& v) { return get() >>= v; }

    T& operator++() { return ++get(); }
    T  operator++(int) { return get()++; }

    T& operator--() { return --get(); }
    T  operator--(int) { return get()--; }

    T operator+() const { return +get(); }
    T operator-() const { return -get(); }
    bool operator!() const { return !get(); }
    T operator~() const { return ~get(); }

    template<typename U> auto operator+(const U& v) const { return get() + v; }
    template<typename U> auto operator-(const U& v) const { return get() - v; }
    template<typename U> auto operator*(const U& v) const { return get() * v; }
    template<typename U> auto operator/(const U& v) const { return get() / v; }
    template<typename U> auto operator%(const U& v) const { return get() % v; }

    template<typename U> auto operator&(const U& v) const { return get() & v; }
    template<typename U> auto operator|(const U& v) const { return get() | v; }
    template<typename U> auto operator^(const U& v) const { return get() ^ v; }
    template<typename U> auto operator<<(const U& v) const { return get() << v; }
    template<typename U> auto operator>>(const U& v) const { return get() >> v; }

    template<typename U> bool operator==(const U& v) const { return get() == v; }
    template<typename U> bool operator!=(const U& v) const { return get() != v; }
    template<typename U> bool operator<(const U& v)  const { return get() < v; }
    template<typename U> bool operator>(const U& v)  const { return get() > v; }
    template<typename U> bool operator<=(const U& v) const { return get() <= v; }
    template<typename U> bool operator>=(const U& v) const { return get() >= v; }

    T& operator*() { return get(); }
    const T& operator*() const { return get(); }

    auto operator->()
    {
        if constexpr (std::is_pointer_v<T>)
            return get();
        else
            return &get();
    }

    auto operator->() const
    {
        if constexpr (std::is_pointer_v<T>)
            return get();
        else
            return &get();
    }

    template<typename U>
    auto operator[](const U& index) { return get()[index]; }

    template<typename U>
    auto operator[](const U& index) const { return get()[index]; }

    explicit operator bool() const { return static_cast<bool>(get()); }
};

#include <cassert>
#include <utility>

export template <typename Fn>
class GamePatternFn
{
public:
    GamePatternFn(const char* signature, int get_first_arg = 0)
    {
        auto pattern = hook::pattern(signature);

        if (!pattern.empty())
            m_fn = reinterpret_cast<Fn>(pattern.get_first(get_first_arg));
    }

    template <typename... Args>
    decltype(auto) operator()(Args&&... args) const
    {
        assert(m_fn);
        return m_fn(std::forward<Args>(args)...);
    }

    explicit operator bool() const
    {
        return m_fn != nullptr;
    }

private:
    Fn m_fn = nullptr;
};
