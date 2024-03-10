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

// class term_colors TC. constructor takes argument (bool colors_enabled = true);
// use: os << TC.BLUE() << "blue text" << TC.CLEAR();
// or:  os << TC(BRIGHT, WHITE) << "bright and white text" << TC.CLEAR();

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
	bool enable()    { return enabled = true; }
	bool disable()   { return enabled = false; }
	bool toggle()    { return enabled = !enabled; }
	bool set(bool e) { return enabled = e; }
	bool enabled = true;
	std::string COLOR()            { return get(color::COLOR); }
	std::string AND()              { return get(color::AND); }
	std::string EOC()              { return get(color::EOC); }
	std::string BRIGHT()           { return get(color::BRIGHT); }
	std::string UNDERLINED()       { return get(color::UNDERLINED); }
	std::string BLINK()            { return get(color::BLINK); }
	std::string CLEAR()            { return getc(color::CLEAR); }
	// foregrounds
	std::string DEFAULT()          { return getc(color::DEFAULT); }
	std::string BLACK()            { return getc(color::BLACK); }
	std::string RED()              { return getc(color::RED); }
	std::string GREEN()            { return getc(color::GREEN); }
	std::string YELLOW()           { return getc(color::YELLOW); }
	std::string BLUE()             { return getc(color::BLUE); }
	std::string MAGENTA()          { return getc(color::MAGENTA); }
	std::string CYAN()             { return getc(color::CYAN); }
	std::string LIGHT_GRAY()       { return getc(color::LIGHT_GRAY); }
	std::string DARK_GRAY()        { return getc(color::DARK_GRAY); }
	std::string LIGHT_RED()        { return getc(color::LIGHT_RED); }
	std::string LIGHT_GREEN()      { return getc(color::LIGHT_GREEN); }
	std::string LIGHT_YELLOW()     { return getc(color::LIGHT_YELLOW); }
	std::string LIGHT_BLUE()       { return getc(color::LIGHT_BLUE); }
	std::string LIGHT_MAGENTA()    { return getc(color::LIGHT_MAGENTA); }
	std::string LIGHT_CYAN()       { return getc(color::LIGHT_CYAN); }
	std::string WHITE()            { return getc(color::WHITE); }
	// backgrounds
	std::string BG_DEFAULT()       { return getc(color::BG_DEFAULT); }
	std::string BG_BLACK()         { return getc(color::BG_BLACK); }
	std::string BG_RED()           { return getc(color::BG_RED); }
	std::string BG_GREEN()         { return getc(color::BG_GREEN); }
	std::string BG_YELLOW()        { return getc(color::BG_YELLOW); }
	std::string BG_BLUE()          { return getc(color::BG_BLUE); }
	std::string BG_MAGENTA()       { return getc(color::BG_MAGENTA); }
	std::string BG_CYAN()          { return getc(color::BG_CYAN); }
	std::string BG_LIGHT_GRAY()    { return getc(color::BG_LIGHT_GRAY); }
	std::string BG_DARK_GRAY()     { return getc(color::BG_DARK_GRAY); }
	std::string BG_LIGHT_RED()     { return getc(color::BG_LIGHT_RED); }
	std::string BG_LIGHT_GREEN()   { return getc(color::BG_LIGHT_GREEN); }
	std::string BG_LIGHT_YELLOW()  { return getc(color::BG_LIGHT_YELLOW); }
	std::string BG_LIGHT_BLUE()    { return getc(color::BG_LIGHT_BLUE); }
	std::string BG_LIGHT_MAGENTA() { return getc(color::BG_LIGHT_MAGENTA); }
	std::string BG_LIGHT_CYAN()    { return getc(color::BG_LIGHT_CYAN); }
	std::string BG_WHITE()         { return getc(color::BG_WHITE); }

	const std::string& get(color c) {
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
			"44", "45", "46", "47", "49"
			"100", "101", "102", "103",
			"104", "105", "106", "107"
		};
		static const std::string no_code{};
		return enabled ? color_code[c] : no_code;
	}
	std::string getc(color c) {
		std::stringstream ss;
		return ss << COLOR() << get(c) << EOC(), ss.str();
	}
	std::string getc(color c1, color c2) {
		std::stringstream ss;
		return ss << COLOR() << get(c1) << AND() << get(c2) << EOC(),
			ss.str();
	}
	std::string getc(color c1, color c2, color c3) {
		std::stringstream ss;
		return ss << COLOR() << get(c1)	<< AND() << get(c2)
			<< AND() << get(c3) << EOC(), ss.str();
	}
	std::string operator()(color c) { return getc(c); }
	std::string operator()(color c1, color c2) { return getc(c1, c2); }
	std::string operator()(color c1, color c2, color c3) {
						return getc(c1, c2, c3); }
};

} // idni::term namespace
#endif // __IDNI_TERM_COLORS_H__