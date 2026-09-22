// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

// Runner for tgf.test.tgf-shaped test files: a matcher in a .tgf.test
// entry is treemr DSL text, read verbatim from source and handed to
// treemr::compile unchanged.

#ifndef __IDNI__PARSER__FORMAT__TGF_TEST__TGF_TEST_TMPL_H__
#define __IDNI__PARSER__FORMAT__TGF_TEST__TGF_TEST_TMPL_H__

#include "tgf_test.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

#include "utility/escapes.h"

namespace idni {

template <typename C, typename T>
template <typename Parser>
typename tgf_test<C, T>::result
tgf_test<C, T>::run_from_string(Parser& p, const std::basic_string<C>& s)
{
	auto& tp = tgf_test_parser::instance();
	auto r = tp.parse(s.c_str(), s.size());
	if (!r.found) {
		std::cerr << "TGF test: "
			<< r.parse_error.to_str(tgf_test_parser::error::
				info_lvl::INFO_BASIC) << "\n";
		return { 1, 0, 0 };
	}
	tref root = r.get_shaped_tree2();

	run_ctx ctx(p.get_grammar());
	int ret = 0;
	auto entries = trv(root) || tgf_test_nt::entry;
	for (const auto& entry : entries()) {
		if (run_entry(p, ctx, entry, /*parent_text*/ "",
			/*start_nt*/ 0, default_ambig_mode,
			/*inherited_raw*/ false, /*inherited_negate*/ false))
			ret = 1;
	}
	std::cout << ctx.passed << " passed, " << ctx.failed
		<< " failed\n";
	return { ret, ctx.passed, ctx.failed };
}

template <typename C, typename T>
template <typename Parser>
typename tgf_test<C, T>::result
tgf_test<C, T>::run_from_file(Parser& p, const std::string& filename) {
	std::cout << "opening file: " << filename << std::endl;
	std::ifstream ifs(filename);
	if (!ifs) {
		std::cerr << "cannot open file: " << filename << std::endl;
		return { 1, 0, 0 };
	}
	return run_from_string(p, std::string(
		std::istreambuf_iterator<C>(ifs),
		std::istreambuf_iterator<C>()));
}

template <typename C, typename T>
template <typename Parser>
int tgf_test<C, T>::run_entry(Parser& p, run_ctx& ctx, const trv& entry,
	const std::string& parent_text, size_t start_nt,
	treemr::ambig_mode inherited_mode, bool inherited_raw,
	bool inherited_negate)
{
	bool negate = inherited_negate
		|| (entry | tgf_test_nt::negate).has_value();

	auto winfo = parse_words(entry | tgf_test_nt::words);
	if (winfo.conflict) {
		std::cerr << "test entry: conflicting words\n";
		return 1;
	}
	treemr::ambig_mode mode = winfo.mode.value_or(inherited_mode);
	bool raw = inherited_raw || winfo.raw;

	std::string own_text = matcher_text(entry | tgf_test_nt::matcher);

	std::string full_text;
	size_t effective_start = start_nt;
	if (parent_text.empty()) {
		// Outermost block: derive the start symbol from the
		// matcher's leading nonterminal.
		auto nm = leading_name(own_text);
		if (!nm) {
			std::cerr << "test entry: matcher must start "
				"with a nonterminal\n";
			return 1;
		}
		auto rn = resolve_nt(p, ctx, *nm);
		if (!rn) return 1;
		effective_start = *rn;
		full_text = own_text;
	} else {
		full_text = parent_text + " > " + own_text;
	}

	int ret = 0;

	auto tail = entry | tgf_test_nt::entry_tail;
	auto il = tail | tgf_test_nt::item_list;
	if (il.has_value()) {
		auto items = il || tgf_test_nt::item;
		for (const auto& it : items()) {
			if (run_item(p, ctx, it, full_text, effective_start,
				mode, raw, negate))
				ret = 1;
		}
	}

	auto nb = tail | tgf_test_nt::nested_block;
	if (nb.has_value()) {
		auto sub_entries = nb || tgf_test_nt::entry;
		for (const auto& sub : sub_entries()) {
			if (run_entry(p, ctx, sub, full_text, effective_start,
				mode, raw, negate))
				ret = 1;
		}
	}

	return ret;
}

template <typename C, typename T>
template <typename Parser>
int tgf_test<C, T>::run_item(Parser& p, run_ctx& ctx, const trv& item,
	const std::string& entry_text, size_t start_nt,
	treemr::ambig_mode entry_mode, bool entry_raw, bool entry_negate)
{
	bool negate = entry_negate
		|| (item | tgf_test_nt::negate).has_value();

	auto winfo = parse_words(item | tgf_test_nt::words);
	treemr::ambig_mode mode = winfo.mode.value_or(entry_mode);
	bool raw = entry_raw || winfo.raw;

	std::string err;
	std::string input = read_input(item | tgf_test_nt::input, err);

	auto per_item = item | tgf_test_nt::matcher;
	std::string full_text = per_item.has_value()
		? entry_text + " > " + matcher_text(per_item)
		: entry_text;

	std::cout << "\t\"" << input << "\"\t\t";

	if (winfo.conflict) {
		std::cout << "test item: conflicting words\n";
		++ctx.failed;
		return 1;
	}
	if (!err.empty()) {
		std::cout << "FAIL (bad escape in input: " << err << ")\n";
		++ctx.failed;
		return 1;
	}

	using parse_options = typename Parser::parse_options;
	parse_options ppo{ .start = start_nt };
	auto r = p.parse(input.c_str(), input.size(), ppo);

	bool positive = false;
	bool hard_error = false;
	std::string fail_reason;

	if (!r.found) {
		std::ostringstream oss;
		oss << "FAIL (parse error: " << r.parse_error << ")";
		fail_reason = oss.str();
	} else {
		tref root = raw ? r.get_bintree() : r.get_shaped_tree2();
		if (!root) {
			fail_reason = "FAIL (tree root is null after "
				"shaping, use @raw)";
			hard_error = true;
		} else {
			auto cit = ctx.compiled.find(full_text);
			if (cit == ctx.compiled.end()) {
				auto cr = treemr::compile(full_text);
				if (!cr.has_value()) {
					std::ostringstream oss;
					cr.report().print(oss);
					fail_reason = "FAIL (matcher compile "
						"error for \"" + full_text +
						"\": " + oss.str() + ")";
					hard_error = true;
				} else cit = ctx.compiled.emplace(full_text,
					std::move(cr).value()).first;
			}
			if (fail_reason.empty()) {
				treemr::matcher<pnode_type<C, T>> m(
					cit->second,
					treemr::parse_node_adapter<C, T>(
						ctx.gi.nts()));
				if (m.match(root, mode)) positive = true;
				else fail_reason = "FAIL (tree shape "
					"mismatch)";
			}
		}
	}

	// A compile error or missing tree fails outright: `~` inverts a
	// parse/shape mismatch only, never a setup failure.
	if (hard_error) {
		std::cout << fail_reason << "\n";
		++ctx.failed;
		return 1;
	}

	if (negate) {
		if (positive) {
			std::cout << "FAIL (negated, but parsed and "
				"matched)\n";
			++ctx.failed;
			return 1;
		}
		std::cout << "OK (negated)\n";
		++ctx.passed;
		return 0;
	}
	if (positive) {
		std::cout << "OK\n";
		++ctx.passed;
		return 0;
	}
	std::cout << fail_reason << "\n";
	++ctx.failed;
	return 1;
}

// `words => word (__ word)*`
template <typename C, typename T>
typename tgf_test<C, T>::words_info
tgf_test<C, T>::parse_words(const trv& words_trv) {
	words_info out;
	if (!words_trv.has_value()) return out;
	auto ws = words_trv || tgf_test_nt::word;
	for (const auto& w : ws()) {
		if ((w | tgf_test_nt::w_raw).has_value()) {
			out.raw = true;
			continue;
		}
		treemr::ambig_mode m = treemr::ambig_mode::ANY;
		if ((w | tgf_test_nt::w_forbid).has_value())
			m = treemr::ambig_mode::FORBID;
		else if ((w | tgf_test_nt::w_unique).has_value())
			m = treemr::ambig_mode::UNIQUE;
		else if ((w | tgf_test_nt::w_all).has_value())
			m = treemr::ambig_mode::ALL;
		else if ((w | tgf_test_nt::w_any).has_value())
			m = treemr::ambig_mode::ANY;
		else continue; // unreachable for a valid parse
		if (out.mode.has_value() && *out.mode != m) {
			out.conflict = true;
			continue;
		}
		out.mode = m;
	}
	return out;
}

// The matcher's terminals reproduce its source text exactly, spaces
// included, since @trim keeps every terminal under this subtree.
template <typename C, typename T>
std::string tgf_test<C, T>::matcher_text(const trv& matcher_trv) {
	if (!matcher_trv.has_value()) return {};
	return matcher_trv | trv::terminals;
}

// The start symbol of a top-level entry is the leading identifier of
// its matcher text, after an optional '/' root anchor and whitespace.
template <typename C, typename T>
std::optional<std::string> tgf_test<C, T>::leading_name(
	const std::string& text)
{
	size_t i = 0;
	auto is_space = [](char c) { return std::isspace(
		static_cast<unsigned char>(c)); };
	while (i < text.size() && is_space(text[i])) ++i;
	if (i < text.size() && text[i] == '/') {
		++i;
		while (i < text.size() && is_space(text[i])) ++i;
	}
	size_t start = i;
	auto is_name_start = [](char c) {
		return std::isalpha(static_cast<unsigned char>(c))
			|| c == '_';
	};
	auto is_name_cont = [](char c) {
		return std::isalnum(static_cast<unsigned char>(c))
			|| c == '_';
	};
	if (i >= text.size() || !is_name_start(text[i]))
		return std::nullopt;
	++i;
	while (i < text.size() && is_name_cont(text[i])) ++i;
	return text.substr(start, i - start);
}

// Looks up `name` among the grammar's known nonterminals first, so an
// unknown name fails the entry instead of grammar::nt() silently
// allocating a new nonterminal for it.
template <typename C, typename T>
template <typename Parser>
std::optional<size_t> tgf_test<C, T>::resolve_nt(Parser& p, run_ctx& ctx,
	const std::string& name)
{
	using C2 = typename Parser::char_type;
	auto s = from_str<C2>(name);
	auto& nts = ctx.gi.nts();
	if (std::find(nts.begin(), nts.end(), s) == nts.end()) {
		std::cerr << "test entry: unknown nonterminal: "
			<< name << "\n";
		return std::nullopt;
	}
	return p.get_grammar().nt(s).n();
}

// Decodes a quoted_string parse-tree node into its literal text.
template <typename C, typename T>
std::string tgf_test<C, T>::decode_quoted_string(const trv& qs,
	std::string& err)
{
	std::string out;
	auto chars = qs || tgf_test_nt::quoted_string_char;
	for (const auto& qc : chars.traversers()) {
		auto only = qc | trv::only_child;
		auto nt = only | trv::nonterminal;
		if (nt == (size_t)tgf_test_nt::unescaped_s) {
			out += only | trv::terminals;
		} else {
			out += decode_escape(
				only | trv::terminals, err);
			if (!err.empty()) return out;
		}
	}
	return out;
}

template <typename C, typename T>
std::string tgf_test<C, T>::decode_escape(const std::string& raw,
	std::string& err)
{
	auto dec = idni::escapes::decode(raw,
		idni::escapes::tgf_string);
	if (!dec.has_value()) {
		std::ostringstream oss;
		dec.report().print(oss);
		err = oss.str();
		return {};
	}
	return std::move(dec).value();
}

template <typename C, typename T>
std::string tgf_test<C, T>::read_input(const trv& in_trv, std::string& err) {
	auto qs = in_trv | tgf_test_nt::quoted_string;
	if (qs.has_value()) return decode_quoted_string(qs, err);
	auto nm = in_trv | tgf_test_nt::name;
	return nm | trv::terminals;
}

} // namespace idni

#endif // __IDNI__PARSER__FORMAT__TGF_TEST__TGF_TEST_TMPL_H__
