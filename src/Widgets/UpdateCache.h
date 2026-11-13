#pragma once

#include <array>
#include <string>
#include <string_view>

namespace Scaleform
{
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

	private:
		Numbers _numbers{};
		std::array<std::string, S> _strings;
		bool _valid = false;
	};
}
