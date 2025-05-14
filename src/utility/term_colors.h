// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#ifndef __IDNI_TERM_COLORS_H__
#define __IDNI_TERM_COLORS_H__

// struct term_colors
// this struct contains methods to generate escape codes for terminal colors
// and modifiers. Depending on the member variable 'enabled' it will return
// the escape code or an empty string.
//
// constructor takes an optional argument (bool colors_enabled = true);
// and it is possible to change it later with the methods enable(), disable(),
// toggle() or set(bool e).
//
// term_colors::operator[color c] returns the escape code for the color,
// term_colors::operator(Colors... colors) returns the whole color code and
// it accepts any number of color codes as arguments.
//
// Example usage:
//     cout << TC.BLUE() << "blue text" << TC.CLEAR();
//     cout << TC(BRIGHT, WHITE) << "bright and white text" << TC.CLEAR();
// or manually compose the color code:
//     cout << TC[COLOR] << TC[BG_GREEN] << TC[AND] << TC[WHITE] << TC[EOC]
//            << "green background and white text" << TC[CLEAR];

#include <vector>
#include <string>
#include <sstream>

namespace idni::term {

using color = enum {
	COLOR, AND, EOC, CLEAR,
	BRIGHT, DIM, UNDERLINED,
	BLINK, REVERSE, HIDDEN,
	CLEAR_BRIGHT, CLEAR_DIM, CLEAR_UNDERLINED,
	CLEAR_BLINK, CLEAR_REVERSE, CLEAR_HIDDEN,
	BLACK, RED, GREEN, YELLOW,
	BLUE, MAGENTA, CYAN, LIGHT_GRAY, DEFAULT,
	DARK_GRAY, LIGHT_RED, LIGHT_GREEN, LIGHT_YELLOW,
	LIGHT_BLUE, LIGHT_MAGENTA, LIGHT_CYAN, WHITE,
	BG_BLACK, BG_RED, BG_GREEN, BG_YELLOW,
	BG_BLUE, BG_MAGENTA, BG_CYAN, BG_LIGHT_GRAY, BG_DEFAULT,
	BG_DARK_GRAY, BG_LIGHT_RED, BG_LIGHT_GREEN, BG_LIGHT_YELLOW,
	BG_LIGHT_BLUE, BG_LIGHT_MAGENTA, BG_LIGHT_CYAN, BG_WHITE
};

struct colors {
	colors(bool colors_enabled = true) : enabled(colors_enabled) {}

	bool enabled = true;
	bool enable()    { return enabled = true; }
	bool disable()   { return enabled = false; }
	bool toggle()    { return enabled = !enabled; }
	bool set(bool e) { return enabled = e; }

	// escape controls COLOR = start, AND = separator, EOC = end of color
	std::string COLOR()            const { return (*this)[color::COLOR]; }
	std::string AND()              const { return (*this)[color::AND]; }
	std::string EOC()              const { return (*this)[color::EOC]; }

