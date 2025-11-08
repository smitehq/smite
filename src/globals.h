#pragma once
#include <string>

#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <fmt/color.h>

namespace globals {

	inline constexpr const char* GAME_NAME = "smite.sh";
	inline constexpr const char* GAME_VERSION = "1.0.0";
	inline constexpr const char* PLAYER_NAME = "zaphod";
	inline constexpr const char* HOSTNAME = "lappy486";

	struct style {
		inline static constexpr auto error  = fmt::fg(fmt::color::red) | fmt::emphasis::bold;
		inline static constexpr auto info   = fmt::fg(fmt::color::cyan);
		inline static constexpr auto success= fmt::fg(fmt::color::green) | fmt::emphasis::bold;
		inline static constexpr auto header = fmt::fg(fmt::color::yellow) | fmt::emphasis::bold;
		inline static constexpr auto sword  = u8"🗡️ ";
		inline static constexpr auto smite  = u8"⚡ ";
		inline static constexpr auto clock  = u8"⏱️ ";
		inline static constexpr auto shield  = u8"🛡️ ";
		inline static constexpr auto quest  = u8"🌍 ";
	};

} // namespace globals