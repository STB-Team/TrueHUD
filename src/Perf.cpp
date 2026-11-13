#include "Perf.h"

#if TRUEHUD_PROFILE
#include <algorithm>
#include <array>
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
			"cached_info", "los", "player_regen", "invoke", "sort", "debug_draw", "flash"
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
		struct Window
		{
			std::array<Timing, PhaseCount> frame{};
			std::array<Metric, PhaseCount> metrics{};
			std::array<std::uint64_t, CounterCount> counters{};
			std::array<BarSample, 32> bars{};
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
			logger::info("TrueHUDPerf: window={} mode={} tid={} duration_s={:.3f} frames={} samples={} cadence_mean_ms={:.4f} cadence_max_ms={:.4f} nested_elapsed_not_additive=1",
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
			logger::info("TrueHUDPerf: counts actors={} hidden={} bosses={} custom={} data_sent={} data_skipped={} special_sent={} special_skipped={} penalty_sent={} penalty_skipped={} position_sent={} position_skipped={} position_unavailable={} clear_sent={} clear_skipped={} sort_sent={} invoke_failures={}",
				c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7], c[8], c[9], c[10], c[11], c[12], c[13], c[14], c[15], c[16]);
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
}
#endif
