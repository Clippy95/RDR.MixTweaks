//module;
//
//#include "common.hxx"
//#include <Windows.h>
//#include <MemoryMgr.h>
//#include <safetyhook.hpp>
//#include <cctype>
//#include <cstddef>
//#include <cstring>
//#include <algorithm>
//#include <cstdio>
//#include <filesystem>
//#include <optional>
//#include <set>
//#include <string>
//#include <vector>
//export module rpf;
//import common;
//import comvars;
//
//namespace
//{
//	constexpr uintptr_t fi_packfile_open = 0x1400CD290;
//	constexpr uintptr_t fi_packfile_open_bulk = 0x1400CE130;
//	constexpr uintptr_t fi_packfile_read = 0x1400CDAF0;
//	constexpr uintptr_t fi_packfile_read_bulk = 0x1400CE520;
//	constexpr uintptr_t fi_packfile_stream_size = 0x1400CDCC0;
//	constexpr uintptr_t fi_packfile_seek = 0x1400CDD00;
//	constexpr uintptr_t fi_packfile_close = 0x1400CD8A0;
//	constexpr uintptr_t fi_packfile_get_file_size = 0x1400CDF90;
//
//	SafetyHookInline packfile_open_hook{};
//	SafetyHookInline packfile_open_bulk_hook{};
//	SafetyHookInline packfile_read_hook{};
//	SafetyHookInline packfile_read_bulk_hook{};
//	SafetyHookInline packfile_stream_size_hook{};
//	SafetyHookInline packfile_seek_hook{};
//	SafetyHookInline packfile_close_hook{};
//	SafetyHookInline packfile_get_file_size_hook{};
//
//	std::filesystem::path mods_root{};
//	std::vector<std::filesystem::path> loose_file_roots{};
//	std::mutex loose_file_mutex{};
//	std::set<uintptr_t> loose_file_handles{};
//	bool print_file_requests = false;
//
//	void TrackLooseHandle(HANDLE handle)
//	{
//		if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
//			return;
//
//		std::scoped_lock lock(loose_file_mutex);
//		loose_file_handles.emplace((uintptr_t)handle);
//	}
//
//	std::string LowerPathName(const std::filesystem::path& path)
//	{
//		auto name = path.filename().string();
//		std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
//			return (char)std::tolower(c);
//			});
//
//		return name;
//	}
//
//	void BuildLooseFileRoots()
//	{
//		loose_file_roots.clear();
//		mods_root = GetExeModulePath() / "mods";
//
//		std::error_code ec{};
//		if (!std::filesystem::is_directory(mods_root, ec))
//			return;
//
//		for (const auto& entry : std::filesystem::directory_iterator(mods_root, ec))
//		{
//			if (ec)
//				break;
//
//			std::error_code entry_ec{};
//			if (entry.is_directory(entry_ec))
//				loose_file_roots.emplace_back(entry.path());
//		}
//
//		std::sort(loose_file_roots.begin(), loose_file_roots.end(), [](const auto& lhs, const auto& rhs) {
//			return LowerPathName(lhs) < LowerPathName(rhs);
//			});
//	}
//
//	void PrintLooseFileRoots()
//	{
//		if (!print_file_requests)
//			return;
//
//		const auto mods_root_string = mods_root.string();
//		char message[2048]{};
//		std::snprintf(message, sizeof(message), "[RPF] mods root %s, %zu mod folders\n", mods_root_string.c_str(), loose_file_roots.size());
//		std::printf("%s", message);
//		OutputDebugStringA(message);
//
//		for (const auto& root : loose_file_roots)
//		{
//			const auto root_string = root.string();
//			std::snprintf(message, sizeof(message), "[RPF] mod root %s\n", root_string.c_str());
//			std::printf("%s", message);
//			OutputDebugStringA(message);
//		}
//
//		std::fflush(stdout);
//	}
//
//	std::optional<std::filesystem::path> GetRelativeLoosePath(const char* game_path)
//	{
//		if (!game_path || !*game_path || loose_file_roots.empty())
//			return std::nullopt;
//
//		std::string normalized{ game_path };
//		std::replace(normalized.begin(), normalized.end(), '\\', '/');
//
//		while (!normalized.empty() && normalized.front() == '/')
//			normalized.erase(normalized.begin());
//
//		for (char& c : normalized)
//		{
//			if (c == ':')
//				c = '/';
//		}
//
//		std::filesystem::path relative{};
//		size_t part_start = 0;
//		while (part_start <= normalized.size())
//		{
//			const size_t part_end = normalized.find('/', part_start);
//			const auto part = normalized.substr(part_start, part_end == std::string::npos ? std::string::npos : part_end - part_start);
//
//			if (part == "..")
//				return std::nullopt;
//
//			if (!part.empty() && part != ".")
//				relative /= part;
//
//			if (part_end == std::string::npos)
//				break;
//
//			part_start = part_end + 1;
//		}
//
//		if (relative.empty() || relative.is_absolute() || relative.has_root_name())
//			return std::nullopt;
//
//		return relative;
//	}
//
//	std::optional<std::filesystem::path> GetLoosePath(const char* game_path)
//	{
//		auto relative = GetRelativeLoosePath(game_path);
//		if (!relative.has_value())
//			return std::nullopt;
//
//		for (auto it = loose_file_roots.rbegin(); it != loose_file_roots.rend(); ++it)
//		{
//			auto loose_path = *it / *relative;
//			std::error_code ec{};
//			if (std::filesystem::is_regular_file(loose_path, ec))
//				return loose_path;
//		}
//
//		return std::nullopt;
//	}
//
//	const char* GetPackfileRelativePath(uintptr_t packfile, const char* path)
//	{
//		if (!packfile || !path)
//			return path;
//
//		const auto offset = *(const int32_t*)(packfile + 0x50);
//		if (offset <= 0)
//			return path;
//
//		const auto path_length = std::strlen(path);
//		if ((size_t)offset >= path_length)
//			return path;
//
//		return path + offset;
//	}
//
//	SAFETYHOOK_NOINLINE void PrintFileRequest(const char* request_kind, const char* game_path, const std::optional<std::filesystem::path>& loose_path)
//	{
//		if (!print_file_requests)
//			return;
//
//		char message[2048]{};
//		if (loose_path.has_value())
//		{
//			const auto loose_path_string = loose_path->string();
//			std::snprintf(message, sizeof(message), "[RPF] %s HIT  %s -> %s\n", request_kind, game_path ? game_path : "<null>", loose_path_string.c_str());
//		}
//		else
//		{
//			std::snprintf(message, sizeof(message), "[RPF] %s MISS %s\n", request_kind, game_path ? game_path : "<null>");
//		}
//
//		std::printf("%s", message);
//		std::fflush(stdout);
//		OutputDebugStringA(message);
//	}
//
//	HANDLE OpenLooseFilePath(const char* request_kind, const char* game_path)
//	{
//		auto loose_path = GetLoosePath(game_path);
//		PrintFileRequest(request_kind, game_path, loose_path);
//		if (!loose_path.has_value())
//			return INVALID_HANDLE_VALUE;
//
//		const auto handle = CreateFileW(
//			loose_path->c_str(),
//			GENERIC_READ,
//			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
//			nullptr,
//			OPEN_EXISTING,
//			FILE_ATTRIBUTE_NORMAL,
//			nullptr);
//
//		if (handle != INVALID_HANDLE_VALUE)
//			TrackLooseHandle(handle);
//
//		return handle;
//	}
//
//	HANDLE OpenLooseFile(uintptr_t packfile, const char* game_path)
//	{
//		return OpenLooseFilePath("open_bulk", GetPackfileRelativePath(packfile, game_path));
//	}
//
//	uintptr_t __fastcall PackfileOpenHook(uintptr_t packfile, const char* path, bool read_only)
//	{
//		const auto loose_handle = OpenLooseFilePath("open", path);
//		if (loose_handle != INVALID_HANDLE_VALUE)
//			return (uintptr_t)loose_handle;
//
//		return packfile_open_hook.unsafe_fastcall<uintptr_t>(packfile, path, read_only);
//	}
//
//	uintptr_t __fastcall PackfileOpenBulkHook(uintptr_t packfile, const char* path, uint64_t* out_offset)
//	{
//		const auto loose_handle = OpenLooseFile(packfile, path);
//		if (loose_handle != INVALID_HANDLE_VALUE)
//		{
//			if (out_offset)
//				*out_offset = 0;
//
//			return (uintptr_t)loose_handle;
//		}
//
//		return packfile_open_bulk_hook.unsafe_fastcall<uintptr_t>(packfile, path, out_offset);
//	}
//
//	uintptr_t ReadLooseFile(HANDLE handle, void* buffer, uint32_t size)
//	{
//		DWORD total_read = 0;
//		auto dst = (char*)buffer;
//		uint32_t remaining = size;
//
//		while (remaining != 0)
//		{
//			const DWORD chunk = (DWORD)remaining;
//			DWORD bytes_read = 0;
//			if (!ReadFile(handle, dst, chunk, &bytes_read, nullptr))
//				return (uintptr_t)-1;
//
//			total_read += bytes_read;
//			if (bytes_read != chunk)
//				break;
//
//			dst += bytes_read;
//			remaining -= bytes_read;
//		}
//
//		return total_read;
//	}
//
//	uintptr_t __fastcall PackfileReadHook(uintptr_t packfile, uintptr_t stream, void* buffer, uint32_t size)
//	{
//		{
//			std::scoped_lock lock(loose_file_mutex);
//			if (loose_file_handles.contains(stream))
//				return ReadLooseFile((HANDLE)stream, buffer, size);
//		}
//
//		return packfile_read_hook.unsafe_fastcall<uintptr_t>(packfile, stream, buffer, size);
//	}
//
//	uintptr_t __fastcall PackfileReadBulkHook(uintptr_t packfile, uintptr_t stream, uint64_t offset, void* buffer, uint32_t size)
//	{
//		{
//			std::scoped_lock lock(loose_file_mutex);
//			if (loose_file_handles.contains(stream))
//			{
//				LARGE_INTEGER distance{};
//				distance.QuadPart = offset;
//				if (!SetFilePointerEx((HANDLE)stream, distance, nullptr, FILE_BEGIN))
//					return (uintptr_t)-1;
//
//				return ReadLooseFile((HANDLE)stream, buffer, size);
//			}
//		}
//
//		return packfile_read_bulk_hook.unsafe_fastcall<uintptr_t>(packfile, stream, offset, buffer, size);
//	}
//
//	uintptr_t __fastcall PackfileStreamSizeHook(uintptr_t packfile, uintptr_t stream)
//	{
//		{
//			std::scoped_lock lock(loose_file_mutex);
//			if (loose_file_handles.contains(stream))
//			{
//				LARGE_INTEGER file_size{};
//				if (!GetFileSizeEx((HANDLE)stream, &file_size))
//					return (uintptr_t)-1;
//
//				return (uintptr_t)file_size.QuadPart;
//			}
//		}
//
//		return packfile_stream_size_hook.unsafe_fastcall<uintptr_t>(packfile, stream);
//	}
//
//	uintptr_t __fastcall PackfileSeekHook(uintptr_t packfile, uintptr_t stream, uint32_t offset, int whence)
//	{
//		{
//			std::scoped_lock lock(loose_file_mutex);
//			if (loose_file_handles.contains(stream))
//			{
//				DWORD move_method = FILE_BEGIN;
//				if (whence == 1)
//					move_method = FILE_CURRENT;
//				else if (whence == 2)
//					move_method = FILE_END;
//
//				LARGE_INTEGER distance{};
//				distance.QuadPart = offset;
//
//				LARGE_INTEGER new_position{};
//				if (!SetFilePointerEx((HANDLE)stream, distance, &new_position, move_method))
//					return (uintptr_t)-1;
//
//				return (uintptr_t)new_position.QuadPart;
//			}
//		}
//
//		return packfile_seek_hook.unsafe_fastcall<uintptr_t>(packfile, stream, offset, whence);
//	}
//
//	uintptr_t __fastcall PackfileCloseHook(uintptr_t packfile, uintptr_t stream)
//	{
//		bool was_loose_handle = false;
//		{
//			std::scoped_lock lock(loose_file_mutex);
//			was_loose_handle = loose_file_handles.erase(stream) != 0;
//		}
//
//		if (!was_loose_handle)
//			return packfile_close_hook.unsafe_fastcall<uintptr_t>(packfile, stream);
//
//		CloseHandle((HANDLE)stream);
//		return 0;
//	}
//
//	uintptr_t __fastcall PackfileGetFileSizeHook(uintptr_t packfile, const char* path)
//	{
//		const auto relative_path = GetPackfileRelativePath(packfile, path);
//		auto loose_path = GetLoosePath(relative_path);
//		PrintFileRequest("size", relative_path, loose_path);
//		if (!loose_path.has_value())
//			return packfile_get_file_size_hook.unsafe_fastcall<uintptr_t>(packfile, path);
//
//		std::error_code ec{};
//		const auto size = std::filesystem::file_size(*loose_path, ec);
//		if (ec)
//			return (uintptr_t)-1;
//
//		return (uintptr_t)size;
//	}
//}
//
//class rpf
//{
//public:
//	rpf()
//	{
//		MixTweaks::onInitEvent() += [] 
//			{
//				CIniReader ini{};
//				if (ini.ReadInteger("RPF", "EnableLooseFileLoading", 1) == 0)
//					return;
//
//				print_file_requests = ini.ReadInteger("RPF", "PrintFileRequests", 1) != 0;
//				BuildLooseFileRoots();
//				PrintLooseFileRoots();
//
//				packfile_open_hook = safetyhook::create_inline(RdrAddress(fi_packfile_open), PackfileOpenHook);
//				packfile_open_bulk_hook = safetyhook::create_inline(RdrAddress(fi_packfile_open_bulk), PackfileOpenBulkHook);
//				packfile_read_hook = safetyhook::create_inline(RdrAddress(fi_packfile_read), PackfileReadHook);
//				packfile_read_bulk_hook = safetyhook::create_inline(RdrAddress(fi_packfile_read_bulk), PackfileReadBulkHook);
//				packfile_stream_size_hook = safetyhook::create_inline(RdrAddress(fi_packfile_stream_size), PackfileStreamSizeHook);
//				packfile_seek_hook = safetyhook::create_inline(RdrAddress(fi_packfile_seek), PackfileSeekHook);
//				packfile_close_hook = safetyhook::create_inline(RdrAddress(fi_packfile_close), PackfileCloseHook);
//				packfile_get_file_size_hook = safetyhook::create_inline(RdrAddress(fi_packfile_get_file_size), PackfileGetFileSizeHook);
//			};
//
//	}
//}rpf;