	// clear/reset all color settings
	std::string CLEAR()            const { return (*this)(color::CLEAR); }
	// set modifiers
	std::string BRIGHT()           const { return (*this)(color::BRIGHT); }
	std::string DIM()              const { return (*this)(color::DIM); }
	std::string UNDERLINED()       const { return (*this)(color::UNDERLINED); }
	std::string BLINK()            const { return (*this)(color::BLINK); }
	std::string REVERSE()          const { return (*this)(color::REVERSE); }
	std::string HIDDEN()           const { return (*this)(color::HIDDEN); }
	// clear modifiers
	std::string CLEAR_BRIGHT()     const { return (*this)(color::CLEAR_BRIGHT); }
	std::string CLEAR_DIM()        const { return (*this)(color::CLEAR_DIM); }
	std::string CLEAR_UNDERLINED() const { return (*this)(color::CLEAR_UNDERLINED); }
	std::string CLEAR_BLINK()      const { return (*this)(color::CLEAR_BLINK); }
	std::string CLEAR_REVERSE()    const { return (*this)(color::CLEAR_REVERSE); }
	std::string CLEAR_HIDDEN()     const { return (*this)(color::CLEAR_HIDDEN); }
	// foregrounds
	std::string DEFAULT()          const { return (*this)(color::DEFAULT); }
	std::string BLACK()            const { return (*this)(color::BLACK); }
	std::string RED()              const { return (*this)(color::RED); }
	std::string GREEN()            const { return (*this)(color::GREEN); }
	std::string YELLOW()           const { return (*this)(color::YELLOW); }
	std::string BLUE()             const { return (*this)(color::BLUE); }
	std::string MAGENTA()          const { return (*this)(color::MAGENTA); }
	std::string CYAN()             const { return (*this)(color::CYAN); }
	std::string LIGHT_GRAY()       const { return (*this)(color::LIGHT_GRAY); }
	std::string DARK_GRAY()        const { return (*this)(color::DARK_GRAY); }
	std::string LIGHT_RED()        const { return (*this)(color::LIGHT_RED); }
	std::string LIGHT_GREEN()      const { return (*this)(color::LIGHT_GREEN); }
	std::string LIGHT_YELLOW()     const { return (*this)(color::LIGHT_YELLOW); }
	std::string LIGHT_BLUE()       const { return (*this)(color::LIGHT_BLUE); }
	std::string LIGHT_MAGENTA()    const { return (*this)(color::LIGHT_MAGENTA); }
	std::string LIGHT_CYAN()       const { return (*this)(color::LIGHT_CYAN); }
	std::string WHITE()            const { return (*this)(color::WHITE); }
	// backgrounds
	std::string BG_DEFAULT()       const { return (*this)(color::BG_DEFAULT); }
	std::string BG_BLACK()         const { return (*this)(color::BG_BLACK); }
	std::string BG_RED()           const { return (*this)(color::BG_RED); }
	std::string BG_GREEN()         const { return (*this)(color::BG_GREEN); }
	std::string BG_YELLOW()        const { return (*this)(color::BG_YELLOW); }
	std::string BG_BLUE()          const { return (*this)(color::BG_BLUE); }
	std::string BG_MAGENTA()       const { return (*this)(color::BG_MAGENTA); }
	std::string BG_CYAN()          const { return (*this)(color::BG_CYAN); }
	std::string BG_LIGHT_GRAY()    const { return (*this)(color::BG_LIGHT_GRAY); }
	std::string BG_DARK_GRAY()     const { return (*this)(color::BG_DARK_GRAY); }
	std::string BG_LIGHT_RED()     const { return (*this)(color::BG_LIGHT_RED); }
	std::string BG_LIGHT_GREEN()   const { return (*this)(color::BG_LIGHT_GREEN); }
	std::string BG_LIGHT_YELLOW()  const { return (*this)(color::BG_LIGHT_YELLOW); }
	std::string BG_LIGHT_BLUE()    const { return (*this)(color::BG_LIGHT_BLUE); }
	std::string BG_LIGHT_MAGENTA() const { return (*this)(color::BG_LIGHT_MAGENTA); }
	std::string BG_LIGHT_CYAN()    const { return (*this)(color::BG_LIGHT_CYAN); }
	std::string BG_WHITE()         const { return (*this)(color::BG_WHITE); }

	// return the code for the color or empty string if colors disabled
	const std::string& operator[](color c) const {
		static const std::vector<std::string> color_code{
			"\x1b[", ";", "m", "0",
			"1", "2", "4",
			"5", "7", "8",
			"21", "22", "24",
			"25", "27", "28",
			"30", "31", "32", "33",
			"34", "35", "36", "37", "39",
			"90", "91", "92", "93",
			"94", "95", "96", "97",
			"40", "41", "42", "43",
			"44", "45", "46", "47", "49",
			"100", "101", "102", "103",
			"104", "105", "106", "107"
		};
		static const std::string no_code{};
		return enabled ? color_code[c] : no_code;
	}
	// return the whole color code or empty string if colors disabled
	template<typename... Colors>
	std::string operator()(Colors... colors) const {
		bool first = true;
		std::stringstream ss; ss << COLOR();
		const auto add = [this, &ss, &first](const auto& c) {
			if (!first) ss << AND(); else first = false;
			ss << (*this)[c];
		};
		return (add(colors), ...), ss << EOC(), ss.str();
	}
};

} // idni::term namespace

namespace idni {

inline static term::colors TC(true);

} // idni namespace

#endif // __IDNI_TERM_COLORS_H__