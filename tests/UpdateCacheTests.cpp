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
	Check(cache.Matches(full, {"Target", "1.00"}), "A failed transmission must not replace the last accepted packet");
	cache.Store(fractional, {"Target", "1.00"});
	Check(!cache.Matches(full, {"Target", "1.00"}), "Healing must not be confused with an older accepted state");

	cache.Reset();
	Check(!cache.Matches(fractional, {"Target", "1.00"}), "Reinitialization must resend unchanged values");

	Scaleform::UpdateCache<1> numeric;
	const auto nan = std::numeric_limits<double>::quiet_NaN();
	numeric.Store({nan});
	Check(!numeric.Matches({nan}), "NaN must retain ordinary comparison semantics, not freeze a Flash update");
	numeric.Store({0.0});
	Check(numeric.Matches({-0.0}), "Signed zero must not cause redundant Flash updates");
	numeric.Store({std::numeric_limits<double>::infinity()});
	Check(numeric.Matches({std::numeric_limits<double>::infinity()}), "Equal infinity must retain ordinary comparison semantics");
	Check(!numeric.Matches({-std::numeric_limits<double>::infinity()}), "Different infinite values must not compare equal");

	std::cout << "UpdateCache: resource changes, text ownership, reset and floating-point edges passed\n";
}
