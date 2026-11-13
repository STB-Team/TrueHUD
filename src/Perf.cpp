#include "Perf.h"

#if TRUEHUD_PROFILE
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <chrono>

namespace Perf
{
	namespace
	{
		using Clock = std::chrono::steady_clock;
		constexpr std::size_t PhaseCount = static_cast<std::size_t>(Phase::Count);
		constexpr std::size_t CounterCount = static_cast<std::size_t>(Counter::Count);
		constexpr std::size_t SampleCapacity = 4096;
		constexpr std::int64_t WindowNs = 10'000'000'000;
		constexpr std::array Names{
			"frame", "delegate", "native", "actor", "boss", "player", "position", "info",
			"cached_info", "los", "player_regen", "invoke", "sort", "debug_draw", "flash", "cache_diagnostic"
		};

		std::int64_t Now()
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count();
		}

		struct Timing
		{
			std::uint64_t ns = 0;
			std::uint64_t calls = 0;
		};
		struct Metric
		{
			std::uint64_t ns = 0;
			std::uint64_t calls = 0;
			std::uint64_t maxNs = 0;
			std::array<std::uint64_t, SampleCapacity> samples{};
		};
		struct BarSample
		{
			std::uint32_t id = 0;
			unsigned firstState = 0;
			unsigned lastState = 0;
			unsigned firstTarget = 0;
			unsigned lastTarget = 0;
			double firstHealth = 0;
			double lastHealth = 0;
			double maxHealth = 0;
			std::uint32_t sent = 0;
			std::uint32_t skipped = 0;
			std::uint32_t transitions = 0;
		};
		constexpr std::size_t PacketCount = static_cast<std::size_t>(PacketKind::Count);
		constexpr std::size_t MaxPacketFields = 19;
		constexpr std::array ActorFields{
			"target_type", "widget_state", "depth", "target_state", "level", "level_color", "outline_color", "soul_type",
			"max_health", "health", "health_pct", "max_magicka", "magicka", "magicka_pct",
			"max_stamina", "stamina", "stamina_pct", "name", "regen_health"
		};
		constexpr std::array SpecialFields{ "max_special", "special", "special_pct" };
		constexpr std::array PenaltyFields{ "health_pct", "magicka_pct", "stamina_pct" };
		constexpr std::array PositionFields{ "screen_x", "screen_y", "scale" };
		constexpr std::array SourceFields{ "enabled_global", "health_global", "magicka_global", "stamina_global" };
		struct PacketDescription
		{
			const char* name;
			std::span<const char* const> fields;
			std::uint64_t depthBit;
		};
		constexpr std::array Descriptions{
			PacketDescription{ "actor_data", ActorFields, std::uint64_t{ 1 } << 2 },
			PacketDescription{ "boss_data", ActorFields, std::uint64_t{ 1 } << 2 },
			PacketDescription{ "npc_special", SpecialFields, 0 },
			PacketDescription{ "player_special", SpecialFields, 0 },
			PacketDescription{ "penalty", PenaltyFields, 0 },
			PacketDescription{ "position", PositionFields, 0 }
		};
		static_assert(Descriptions.size() == PacketCount && ActorFields.size() == MaxPacketFields);

		struct NumberExample
		{
			std::uint32_t id = 0;
			double before = 0;
			double after = 0;
			bool previousValid = false;
		};
		struct FieldStats
		{
			std::uint64_t changed = 0;
			std::uint64_t nan = 0;
			std::uint64_t infinity = 0;
			std::uint64_t sameBitsChanged = 0;
			NumberExample first;
			NumberExample last;
			double maxAbsDelta = 0;
			bool sampled = false;
		};
		struct PacketStats
		{
			std::uint64_t observed = 0;
			std::uint64_t sent = 0;
			std::uint64_t warmup = 0;
			std::uint64_t cold = 0;
			std::uint64_t mismatch = 0;
			std::uint64_t bypassEqual = 0;
			std::uint64_t hits = 0;
			std::uint64_t depthOnly = 0;
			std::uint64_t depthAndOther = 0;
			std::uint64_t multipleFields = 0;
			std::uint64_t nonFinitePackets = 0;
			std::array<FieldStats, MaxPacketFields> fields{};
		};

