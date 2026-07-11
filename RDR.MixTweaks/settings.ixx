module;

#include "common.hxx"
#include <Windows.h>
#include <MemoryMgr.h>
#include <safetyhook.hpp>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <iostream>
export module settings;

import common;

import comvars;

export class RDRSettingsRegistry
{
public:
	enum class OptionsList
	{
		Gameplay,
		Display,
		Graphics,
		Audio,
		Gamepad,
		KeyboardMouse,
		//Legal
	};

	enum class OptionType
	{
		Bool,
		Int,
		Float
	};

	using SyncToIntCallback = int(__fastcall*)();
	using SyncFromIntCallback = void(__fastcall*)(int value);
	using SyncToFloatCallback = float(__fastcall*)();
	using SyncFromFloatCallback = void(__fastcall*)(float value);

	struct Option
	{
		OptionType type{};
		std::string optionsListId;
		std::string treeId;
		std::string optionId;
		uint32_t treeHash{};
		uint32_t optionHash{};
		std::string fallbackName;
		std::vector<std::string> labels;
		SyncToIntCallback syncToIntOptions{};
		SyncFromIntCallback syncFromIntOptions{};
		SyncToFloatCallback syncToFloatOptions{};
		SyncFromFloatCallback syncFromFloatOptions{};
	};

	RDRSettingsRegistry()
	{
		Instance() = this;

		//AddOptionBool(
		//	"SnowTree",
		//	"Snow",
		//	"Snow at high heights",
		//	SyncSnowToOptions,
		//	SyncSnowFromOptions
		//);

		MixTweaks::onInitEvent() += []() {
			if (Instance())
				Instance()->InstallHooks();
		};
	}

	Option& AddOptionBool(
		std::string_view treeId,
		std::string_view optionId,
		std::string_view fallbackName,
		SyncToIntCallback syncToOptions,
		SyncFromIntCallback syncFromOptions,
		OptionsList optionsList = OptionsList::Gameplay)
	{
		return AddOptionBool(GetOptionsListId(optionsList), treeId, optionId, fallbackName, syncToOptions, syncFromOptions);
	}

	Option& AddOptionBool(
		OptionsList optionsList,
		std::string_view treeId,
		std::string_view optionId,
		std::string_view fallbackName,
		SyncToIntCallback syncToOptions,
		SyncFromIntCallback syncFromOptions)
	{
		return AddOptionBool(treeId, optionId, fallbackName, syncToOptions, syncFromOptions, optionsList);
	}

	Option& AddOptionBool(
		std::string_view optionsListId,
		std::string_view treeId,
		std::string_view optionId,
		std::string_view fallbackName,
		SyncToIntCallback syncToOptions,
		SyncFromIntCallback syncFromOptions)
	{
		return AddOption(
			OptionType::Bool,
			optionsListId,
			treeId,
			optionId,
			fallbackName,
			{ "Common_Off", "Common_On" },
			syncToOptions,
			syncFromOptions
		);
	}

	Option& AddOptionInt(
		std::string_view treeId,
		std::string_view optionId,
		std::string_view fallbackName,
		std::initializer_list<const char*> labels,
		SyncToIntCallback syncToOptions,
		SyncFromIntCallback syncFromOptions,
		OptionsList optionsList = OptionsList::Gameplay)
	{
		return AddOptionInt(GetOptionsListId(optionsList), treeId, optionId, fallbackName, labels, syncToOptions, syncFromOptions);
	}

	Option& AddOptionInt(
		OptionsList optionsList,
		std::string_view treeId,
		std::string_view optionId,
		std::string_view fallbackName,
		std::initializer_list<const char*> labels,
		SyncToIntCallback syncToOptions,
		SyncFromIntCallback syncFromOptions)
	{
		return AddOptionInt(treeId, optionId, fallbackName, labels, syncToOptions, syncFromOptions, optionsList);
	}

	Option& AddOptionInt(
		std::string_view optionsListId,
		std::string_view treeId,
		std::string_view optionId,
		std::string_view fallbackName,
		std::initializer_list<const char*> labels,
		SyncToIntCallback syncToOptions,
		SyncFromIntCallback syncFromOptions)
	{
		return AddOption(OptionType::Int, optionsListId, treeId, optionId, fallbackName, labels, syncToOptions, syncFromOptions);
	}

	Option& AddOptionFloat(
		std::string_view treeId,
		std::string_view optionId,
		std::string_view fallbackName,
		std::initializer_list<const char*> labels,
		SyncToFloatCallback syncToOptions,
		SyncFromFloatCallback syncFromOptions,
		OptionsList optionsList = OptionsList::Gameplay)
	{
		return AddOptionFloat(GetOptionsListId(optionsList), treeId, optionId, fallbackName, labels, syncToOptions, syncFromOptions);
	}

