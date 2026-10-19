// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TGF__PARSER_GEN_H__
#define __IDNI__PARSER__TGF__PARSER_GEN_H__
#include <filesystem>
#include <string_view>
#include <cmath>
#include "parser.h"
#include "grammar_inspector.h"
#include "utility/escapes.h"

namespace idni {

struct parser_gen_options {
	std::string output_dir                       = "";
	std::string output                           = "";
	std::string name                             = "";
	std::string ns                               = "";
	std::string char_type                        = "char";
	std::string terminal_type                    = "char";
	std::string decoder                          = "";
	std::string encoder                          = "";
	// true sets the char type char, the terminal type char32_t and an
	// empty decoder and encoder to the utf8 recoders
	bool utf8                                    = false;
	// false emits productions() into a companion .cpp instead of the header
	bool header_only                             = true;
	// true adds a matcher() member built on idni::treemr::matcher_for
	bool treemr                                  = false;
};

/// Write the parser of @p g. The value lists the files it wrote. A file
/// that does not open or write is an error, and no later file is written.
template <typename C = char, typename T = C>
idni::diagnostics::result<std::vector<std::string>> generate_parser_cpp(
	const std::string& tgf_filename, const grammar<C, T>& g,
	parser_gen_options opt = {})
{
	using idni::parser_strings::label;
	using idni::parser_strings::messages;
	idni::diagnostics::result<std::vector<std::string>> r;
	if (g.size() == 0) return r.with_value(std::vector<std::string>{});
	auto file_error = [&r](std::string_view msg, const std::string& path)
		-> idni::diagnostics::result<std::vector<std::string>>
	{
		return r.with_error(idni::diagnostics::code::io_error, msg,
			{{ label::path, path }});
	};

	auto strip_pwd = [](const std::string& p) {
		auto pwd = std::filesystem::current_path().string();
		if (pwd.back() != '/') pwd += '/';
		if (p.compare(0, pwd.size(), pwd) == 0)
			return p.substr(pwd.size());
		return p;
	};
	auto strip_path = [](const std::string& p) {
		auto pos = p.rfind('/');
		return (pos == std::string::npos) ? p : p.substr(pos + 1);
	};
	auto strip_filename = [](const std::string& p) {
		auto pos = p.rfind('/');
		return (pos == std::string::npos) ? "" : p.substr(0, pos);
	};
	auto strip_ext = [](const std::string& p) {
		auto pos = p.rfind('.');
		return (pos == std::string::npos) ? p : p.substr(0, pos);
	};

	std::string tgf_filename_stripped = strip_pwd(tgf_filename);
	std::string basename = strip_ext(strip_path(tgf_filename_stripped));
	if (opt.name.size() == 0) opt.name = basename + "_parser";
	if (opt.output_dir.size() == 0) opt.output_dir = strip_filename(tgf_filename);
	if (opt.output_dir.size() && opt.output_dir.back() != '/')
		opt.output_dir += '/';
	if (opt.output.size() == 0) opt.output = opt.name + ".generated.h";
	if (opt.utf8) {
		opt.char_type = "char";
		opt.terminal_type = "char32_t";
		if (opt.decoder.empty()) opt.decoder = "idni::utf8_to_u32_conv";
		if (opt.encoder.empty()) opt.encoder = "idni::u32_to_utf8_conv";
	}

	const std::string header = opt.output_dir + opt.output;
	std::ofstream os(header);
	if (!os) return file_error(messages::cannot_open_file, header);

	// convert name to upper case and append __PARSER_GEN_H__ as a guard at the begining
	std::string guard = opt.name;
	std::transform(guard.begin(), guard.end(), guard.begin(), ::toupper);

	std::string U = opt.terminal_type == "char32_t" ? "U" : "";
	std::vector<C> ts{ (C)0 };
	grammar_inspector<C, T> gi(g);

	auto gen_ts = [&ts, &U]() {
		std::stringstream os;
		os << "\t";
		size_t n = 0;
		for (const C ch : ts) {
			if (++n == 10) n = 0, os << "\n\t";
			os << U << "'" <<
				( ch == (C)    0 ? "\\0"
				: ch == (C) '\r' ? "\\r"
				: ch == (C) '\n' ? "\\n"
				: ch == (C) '\t' ? "\\t"
				: ch == (C) '\\' ? "\\\\"
				: ch == (C) '\'' ? "\\'"
				: to_std_string(ch)) << "', ";
		}
		os << "\n";
		return os.str();
	};
	auto gen_nts_enum_cte = [&gi]() {
		std::stringstream os;
		auto& x = gi.nts();
		for (size_t n = 0; n != x.size(); ++n)
			os << (n % 10 == 0 ? "\n\t\t" : "") <<
				(n == 0 ? "nul" : to_std_string(x[n]))
				<< ", ";
		os << "\n";
		return os.str();
	};
	auto gen_nts = [&gi]() {
		std::stringstream os;
		auto& x = gi.nts();
		for (size_t n = 0; n != x.size(); ++n)
			os << (n % 10 == 0 ? "\n\t" : "") <<
				"\"" << to_std_string(x[n]) << "\", ";
		os << "\n";
		return os.str();
	};
	// the names, and the renames only when the grammar has one, so a
	// grammar without renames keeps the two-argument call
	auto gen_cc_fns = [&gi]() {
		std::stringstream os, renames;
		auto& x = gi.nts();
		const auto& renamed = gi.cc_fns().renamed;
		for (const auto& fn : gi.cc_fns().fns) {
			if (gi.cc_fns().is_derived(fn.first)) continue;
			if (!x[fn.first].size()) continue;
			auto it = renamed.find(fn.first);
			if (it == renamed.end()) os << "\t\t\""
				<< to_std_string(x[fn.first]) << "\",\n";
			else renames << "\t\t{ \"" << it->second << "\", \""
				<< to_std_string(x[fn.first]) << "\" },\n";
		}
		if (!renames.str().empty())
			os << "\t}, {\n" << renames.str();
		return os.str();
	};
	auto gen_grammar_opts = [&g]() {
		std::stringstream os;
		std::array<const char*, 2> pbool = { "false", "true" };
		auto plist = [](std::ostream& os,
			const std::set<size_t>& list) -> std::ostream&
		{
			size_t i = 0;
			for (const auto& s : list) os << (i ? "," : "")
				<< (i % 10 == 0 ? "\n\t\t\t" : " ") << s, i++;
			return os;
		};
		auto pvlist = [](std::ostream& os,
			const std::set<std::vector<size_t>>& vlist)
				-> std::ostream&
		{
			size_t i = 0;
			for (const auto& v : vlist) {
				os << (i++ ? "," : "") << "\n\t\t\t{ ";
				size_t j = 0;
				for (const auto& s : v)
					os << (j++ ? ", " : "") << s;
				os << " }";
			}
			return os;
		};
		os << "\t.auto_disambiguate = "
			<< pbool[g.opt().auto_disambiguate] << ",\n";
		if (g.opt().nodisambig_list.size()) {
			os << "\t.nodisambig_list = {";
			plist(os, g.opt().nodisambig_list) << "\n\t},\n";
		}
		os << "\t.shaping = {\n";
		if (g.opt().shaping.to_trim.size()) {
			os << "\t\t.to_trim = {";
			plist(os, g.opt().shaping.to_trim) << "\n\t\t},\n";
		}
		if (g.opt().shaping.to_trim_children.size()) {
			os << "\t\t.to_trim_children = {";
			plist(os, g.opt().shaping.to_trim_children)<<"\n\t\t},\n";
		}
		if (g.opt().shaping.to_trim_children_terminals.size()) {
			os << "\t\t.to_trim_children_terminals = {";
			plist(os, g.opt().shaping.to_trim_children_terminals)
								<< "\n\t\t},\n";
		}
		os << "\t\t.trim_terminals = "
			<< pbool[g.opt().shaping.trim_terminals] << ",\n";
		if (g.opt().shaping.dont_trim_terminals_of.size()) {
			os << "\t\t.dont_trim_terminals_of = {";
			plist(os, g.opt().shaping.dont_trim_terminals_of)
				<< "\n\t\t},\n";
		}
		if (g.opt().shaping.to_inline.size()) {
			os << "\t\t.to_inline = {";
			pvlist(os, g.opt().shaping.to_inline) << "\n\t\t},\n";
		}
		os << "\t\t.inline_char_classes = "
			<< pbool[g.opt().shaping.inline_char_classes] << "\n";
		os << "\t}";
		if (g.opt().enabled_guards.size()) {
			os << ",\n\t.enabled_guards = {";
			size_t i = 0;
			for (const auto& s : g.opt().enabled_guards) os << (i ? "," : "")
				<< (i % 10 == 0 ? "\n\t\t" : " ")
				<< "\"" << s << "\"", i++;
			os << "\n\t}";
		}
		if (g.opt().dynamic.size()) {
			os << ",\n\t.dynamic = {";
			size_t i = 0;
			for (const auto& [name, values] : g.opt().dynamic) {
				os << (i++ ? "," : "") << "\n\t\t{ \""
					<< escapes::encode(to_std_string(name),
						escapes::c_like)
					<< "\", ";
				if (values.empty()) os << "{}";
				else {
					os << "{ ";
					size_t j = 0;
					for (const auto& v : values)
						os << (j++ ? ", " : "")
							<< "\""
							<< escapes::encode(
								to_std_string(v),
								escapes::c_like)
							<< "\"";
					os << " }";
				}
				os << " }";
			}
			os << "\n\t}";
		}
		auto quoted = [](const std::string& v) {
			return "\"" + escapes::encode(v, escapes::c_like) + "\"";
		};
		// a host selects a profile by name, so the profiles go with the guards
		if (g.opt().variants.size()) {
			os << ",\n\t.variants = {";
			size_t i = 0;
			for (const auto& [name, guards] : g.opt().variants) {
				os << (i++ ? "," : "") << "\n\t\t{ "
					<< quoted(name) << ", { ";
				size_t j = 0;
				for (const auto& guard : guards)
					os << (j++ ? ", " : "") << quoted(guard);
				os << " } }";
			}
			os << "\n\t}";
		}
		if (g.opt().variant.size())
			os << ",\n\t.variant = " << quoted(g.opt().variant);
		os << "\n";
		return os.str();
	};
	auto gen_opts = [&opt]() {
		std::stringstream os;
		if (opt.decoder.size()) os << "\to.codec.decode = "
			<< opt.decoder << ";\n";
		if (opt.encoder.size()) os << "\to.codec.encode = "
			<< opt.encoder << ";\n";
		return os.str();
	};
	auto gen_prods = [&g, &gi, &ts]() {
		std::stringstream os;
		// every cached A => ch production is synthetic, whatever class
		// made it, so keep it out of the generated source
		std::set<size_t> cc_prods = gi.cc_char_prod_ids();
		for (const auto& ps_nt : gi.cc_fns().ps)
			for (const auto& ps_ch : ps_nt.second)
				cc_prods.insert(ps_ch.second);
		auto terminal = [&ts](const lit<C, T>& l) {
			for (size_t n = 0; n != ts.size(); ++n)
				if (ts[n] == l.t()) return n;
			ts.push_back(l.t());
			return ts.size() - 1;
		};
		for (size_t i = 0; i != gi.G().size(); ++i) {
			if (cc_prods.count(i)) continue;
			const auto& p = gi.G()[i];
			g.print_production(os << "//", i, true) << "\n";
			os << "\tp(NT(" << p.first.n() << "), ";
			bool first_c = true;
			for (const auto& c : p.second) {
				if (!first_c) os << " & "; else first_c = false;
				if (c.neg) os << '~';
				os << '(';
				bool first_l = true;
				for (const auto& l : c) {
					if (!first_l) os << "+";
					else first_l = false;
					if (l.nt()) os << "NT(" << l.n() << ")";
					else if (l.is_null()) os << "nul";
					else os << "T(" << terminal(l) << ")";
				}
				os << ')';
			}
			os << ");\n";
			if (auto it = gi.grdm().find(i); it != gi.grdm().end())
				os << "\tp.back().guard = \""
					<< gi.guards()[it->second] << "\";\n";
		}
		return os.str();
	};
	const auto ps = gen_prods();
	os <<	"// This file is generated from a file " <<
					tgf_filename_stripped << " by\n"
		"//       https://github.com/IDNI/parser/src/tgf\n"
		"//\n"
		"#ifndef __" << guard << "_H__\n"
		"#define __" << guard << "_H__\n"
		"\n"
		"#include \"parser.h\"\n";
	if ((opt.decoder.size() && opt.decoder.starts_with("idni::")) ||
		(opt.encoder.size() && opt.encoder.starts_with("idni::")))
			os << "#include \"recoders.h\"\n";
	if (opt.treemr) os << "#include \"format/treemr/treemr.h\"\n";
	os <<	"\n";
	if (opt.ns.size()) os << "namespace " << opt.ns << " {\n\n";
	os << "namespace " << opt.name << "_data {\n\n";
	size_t nt_bits = static_cast<size_t>(std::ceil(std::log2(gi.nts().size())));
	os <<
		"using char_type     = " << opt.char_type << ";\n"
		"using terminal_type = " << opt.terminal_type << ";\n"
		"\n"
		"inline static constexpr size_t nt_bits = " << nt_bits << ";\n"
		"inline const std::vector<std::string> symbol_names{"
			<< gen_nts() <<
		"};\n"
		"\n"
		"inline ::idni::nonterminals<char_type, terminal_type> nts{symbol_names};\n"
		"\n"
		"inline std::vector<terminal_type> terminals{\n"
			<< gen_ts() <<
		"};\n"
		"\n"
		"inline ::idni::char_class_fns<terminal_type> char_classes =\n"
		"	::idni::predefined_char_classes<char_type, terminal_type>({\n"
			<< gen_cc_fns() <<
		"	}, nts);\n"
		"\n"
		"inline struct ::idni::grammar<char_type, terminal_type>::options\n"
		"	grammar_options\n"
		"{\n"
		"	.transform_negation = false,\n"
			<< gen_grammar_opts() <<
		"};\n"
		"\n"
		"inline auto make_parser_options() {\n"
		"\tauto o = ::idni::default_parser_options"
			"<char_type, terminal_type>();\n"
			<< gen_opts() <<
		"\treturn o;\n"
		"}\n"
		"\n"
		"inline ::idni::prods<char_type, terminal_type> start_symbol{ nts("
						<< gi.start().n() << ") };\n"
		"\n";

	// grammar table; inline in the header it lands in every including TU
	const std::string productions_body =
		"	static bool loaded = false;\n"
		"	static idni::prods<char_type, terminal_type>\n"
		"		p, nul(idni::lit<char_type, terminal_type>{});\n"
		"	if (loaded) return p;\n"
		"	#define  T(x) (idni::prods<char_type, terminal_type>{ terminals[x] })\n"
		"	#define NT(x) (idni::prods<char_type, terminal_type>{ nts(x) })\n"
		+ ps +
		"	#undef T\n"
		"	#undef NT\n"
		"	return loaded = true, p;\n";

	if (opt.header_only) os <<
		"inline idni::prods<char_type, terminal_type>& productions() {\n"
			<< productions_body <<
		"}\n";
	else os <<
		"#ifdef TAU_PARSER_BUILD_HEADER_ONLY\n"
		"inline idni::prods<char_type, terminal_type>& productions() {\n"
			<< productions_body <<
		"}\n"
		"#else\n"
		"idni::prods<char_type, terminal_type>& productions();\n"
		"#endif\n";

	os <<	"\n"
		"inline ::idni::grammar<char_type, terminal_type> grammar(\n"
		"	nts, productions(), start_symbol, char_classes, grammar_options);\n"
		"\n"
		"} // namespace " << opt.name << "_data\n"
		"\n"
		"struct " << opt.name << "_nonterminals {\n"
		"	enum nonterminal {"
			<< gen_nts_enum_cte() <<
		"	};\n"
		"};\n"
		"\n"
		"#ifdef _MSC_VER\n"
		"// the base parser puts its chart indexes on a cache line, so the padding is wanted\n"
		"#\tpragma warning(push)\n"
		"#\tpragma warning(disable : 4324)\n"
		"#endif\n"
		"struct " << opt.name << " : public idni::parser<"
			<< opt.char_type << ", " << opt.terminal_type << ">, "
			"public " << opt.name << "_nonterminals {\n"
		"	static " << opt.name << "& instance() {\n"
		"		static " << opt.name << " inst;\n"
		"		return inst;\n"
		"	}\n"
		"	" << opt.name << "() : idni::parser<char_type, terminal_type>(\n"
		"		" << opt.name << "_data::grammar,\n"
		"		" << opt.name << "_data::make_parser_options()) {}\n"
		"	size_t id(const std::basic_string<char_type>& name) {\n"
		"		return " << opt.name << "_data::nts.get(name);\n"
		"	}\n"
		"	const std::basic_string<char_type>& name(size_t id) {\n"
		"		return " << opt.name << "_data::nts.get(id);\n"
		"	}\n"
		"	symbol_type literal(const nonterminal& nt) {\n"
		"		return symbol_type(nt, &" << opt.name << "_data::nts);\n"
		"	}\n";
	if (opt.treemr) os <<
		"	auto matcher(std::string_view pattern) const {\n"
		"		return ::idni::treemr::matcher_for(*this, pattern);\n"
		"	}\n";
	os <<	"};\n"
		"#ifdef _MSC_VER\n"
		"#\tpragma warning(pop)\n"
		"#endif\n"
		"\n";
	if (opt.ns.size()) os << "\n} // " <<opt.ns<< " namespace\n";
	os <<	"#endif // __" << guard << "_H__\n";
	// a full disk or a lost device shows only when the buffer goes out
	os.close();
	if (!os) return file_error(messages::cannot_write_file, header);

	if (opt.header_only) return r.with_value(
		std::vector<std::string>{ header });

	// companion translation unit holding the grammar table, compiled once
	const std::string cpp_name = opt.name + ".generated.cpp";
	const std::string source = opt.output_dir + cpp_name;
	std::ofstream cs(source);
	if (!cs) return file_error(messages::cannot_open_file, source);
	cs <<	"// This file is generated from a file " <<
					tgf_filename_stripped << " by\n"
		"//       https://github.com/IDNI/parser/src/tgf\n"
		"//\n"
		"// productions() lives here so the table is compiled once, "
			"not per TU.\n"
		"//\n"
		// parser.h first, so its guard opens before this header's
		// does: tgf.h re-includes this header, and if this header's
		// guard opens first that nested include short-circuits early
		"#include \"parser.h\"\n"
		"#include \"" << opt.output << "\"\n"
		"\n";
	if (opt.ns.size()) cs << "namespace " << opt.ns << " {\n\n";
	cs <<	"namespace " << opt.name << "_data {\n"
		"\n"
		"#ifndef TAU_PARSER_BUILD_HEADER_ONLY\n"
		"idni::prods<char_type, terminal_type>& productions() {\n"
			<< productions_body <<
		"}\n"
		"#endif\n"
		"\n"
		"} // namespace " << opt.name << "_data\n";
	if (opt.ns.size()) cs << "\n} // " << opt.ns << " namespace\n";
	cs.close();
	if (!cs) return file_error(messages::cannot_write_file, source);
	return r.with_value(std::vector<std::string>{ header, source });
}

// generate_parser_cpp_from_* returns a result<vector<string>> whose report
// holds the report of the load and of the generator. On success the value
// lists the files it wrote. On failure the value is absent and the report
// carries the errors. Caller prints the report once at the end via
// print_and_ok().
template <typename C = char, typename T = C>
idni::diagnostics::result<std::vector<std::string>>
generate_parser_cpp_from_string(
	const std::string& tgf_filename,
	const std::basic_string<C>& grammar_tgf,
	parser_gen_options opt = {},
	bool measure = false)
{
	nonterminals<C, T> nts;
	auto gr = tgf<C, T>::from_string(nts, grammar_tgf, measure);
	idni::diagnostics::result<std::vector<std::string>> r;
	auto g = r.merge_take(std::move(gr));
	if (!g) return r;
	auto files = r.merge_take(generate_parser_cpp(tgf_filename, *g, opt));
	if (files) r.emplace(std::move(*files));
	return r;
}

template <typename C = char, typename T = C>
idni::diagnostics::result<std::vector<std::string>>
generate_parser_cpp_from_file(
	const std::string& tgf_filename,
	parser_gen_options opt = {},
	bool measure = false)
{
	nonterminals<C, T> nts;
	auto gr = tgf<C, T>::from_file(nts, tgf_filename, measure);
	idni::diagnostics::result<std::vector<std::string>> r;
	auto g = r.merge_take(std::move(gr));
	if (!g) return r;
	auto files = r.merge_take(generate_parser_cpp(tgf_filename, *g, opt));
	if (files) r.emplace(std::move(*files));
	return r;
}

} // idni namespace
#endif //__IDNI__PARSER__TGF__PARSER_GEN_H__