		void RecordNumber(FieldStats& a_field, std::uint32_t a_id, double a_before, double a_after, bool a_valid, bool a_changed)
		{
			a_field.changed += a_changed;
			a_field.nan += std::isnan(a_after);
			a_field.infinity += std::isinf(a_after);
			if (a_valid && a_changed &&
				std::bit_cast<std::uint64_t>(a_before) == std::bit_cast<std::uint64_t>(a_after)) {
				++a_field.sameBitsChanged;
			}
			const NumberExample example{ a_id, a_before, a_after, a_valid };
			if (!a_field.sampled) {
				a_field.first = example;
				a_field.sampled = true;
			}
			a_field.last = example;
			if (a_valid && std::isfinite(a_before) && std::isfinite(a_after)) {
				a_field.maxAbsDelta = std::max(a_field.maxAbsDelta, std::abs(a_after - a_before));
			}
		}

		void ReportField(const char* a_kind, const char* a_name, const FieldStats& a_field)
		{
			logger::info("TrueHUDPerf: cache_field kind={} field={} changed={} nan={} inf={} same_bits_changed={}",
				a_kind, a_name, a_field.changed, a_field.nan, a_field.infinity, a_field.sameBitsChanged);
			if (a_field.sampled) {
				const auto& first = a_field.first;
				const auto& last = a_field.last;
				logger::info("TrueHUDPerf: cache_values kind={} field={} first_id={:08X} first_previous_valid={} first_before={:.17g} first_after={:.17g} last_id={:08X} last_previous_valid={} last_before={:.17g} last_after={:.17g} max_abs_delta={:.17g} first_before_bits={:016X} first_after_bits={:016X} last_before_bits={:016X} last_after_bits={:016X}",
					a_kind, a_name, first.id, first.previousValid, first.before, first.after,
					last.id, last.previousValid, last.before, last.after, a_field.maxAbsDelta,
					std::bit_cast<std::uint64_t>(first.before), std::bit_cast<std::uint64_t>(first.after),
					std::bit_cast<std::uint64_t>(last.before), std::bit_cast<std::uint64_t>(last.after));
			}
		}

		struct Window
		{
			std::array<Timing, PhaseCount> frame{};
			std::array<Metric, PhaseCount> metrics{};
			std::array<std::uint64_t, CounterCount> counters{};
			std::array<BarSample, 32> bars{};
			std::array<PacketStats, PacketCount> packets{};
			std::array<FieldStats, 4> penaltySources{};
			std::uint64_t sourceSamples = 0;
			std::uint64_t sourceEnabled = 0;
			std::uint64_t sourceMissing = 0;
			std::size_t barCount = 0;
			std::uint64_t barOverflow = 0;
			std::uint64_t frames = 0;
			std::uint64_t sequence = 0;
			std::int64_t start = 0;
			double dtSum = 0;
			double maxDt = 0;
			bool active = false;
			bool optimized = false;
		};
		thread_local Window window;