	Option& AddOptionFloat(
		OptionsList optionsList,
		std::string_view treeId,
		std::string_view optionId,
		std::string_view fallbackName,
		std::initializer_list<const char*> labels,
		SyncToFloatCallback syncToOptions,
		SyncFromFloatCallback syncFromOptions)
	{
		return AddOptionFloat(treeId, optionId, fallbackName, labels, syncToOptions, syncFromOptions, optionsList);
	}

	Option& AddOptionFloat(
		std::string_view optionsListId,
		std::string_view treeId,
		std::string_view optionId,
		std::string_view fallbackName,
		std::initializer_list<const char*> labels,
		SyncToFloatCallback syncToOptions,
		SyncFromFloatCallback syncFromOptions)
	{
		return AddOption(OptionType::Float, optionsListId, treeId, optionId, fallbackName, labels, syncToOptions, syncFromOptions);
	}

	const Option* FindOptionByHash(uint32_t hash) const
	{
		const auto it = m_optionsByHash.find(hash);
		if (it != m_optionsByHash.end())
			return &m_options[it->second];

		return nullptr;
	}

	const char* FindFallbackString(uint32_t hash) const
	{
		const auto* option = FindOptionByHash(hash);
		if (!option || option->fallbackName.empty())
			return nullptr;

		return option->fallbackName.c_str();
	}

	void ModifyOptionsXml(std::string& xml) const
	{
		for (const auto& optionsListId : GetRegisteredOptionsLists())
		{
			const auto listRange = FindScrollableList(xml, optionsListId);
			if (listRange.close == std::string::npos)
				continue;

			std::string insertion;
			for (const auto& option : m_options)
			{
				if (option.optionsListId != optionsListId)
					continue;

				if (ContainsInRange(xml, listRange.open, listRange.close, option.treeId))
					continue;

				AppendOptionXml(insertion, option);
			}

			if (insertion.empty())
				continue;

			if (!ContainsInRange(xml, listRange.open, listRange.close, "OptionsHeader_Mix"))
			{
				std::string withHeader;
				AppendMixHeaderXml(withHeader);
				withHeader.append(insertion);
				insertion = std::move(withHeader);
			}

			xml.insert(listRange.close, insertion);
		}
	}

	SAFETYHOOK_NOINLINE void SyncToOptions()
	{
		for (const auto& option : m_options)
		{
			const auto object = GetOptionObject(option);
			if (!object)
				continue;

			switch (option.type)
			{
			case OptionType::Bool:
			case OptionType::Int:
				if (option.syncToIntOptions)
					vftable_call<void>(reinterpret_cast<void*>(object), 1128, option.syncToIntOptions(), 0);
				break;
			case OptionType::Float:
				if (option.syncToFloatOptions)
					vftable_call<void>(reinterpret_cast<void*>(object), 1128, option.syncToFloatOptions(), 0);
				break;
			}
		}
	}

	SAFETYHOOK_NOINLINE void SyncFromOptions()
	{
		for (const auto& option : m_options)
		{
			const auto object = GetOptionObject(option);
			if (!object)
				continue;

			switch (option.type)
			{
			case OptionType::Bool:
			case OptionType::Int:
				if (option.syncFromIntOptions)
					option.syncFromIntOptions(vftable_call<int>(reinterpret_cast<void*>(object), 1040, 0));
				break;
			case OptionType::Float:
				if (option.syncFromFloatOptions)
					option.syncFromFloatOptions(vftable_call<float>(reinterpret_cast<void*>(object), 1040, 0));
				break;
			}
		}
	}

	const std::vector<Option>& Options() const
	{
		return m_options;
	}

private:
	struct ScrollableListRange
	{
		size_t open{ std::string::npos };
		size_t close{ std::string::npos };
	};

	static constexpr std::string_view GetOptionsListId(OptionsList optionsList)
	{
		switch (optionsList)
		{
		case OptionsList::Gameplay:
			return "OptionsList_Controls";
		case OptionsList::Display:
			return "OptionsList_Display";
		case OptionsList::Graphics:
			return "OptionsList_Graphics";
		case OptionsList::Audio:
			return "OptionsList_Audio";
		case OptionsList::Gamepad:
			return "OptionsList_Gamepad";
		case OptionsList::KeyboardMouse:
			return "OptionsList_KBM";
		//case OptionsList::Legal:
		//	return "OptionsList_Legal";
		}

		return "OptionsList_Controls";
	}

