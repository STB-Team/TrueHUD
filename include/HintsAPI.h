#pragma once

#include <Windows.h>
#include <string>
#include <unordered_map>
#include <vector>

// ======================== STB Hints Framework — Public API ========================
//
// Подключите этот заголовок в вашем SKSE плагине чтобы показывать подсказки.
//
// Использование:
//   1. Скопируйте HintsAPI.h в ваш проект
//   2. В коде:
//      #include "HintsAPI.h"
//
//      // Один раз при загрузке плагина (в SKSEPlugin_Load или позже):
//      HintsAPI::Init();
//
//      // Показать подсказку по ID:
//      HintsAPI::ShowHint("my_hint_id");
//
//

namespace HintsAPI
{
	// Типы функций
		using SetVisibilityFunc = void (*)(bool visible);
	using IsHintShowingFunc = bool (*)();
	using IsWidgetActiveFunc = bool (*)();
	using ShowHintFunc = void (*)(const char* id);
	using WasHintShownFunc = bool (*)(const char* id);
	// Инициализация — ищет STB Hints Framework DLL и резолвит функции
	// Возвращает true если фреймворк найден
	inline bool Init();
inline bool WasHintShown(const std::string& id);
inline bool IsHintShowing();
inline bool IsWidgetActive();
	inline void SetVisibility(bool visible);
	// Показать подсказку по ID (без проверки условий)
	inline void ShowHint(const std::string& id);

	// ======================== Реализация ========================

	namespace Detail
	{
		inline SetVisibilityFunc g_SetVisibility = nullptr;
		inline IsHintShowingFunc g_IsHintShowing = nullptr;
		inline IsWidgetActiveFunc g_IsWidgetActive = nullptr;
		inline ShowHintFunc g_showHint = nullptr;
		inline WasHintShownFunc g_wasHintHint = nullptr;
		inline bool g_initialized = false;
	}

	inline bool Init()
	{
		if (Detail::g_initialized)
			return Detail::g_showHint != nullptr;

		Detail::g_initialized = true;
	
		auto handle = GetModuleHandle(L"SKSEHintsFramework.dll");
		if (!handle) {
			return false;
		}
		Detail::g_SetVisibility = reinterpret_cast<SetVisibilityFunc>(GetProcAddress(handle, "HintsAPI_SetVisibility"));
		Detail::g_IsHintShowing = reinterpret_cast<IsHintShowingFunc>(GetProcAddress(handle, "HintsAPI_IsHintShowing"));
		Detail::g_IsWidgetActive = reinterpret_cast<IsWidgetActiveFunc>(GetProcAddress(handle, "HintsAPI_IsWidgetActive"));
		Detail::g_showHint = reinterpret_cast<ShowHintFunc>(GetProcAddress(handle, "HintsAPI_ShowHint"));
		Detail::g_wasHintHint = reinterpret_cast<WasHintShownFunc>(GetProcAddress(handle, "HintsAPI_WasHintShown"));
		return Detail::g_showHint != nullptr;
	}

		inline bool WasHintShown(const std::string& id)
	{
		if (!Detail::g_initialized)
			Init();
		return Detail::g_wasHintHint(id.c_str());
	}
			inline bool IsHintShowing()
	{
		if (!Detail::g_initialized)
			Init();
		return Detail::g_IsHintShowing();
	}
			inline bool IsWidgetActive()
	{
		if (!Detail::g_initialized)
			Init();
		return Detail::g_IsWidgetActive();
	}
		inline void SetVisibility(bool visible)
	{
		if (!Detail::g_initialized)
			Init();
		if (Detail::g_SetVisibility)
			Detail::g_SetVisibility(visible);
	}
	inline void ShowHint(const std::string& id)
	{
		if (!Detail::g_initialized)
			Init();
		if (Detail::g_showHint)
			Detail::g_showHint(id.c_str());
	}
}