		void Report(std::int64_t a_now)
		{
			auto& w = window;
			const auto reportStart = Now();
			const auto sampleCount = static_cast<std::size_t>(std::min<std::uint64_t>(w.frames, SampleCapacity));
			logger::info("TrueHUDPerf: window={} mode={} tid={} duration_s={:.3f} frames={} samples={} cadence_mean_ms={:.4f} cadence_max_ms={:.4f} nested_elapsed_not_additive=1 schema=3",
				w.sequence, w.optimized ? "optimized" : "baseline", GetCurrentThreadId(), (a_now - w.start) * 1e-9,
				w.frames, sampleCount, w.dtSum * 1000.0 / w.frames, w.maxDt * 1000.0);
			for (std::size_t i = 0; i < PhaseCount; ++i) {
				auto& metric = w.metrics[i];
				if (metric.calls == 0) {
					continue;
				}
				// The window is finished: sort its own samples, without a temporary allocation.
				std::sort(metric.samples.begin(), metric.samples.begin() + sampleCount);
				const auto p95 = (sampleCount * 95 + 99) / 100 - 1;
				const auto p99 = (sampleCount * 99 + 99) / 100 - 1;
				logger::info("TrueHUDPerf: phase={} elapsed_mean_ms_per_frame={:.6f} p95_ms={:.6f} p99_ms={:.6f} max_ms={:.6f} calls={}",
					Names[i], metric.ns * 1e-6 / w.frames, metric.samples[p95] * 1e-6,
					metric.samples[p99] * 1e-6, metric.maxNs * 1e-6, metric.calls);
			}
			const auto& c = w.counters;
			logger::info("TrueHUDPerf: counts actors={} hidden={} bosses={} custom={} data_sent={} data_skipped={} special_sent={} special_skipped={} penalty_sent={} penalty_skipped={} position_sent={} position_skipped={} position_unavailable={} clear_sent={} clear_skipped={} sort_sent={} invoke_failures={} depth_sent={} depth_failures={}",
				c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8], c[9], c[10], c[11], c[12], c[13], c[14], c[15], c[16], c[17], c[18]);
			for (std::size_t i = 0; i < PacketCount; ++i) {
				const auto& packet = w.packets[i];
				const auto& description = Descriptions[i];
				if (packet.observed == 0) {
					continue;
				}
				logger::info("TrueHUDPerf: cache kind={} observed={} sent={} warmup={} cold={} mismatch={} bypass_equal={} hits={} depth_only={} depth_and_other={} multiple_fields={} nonfinite_packets={}",
					description.name, packet.observed, packet.sent, packet.warmup, packet.cold, packet.mismatch,
					packet.bypassEqual, packet.hits, packet.depthOnly, packet.depthAndOther, packet.multipleFields, packet.nonFinitePackets);
				for (std::size_t field = 0; field < description.fields.size(); ++field) {
					ReportField(description.name, description.fields[field], packet.fields[field]);
				}
			}
			if (w.sourceSamples || w.sourceMissing) {
				logger::info("TrueHUDPerf: penalty_source samples={} enabled={} unavailable={}", w.sourceSamples, w.sourceEnabled, w.sourceMissing);
				for (std::size_t i = 0; i < SourceFields.size(); ++i) {
					ReportField("penalty_source", SourceFields[i], w.penaltySources[i]);
				}
			}
			for (std::size_t i = 0; i < w.barCount; ++i) {
				const auto& bar = w.bars[i];
				logger::info("TrueHUDPerf: bar={:08X} state={}->{} target={}->{} hp={:.3f}->{:.3f} max_hp={:.3f} transitions={} sent={} skipped={}",
					bar.id, bar.firstState, bar.lastState, bar.firstTarget, bar.lastTarget,
					bar.firstHealth, bar.lastHealth, bar.maxHealth, bar.transitions, bar.sent, bar.skipped);
			}
			logger::info("TrueHUDPerf: detail_overflow={} report_ms={:.4f} percentile_sample_overflow={}",
				w.barOverflow, (Now() - reportStart) * 1e-6, w.frames > SampleCapacity ? w.frames - SampleCapacity : 0);
		}
	}

	Scope::Scope(Phase a_phase) : _phase(a_phase), _start(Now()) {}
	Scope::~Scope()
	{
		if (window.active) {
			auto& timing = window.frame[static_cast<std::size_t>(_phase)];
			timing.ns += Now() - _start;
			++timing.calls;
		}
	}

	Frame::Frame(float a_deltaTime) : _start(Now())
	{
		auto& w = window;
		if (w.start == 0) {
			w.start = _start;
			w.optimized = (w.sequence % 2) != 0;
		}
		w.frame.fill({});
		w.dtSum += a_deltaTime;
		w.maxDt = std::max(w.maxDt, static_cast<double>(a_deltaTime));
		w.active = true;
	}
	Frame::~Frame()
	{
		auto& w = window;
		const auto end = Now();
		w.frame[static_cast<std::size_t>(Phase::Frame)] = { static_cast<std::uint64_t>(end - _start), 1 };
		w.active = false;
		for (std::size_t i = 0; i < PhaseCount; ++i) {
			auto& metric = w.metrics[i];
			const auto& timing = w.frame[i];
			metric.ns += timing.ns;
			metric.calls += timing.calls;
			metric.maxNs = std::max(metric.maxNs, timing.ns);
			if (w.frames < SampleCapacity) {
				metric.samples[w.frames] = timing.ns;
			}
		}
		++w.frames;
		if (end - w.start >= WindowNs) {
			Report(end);
			++w.sequence;
			w.start = 0;
			w.frames = 0;
			w.dtSum = 0;
			w.maxDt = 0;
			w.counters.fill(0);
			w.barCount = 0;
			w.barOverflow = 0;
			w.packets.fill({});
			w.penaltySources.fill({});
			w.sourceSamples = w.sourceEnabled = w.sourceMissing = 0;
			for (auto& metric : w.metrics) {
				metric.ns = metric.calls = metric.maxNs = 0;
			}
		}
	}

	bool UseOptimizations() { return !window.active || window.optimized; }
	void Count(Counter a_counter, std::uint32_t a_value)
	{
		if (window.active) {
			window.counters[static_cast<std::size_t>(a_counter)] += a_value;
		}
	}

	void Bar(std::uint32_t a_id, unsigned a_state, unsigned a_target, double a_health, double a_maxHealth, bool a_sent)
	{
		auto& w = window;
		if (!w.active) {
			return;
		}
		std::size_t i = 0;
		while (i < w.barCount && w.bars[i].id != a_id) {
			++i;
		}
		if (i == w.barCount) {
			if (i == w.bars.size()) {
				++w.barOverflow;
				return;
			}
			w.bars[i] = { a_id, a_state, a_state, a_target, a_target, a_health, a_health, a_maxHealth };
			++w.barCount;
		}
		auto& bar = w.bars[i];
		bar.transitions += bar.lastState != a_state || bar.lastTarget != a_target;
		bar.lastState = a_state;
		bar.lastTarget = a_target;
		bar.lastHealth = a_health;
		bar.maxHealth = a_maxHealth;
		a_sent ? ++bar.sent : ++bar.skipped;
	}
	void Packet(PacketKind a_kind, std::uint32_t a_id, const Scaleform::CacheDifference& a_difference,
		bool a_ready, bool a_sent, std::span<const double> a_previous, std::span<const double> a_current)
	{
		auto& w = window;
		if (!w.active) {
			return;
		}
		const auto index = static_cast<std::size_t>(a_kind);
		const auto& description = Descriptions[index];
		auto& packet = w.packets[index];
		++packet.observed;
		packet.sent += a_sent;
		packet.nonFinitePackets += a_difference.nonFinite != 0;
		if (!a_ready) {
			++packet.warmup;
		} else if (!a_difference.valid) {
			++packet.cold;
		} else if (a_difference.changed) {
			++packet.mismatch;
		} else if (a_sent) {
			++packet.bypassEqual;
		} else {
			++packet.hits;
		}
		if (a_difference.changed) {
			packet.multipleFields += std::popcount(a_difference.changed) > 1;
			if (description.depthBit && (a_difference.changed & description.depthBit)) {
				a_difference.changed == description.depthBit ? ++packet.depthOnly : ++packet.depthAndOther;
			}
		}
		for (std::size_t i = 0; i < description.fields.size(); ++i) {
			const auto bit = std::uint64_t{ 1 } << i;
			const bool changed = (a_difference.changed & bit) != 0;
			auto& field = packet.fields[i];
			if (i < a_current.size()) {
				if (!a_difference.valid || changed || (a_difference.nonFinite & bit) || a_kind == PacketKind::Penalty) {
					RecordNumber(field, a_id, a_previous[i], a_current[i], a_difference.valid, changed);
				}
			} else {
				field.changed += changed;
			}
		}
	}

	void PenaltySource(const std::array<double, 4>& a_values)
	{
		Scope scope(Phase::CacheDiagnostic);
		auto& w = window;
		if (!w.active) {
			return;
		}
		++w.sourceSamples;
		w.sourceEnabled += a_values[0] == 1;
		for (std::size_t i = 0; i < a_values.size(); ++i) {
			auto& field = w.penaltySources[i];
			const bool valid = field.sampled;
			const double previous = field.last.after;
			RecordNumber(field, 0, previous, a_values[i], valid, valid && previous != a_values[i]);
		}
	}

	void PenaltySourceUnavailable()
	{
		if (window.active) {
			++window.sourceMissing;
		}
	}
}
#endif