	static std::string MakeGameOptionId(std::string_view name)
	{
		constexpr std::string_view prefix = "GameOption_";

		if (name.starts_with(prefix))
			return std::string(name);

		std::string result(prefix);
		result.append(name);
		return result;
	}

	Option& AddOption(
		OptionType type,
		std::string_view optionsListId,
		std::string_view treeId,
		std::string_view optionId,
		std::string_view fallbackName,
		std::initializer_list<const char*> labels,
		SyncToIntCallback syncToOptions,
		SyncFromIntCallback syncFromOptions)
	{
		auto fullOptionId = MakeGameOptionId(optionId);
		auto fullTreeId = MakeGameOptionId(treeId);
		const auto optionHash = rage::atStringHash(fullOptionId.c_str());
		const auto treeHash = rage::atStringHash(fullTreeId.c_str());

		if (const auto it = m_optionsByHash.find(optionHash); it != m_optionsByHash.end())
		{
			auto& option = m_options[it->second];
			option.type = type;
			option.optionsListId = optionsListId;
			option.treeId = std::move(fullTreeId);
			option.optionId = std::move(fullOptionId);
			option.treeHash = treeHash;
			option.optionHash = optionHash;
			option.fallbackName = fallbackName;
			option.labels.assign(labels.begin(), labels.end());
			option.syncToIntOptions = syncToOptions;
			option.syncFromIntOptions = syncFromOptions;
			return option;
		}

		auto& option = m_options.emplace_back();
		option.type = type;
		option.optionsListId = optionsListId;
		option.treeId = std::move(fullTreeId);
		option.optionId = std::move(fullOptionId);
		option.treeHash = treeHash;
		option.optionHash = optionHash;
		option.fallbackName = fallbackName;
		option.labels.assign(labels.begin(), labels.end());
		option.syncToIntOptions = syncToOptions;
		option.syncFromIntOptions = syncFromOptions;
		m_optionsByHash.emplace(option.optionHash, m_options.size() - 1);
		return option;
	}

	Option& AddOption(
		OptionType type,
		std::string_view optionsListId,
		std::string_view treeId,
		std::string_view optionId,
		std::string_view fallbackName,
		std::initializer_list<const char*> labels,
		SyncToFloatCallback syncToOptions,
		SyncFromFloatCallback syncFromOptions)
	{
		auto& option = AddOption(type, optionsListId, treeId, optionId, fallbackName, labels, SyncToIntCallback{}, SyncFromIntCallback{});
		option.syncToFloatOptions = syncToOptions;
		option.syncFromFloatOptions = syncFromOptions;
		return option;
	}

	std::vector<std::string> GetRegisteredOptionsLists() const
	{
		std::vector<std::string> optionsLists;

		for (const auto& option : m_options)
		{
			bool found = false;
			for (const auto& optionsListId : optionsLists)
			{
				if (optionsListId == option.optionsListId)
				{
					found = true;
					break;
				}
			}

			if (!found)
				optionsLists.emplace_back(option.optionsListId);
		}

		return optionsLists;
	}

	static bool ContainsInRange(const std::string& text, size_t begin, size_t end, std::string_view needle)
	{
		if (begin == std::string::npos || end == std::string::npos || begin >= end)
			return false;

		const auto pos = text.find(needle, begin);
		return pos != std::string::npos && pos < end;
	}

	static ScrollableListRange FindScrollableList(const std::string& xml, std::string_view listId)
	{
		constexpr std::string_view openNeedle = "<UIScrollableList";
		constexpr std::string_view closeNeedle = "</UIScrollableList>";

		size_t search = 0;
		while (true)
		{
			const auto open = xml.find(openNeedle, search);
			if (open == std::string::npos)
				return {};

			const auto openEnd = xml.find('>', open);
			if (openEnd == std::string::npos)
				return {};

			const auto tag = std::string_view(xml).substr(open, openEnd - open + 1);
			if (tag.find(listId) == std::string_view::npos)
			{
				search = openEnd + 1;
				continue;
			}

			size_t depth = 1;
			size_t scan = openEnd + 1;
			while (depth != 0)
			{
				const auto nextOpen = xml.find(openNeedle, scan);
				const auto nextClose = xml.find(closeNeedle, scan);
				if (nextClose == std::string::npos)
					return {};

				if (nextOpen != std::string::npos && nextOpen < nextClose)
				{
					++depth;
					scan = nextOpen + openNeedle.size();
					continue;
				}

				--depth;
				if (depth == 0)
					return { open, nextClose };

				scan = nextClose + closeNeedle.size();
			}
		}
	}

