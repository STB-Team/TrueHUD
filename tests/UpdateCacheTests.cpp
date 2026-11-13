#include "Widgets/UpdateCache.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace
{
	void Check(bool a_condition, const char* a_message)
	{
		if (!a_condition) {
			std::cerr << a_message << '\n';
			std::exit(1);
		}
	}
}

int main()
{
	using Cache = Scaleform::UpdateCache<3, 2>;
	Cache cache;
	const Cache::Numbers full{ 100, 100, 1.0 };
	std::string name = "Target";
	std::string regen = "1.00";
	Check(!cache.Matches(full, {name, regen}), "Initial state must be transmitted");
	const auto cold = cache.Difference(full, {name, regen});
	Check(!cold.valid && cold.changed == 0, "A cold cache has no previous packet to blame");
	cache.Store(full, {name, regen});
	Check(cache.Matches(full, {name, regen}), "An identical accepted packet must match");

	// A source string may reallocate or be replaced by the next slow refresh.
	name = std::string(256, 'X');
	Check(!cache.Matches(full, {name, regen}), "A renamed target must invalidate the packet");
	Check(cache.Matches(full, {"Target", regen}), "The cache must own the accepted name");
	regen = "2.00";
	Check(!cache.Matches(full, {"Target", regen}), "Regeneration text changes must be transmitted");

	// The rounded health text can stay constant while the bar visibly changes.
	const Cache::Numbers fractional{ 100, 100, 0.999 };
	Check(!cache.Matches(fractional, {"Target", "1.00"}), "Sub-integer resource changes must update the bar");
	Check(cache.Difference(fractional, {"Target", "1.00"}).changed == (std::uint64_t{1} << 2),
		"Resource percentage changes must identify their own field");
	Check(cache.Difference(full, {"Renamed", "2.00"}).changed == ((std::uint64_t{1} << 3) | (std::uint64_t{1} << 4)),
		"Text fields must not overlap numeric-field mask bits");
	Check(cache.Matches(full, {"Target", "1.00"}), "A failed transmission must not replace the last accepted packet");
	cache.Store(fractional, {"Target", "1.00"});
	Check(!cache.Matches(full, {"Target", "1.00"}), "Healing must not be confused with an older accepted state");

	cache.Reset();
	Check(!cache.Matches(fractional, {"Target", "1.00"}), "Reinitialization must resend unchanged values");

	Scaleform::UpdateCache<1> numeric;
	const auto nan = std::numeric_limits<double>::quiet_NaN();
	numeric.Store({nan});
	Check(!numeric.Matches({nan}), "NaN must retain ordinary comparison semantics, not freeze a Flash update");
	const auto nanDifference = numeric.Difference({nan});
	Check(nanDifference.valid && nanDifference.changed == 1 && nanDifference.nonFinite == 1,
		"An identical NaN is still a cache miss and a non-finite input");
	numeric.Store({0.0});
	Check(numeric.Matches({-0.0}), "Signed zero must not cause redundant Flash updates");
	Check(numeric.Difference({-0.0}).changed == 0, "Diagnostics must not invent a signed-zero cache miss");
	numeric.Store({std::numeric_limits<double>::infinity()});
	Check(numeric.Matches({std::numeric_limits<double>::infinity()}), "Equal infinity must retain ordinary comparison semantics");
	const auto infinity = numeric.Difference({std::numeric_limits<double>::infinity()});
	Check(infinity.changed == 0 && infinity.nonFinite == 1, "Equal infinity is non-finite, not a changed field");
	Check(!numeric.Matches({-std::numeric_limits<double>::infinity()}), "Different infinite values must not compare equal");

	Scaleform::UpdateCache<17, 2> actor;
	Scaleform::UpdateCache<17, 2>::Numbers original{};
	original[2] = 0.5;
	original[10] = 1.0;
	actor.Store(original, {"Target", "1.00"});
	auto moved = original;
	moved[2] = 0.50001;
	Check(actor.Difference(moved, {"Target", "1.00"}).changed == (std::uint64_t{1} << 2),
		"Depth-only motion must remain distinguishable from resource changes");
	moved[10] = 0.9;
	const auto combined = actor.Difference(moved, {"Renamed", "2.00"});
	Check(combined.changed == ((std::uint64_t{1} << 2) | (std::uint64_t{1} << 10) |
		(std::uint64_t{1} << 17) | (std::uint64_t{1} << 18)), "Combined actor reasons must retain every changed field");
	Check(actor.Matches(original, {"Target", "1.00"}) && actor.AcceptedNumbers() == original,
		"Inspecting a diagnostic difference must not accept the observed packet");
	actor.Reset();
	Check(!actor.Difference(moved, {"Renamed", "2.00"}).valid, "Reset must not blame stale accepted values");

	std::cout << "UpdateCache: packet decisions, field reasons, text ownership, reset and floating-point edges passed\n";
}
