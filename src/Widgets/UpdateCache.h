#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

namespace Scaleform
{
	struct CacheDifference
	{
		std::uint64_t changed = 0;
		std::uint64_t nonFinite = 0;
		bool valid = false;
	};

	// Retains exactly what the movie accepted, not pointers into mutable source strings.
	template <std::size_t N, std::size_t S = 0>
	class UpdateCache
	{
	public:
		using Numbers = std::array<double, N>;
		using Strings = std::array<std::string_view, S>;

		bool Matches(const Numbers& a_numbers, const Strings& a_strings = {}) const
		{
			if (!_valid || _numbers != a_numbers) {
				return false;
			}
			for (std::size_t i = 0; i < S; ++i) {
				if (_strings[i] != a_strings[i]) {
					return false;
				}
			}
			return true;
		}

		void Store(const Numbers& a_numbers, const Strings& a_strings = {})
		{
			_numbers = a_numbers;
			for (std::size_t i = 0; i < S; ++i) {
				if (_strings[i] != a_strings[i]) {
					_strings[i] = a_strings[i];
				}
			}
			_valid = true;
		}

		void Reset() { _valid = false; }

		// Diagnostic-only inspection: ordinary floating-point equality, including NaN,
		// must agree with Matches; inspecting a packet never accepts it.
		CacheDifference Difference(const Numbers& a_numbers, const Strings& a_strings = {}) const
		{
			static_assert(N + S <= 64);
			CacheDifference result{ 0, 0, _valid };
			for (std::size_t i = 0; i < N; ++i) {
				const auto bit = std::uint64_t{ 1 } << i;
				if (!std::isfinite(a_numbers[i])) {
					result.nonFinite |= bit;
				}
				if (_valid && _numbers[i] != a_numbers[i]) {
					result.changed |= bit;
				}
			}
			if (_valid) {
				for (std::size_t i = 0; i < S; ++i) {
					if (_strings[i] != a_strings[i]) {
						result.changed |= std::uint64_t{ 1 } << (N + i);
					}
				}
			}
			return result;
		}

		const Numbers& AcceptedNumbers() const { return _numbers; }

	private:
		Numbers _numbers{};
		std::array<std::string, S> _strings;
		bool _valid = false;
	};
}