	static void AppendMixHeaderXml(std::string& xml)
	{
		xml.append(
			"\n"
			"\t  \t<UIList horizontal=\"true\" scrollable=\"false\" allowInput=\"false\">\n"
			"\t\t\t<UIHeader text=\"OptionsHeader_Mix\"></UIHeader>\n"
			"\t\t\t<UIOption></UIOption>\n"
			"\t\t</UIList>\n"
		);
	}

	static void AppendOptionXml(std::string& xml, const Option& option)
	{
		xml.append("\t\t<UIList id=\"");
		xml.append(option.treeId);
		xml.append("\" horizontal=\"true\" scrollable=\"false\" allowInput=\"false\" >\n");
		xml.append("\t\t  <onfocused expr=\"focus(");
		xml.append(option.optionId);
		xml.append(")\"></onfocused>\n");
		xml.append("\t\t  <UILabel text=\"");
		xml.append(option.optionId);
		xml.append("\"></UILabel>\n");
		xml.append("\t\t  <UIOption id=\"");
		xml.append(option.optionId);
		xml.append("\" allowWrap=\"true\">\n");

		for (const auto& label : option.labels)
		{
			xml.append("\t\t\t<UILabel text=\"");
			xml.append(label);
			xml.append("\"></UILabel>\n");
		}

		xml.append(
			"\t\t  </UIOption>\n"
			"\t\t</UIList>\n"
		);
	}

	static bool IsOptionsXmlName(const char* name)
	{
		return name && (_stricmp(name, "options.sc") == 0 || _stricmp(name, "Options.sc.xml") == 0);
	}

	static uintptr_t GetOptionObject(const Option& option)
	{
		if (!rage::UIFactory::sm_Instance || !*rage::UIFactory::sm_Instance)
			return 0;

		return rage::UIFactory::GetUIObjectByIdName(*rage::UIFactory::sm_Instance, option.optionId.c_str());
	}

	void InstallHooks()
	{
		auto pattern = hook::pattern("48 85 C0 74 ? 40 38 B7");
		if (!pattern.empty())
		{
			static auto sync_to_options = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&) {
				if (Instance())
					Instance()->SyncToOptions();
			});
		}

		pattern = hook::pattern("48 85 C0 74 ? ? ? ? 33 D2 4C 8B 81 ? ? ? ? 48 8B C8 41 FF D0 85 C0 0F 95 C0 88 83 ? ? ? ? 48 8B 5C 24");
		if (!pattern.empty())
		{
			static auto sync_from_options = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext&) {
				if (Instance())
					Instance()->SyncFromOptions();
			});
		}

		static auto options_xml_hook = hooks::pattern_mid("41 39 6E ? 75", [](SafetyHookContext& ctx) {
			auto* instance = Instance();
			const char* name = reinterpret_cast<const char*>(ctx.r13);
			if (!instance || !IsOptionsXmlName(name))
				return;

			const auto cacheEntry = static_cast<uintptr_t>(ctx.rdi);
			char* xmlBuf = *reinterpret_cast<char**>(cacheEntry + 0x08);
			const int xmlLen = *reinterpret_cast<int*>(cacheEntry + 0x10);
			if (!xmlBuf || xmlLen <= 0)
				return;

			std::string modifiedXml(xmlBuf, static_cast<size_t>(xmlLen));
			instance->ModifyOptionsXml(modifiedXml);

			const auto modifiedLen = modifiedXml.size();
			auto* modifiedBuf = static_cast<char*>(rage::sysMemAllocator::Alloc(modifiedLen + 1));
			if (!modifiedBuf)
				return;

			std::memcpy(modifiedBuf, modifiedXml.data(), modifiedLen);
			modifiedBuf[modifiedLen] = '\0';

			*reinterpret_cast<char**>(cacheEntry + 0x08) = modifiedBuf;
			*reinterpret_cast<int*>(cacheEntry + 0x10) = static_cast<int>(modifiedLen);

			// TODO PLS VERY OLD BUFFER ITS NOT NEEDED

			//std::cout << modifiedXml << std::endl << modifiedXml.size();

		});
	}

	static RDRSettingsRegistry*& Instance()
	{
		static RDRSettingsRegistry* instance{};
		return instance;
	}

	std::vector<Option> m_options;
	std::unordered_map<uint32_t, size_t> m_optionsByHash;
};

export inline RDRSettingsRegistry RDRSettings;
