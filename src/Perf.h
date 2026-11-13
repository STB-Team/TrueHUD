#pragma once

#include <RE/G/GFxValue.h>
#include "Widgets/UpdateCache.h"
#include <span>
#include <cstdint>

#ifndef TRUEHUD_PROFILE
#	define TRUEHUD_PROFILE 0
#endif

namespace Perf
{
	enum class Phase : std::uint8_t
	{
		Frame, Delegate, Native, Actor, Boss, Player, Position, Info, CachedInfo,
		LineOfSight, PlayerRegen, Invoke, Sort, DebugDraw, Flash, CacheDiagnostic, Count
	};

	enum class Counter : std::uint8_t
	{
		Actors, Hidden, Bosses, Custom, DataSent, DataSkipped, SpecialSent,
		SpecialSkipped, PenaltySent, PenaltySkipped, PositionSent, PositionSkipped,
		PositionUnavailable, ClearSent, ClearSkipped, SortSent, InvokeFailures, DepthSent, DepthFailures, Count
	};

	enum class PacketKind : std::uint8_t
	{
		ActorData, BossData, NPCSpecial, PlayerSpecial, Penalty, Position, Count
	};

#if TRUEHUD_PROFILE
	class Scope
	{
	public:
		explicit Scope(Phase a_phase);
		~Scope();
		Scope(const Scope&) = delete;
		Scope& operator=(const Scope&) = delete;
	private:
		Phase _phase;
		std::int64_t _start;
	};

	class Frame
	{
	public:
		explicit Frame(float a_deltaTime);
		~Frame();
		Frame(const Frame&) = delete;
		Frame& operator=(const Frame&) = delete;
	private:
		std::int64_t _start;
	};

	bool UseOptimizations();
	void Count(Counter a_counter, std::uint32_t a_value = 1);
	void Bar(std::uint32_t a_id, unsigned a_state, unsigned a_target, double a_health, double a_maxHealth, bool a_sent);
	void Packet(PacketKind a_kind, std::uint32_t a_id, const Scaleform::CacheDifference& a_difference,
		bool a_ready, bool a_sent, std::span<const double> a_previous, std::span<const double> a_current);
	void PenaltySource(const std::array<double, 4>& a_values);
	void PenaltySourceUnavailable();
#else
	class Scope { public: explicit Scope(Phase) {} };
	class Frame { public: explicit Frame(float) {} };
	constexpr bool UseOptimizations() { return true; }
	inline void Count(Counter, std::uint32_t = 1) {}
	inline void Bar(std::uint32_t, unsigned, unsigned, double, double, bool) {}
#endif

	template <std::size_t N, std::size_t S>
	inline void TracePacket(PacketKind a_kind, std::uint32_t a_id, const Scaleform::UpdateCache<N, S>& a_cache,
		const std::array<double, N>& a_values, bool a_ready, bool a_sent,
		const std::array<std::string_view, S>& a_strings = {})
	{
#if TRUEHUD_PROFILE
		Scope scope(Phase::CacheDiagnostic);
		Packet(a_kind, a_id, a_cache.Difference(a_values, a_strings), a_ready, a_sent, a_cache.AcceptedNumbers(), a_values);
#endif
	}

	inline bool Invoke(RE::GFxValue& a_object, const char* a_name, RE::GFxValue* a_result = nullptr,
		const RE::GFxValue* a_args = nullptr, std::uint32_t a_count = 0)
	{
		Scope scope(Phase::Invoke);
		const bool success = a_object.Invoke(a_name, a_result, a_args, a_count);
		if (!success) {
			Count(Counter::InvokeFailures);
		}
		return success;
	}
}
