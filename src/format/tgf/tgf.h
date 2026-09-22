// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TGF_H__
#define __IDNI__PARSER__TGF_H__
#include <fstream>
#include <streambuf>
#include <optional>

#include "tgf_parser.generated.h"
#include "parser_strings.h"
#include "utility/devhelpers.h"
#include "utility/diagnostics.h"
#include "utility/escapes.h"

namespace idni {

template <typename C = char, typename T = C>
struct tgf {
	using tree     = tgf_parser::tree;
	using trv      = tree::traverser;
	using lit_t    = lit<C, T>;
	using prods_t  = prods<C, T>;
	using code     = idni::diagnostics::code;
	using result   = idni::diagnostics::result<grammar<C, T>>;
	using messages = idni::parser_strings::messages;
	using label    = idni::parser_strings::label;

	/// Parse TGF from string
	static result from_string(nonterminals<C, T>& nts_,
				  const std::basic_string<C>& s,
				  bool measure = false)
	{
		auto& p = tgf_parser::instance();
		tgf_parser::parse_options po{
			.start = tgf_parser::start,
			.measure_scopes = measure,
			.measure_counters = measure
		};

		result R;
		{
			auto _ = R.open_if(measure, label::grammar_load);
			std::optional<typename tgf_parser::result> pr;
			tref n;
			{
				auto _p = R.open_if(measure, label::tgf_parse);
				pr.emplace(p.parse(s.c_str(), s.size(), po));
				if (!pr->found) {
					if (!pr->report().nodes().empty())
						R.append(std::move(pr->report()));
					return R;
				}
				n = pr->get_shaped_tree2();
				if (!pr->report().nodes().empty())
					R.append(std::move(pr->report()));
			}
			{
				auto _b = R.open_if(measure, label::tgf_build);
				grammar_builder b(nts_);
				// build() / predefined_char_classes() take a raw
				// report*, so errors written there do NOT trigger
				// the result<> value-drop invariant. Gate emplace
				// on has_error() — this is the canonical pattern;
				// see the comment on predefined_char_classes in
				// src/grammar.tmpl.h.
				b.build(trv(n), &R.report(), s.c_str());
				if (!R.report().has_error()) R.emplace(b.g());
			}
		}
		return R;
	}
	/// Parse TGF from a file
	static result from_file(nonterminals<C, T>& nts_,
				const std::string& filename,
				bool measure = false)
	{
		std::ifstream ifs(filename);
		if (!ifs) {
			result R;
			R.report().reset(label::grammar_load);
			R.error(code::io_error, messages::cannot_open_file,
				{{label::path, filename}});
			return R;
		}
		return from_string(nts_,
			std::string(std::istreambuf_iterator<C>(ifs),
				std::istreambuf_iterator<C>()),
			measure);
	}

	/// Parse TGF from a string but split it to statements first
	static result from_string_presplit(
		nonterminals<C, T>& nts_,
		const std::basic_string<C>& s,
		tgf_parser::parse_options po = {})
	{
		result R;
		R.report().reset(label::grammar_load);
		grammar_builder b(nts_);
		auto c = s.c_str();
		size_t line = 0;
		size_t last = 0;
		auto l = s.size();
		bool in_string = false;
		bool in_comment = false;
		bool in_char = false;
		bool in_escape = false;
		bool had_parse_error = false;

		auto escape = [&in_escape](char c) {
			bool ret = in_escape;
			if (in_escape) in_escape = false;
			if (ret) return ret;
			return in_escape = (c == '\\');
		};
		for (size_t i = 0; i != l; ++i) {
			if (in_comment) {
				if (c[i] == '\n') in_comment = false;
			}
			else if (in_string) {
				if (escape(c[i])) continue;
				if (c[i] == '"') in_string = false;
			}
			else if (in_char) {
				if (escape(c[i])) continue;
				if (c[i] == '\'') in_char = false;
			}
			else if (c[i] == '\'') in_char = true;
			else if (c[i] == '"') in_string = true;
			else if (c[i] == '#') in_comment = true;
			else if (c[i] == '.') {
				const char* nc = c + last;
				size_t nl = i - last + 1;
				last = i + 1;
				size_t line_start = line;
				for (size_t j = 0; j < nl; ++j)
					if (nc[j] == '\n') --line_start;
				int ret = b.parse(nc, nl, line_start, R, po);
				if (ret == 1) { had_parse_error = true; break; }
			}
			if (c[i] == '\n') ++line;
		}
		if (!had_parse_error) R.emplace(b.g());
		return R;
	}
	/// Parse TGF from a file but split it to statements first
	static result from_file_presplit(
		nonterminals<C, T>& nts_,
		const std::string& filename,
		tgf_parser::parse_options po = {})
	{
		std::ifstream ifs(filename);
		if (!ifs) {
			result R;
			R.report().reset(label::grammar_load);
			R.error(code::io_error, messages::cannot_open_file,
				{{label::path, filename}});
			return R;
		}
		return from_string_presplit(nts_, std::string(
				std::istreambuf_iterator<C>(ifs),
				std::istreambuf_iterator<C>()), po);
	}

private:
	struct grammar_builder {
		using label    = idni::parser_strings::label;
		using messages = idni::parser_strings::messages;
		prods_t ps, nul{ lit_t{} };
		nonterminals<C, T>& nts;
		/// set by @start ...
		prods_t start = nul;
		/// char class names coming from @use char class ...
		std::vector<std::string> cc_names{};
		/// grammar options
		grammar<C, T>::options opt{};
		char_class_fns<T> cc;
		idni::diagnostics::report* diag = nullptr;
		const char* source_ = nullptr;
		grammar_builder(nonterminals<C, T>& nts) : nts(nts) {}
		grammar_builder(nonterminals<C, T>& nts, const trv& t,
			idni::diagnostics::report* diag = nullptr)
			: nts(nts) { build(t, diag); }
		void build(const trv& t,
			idni::diagnostics::report* diag = nullptr,
			const char* source = nullptr) {
			this->diag = diag;
			source_ = source;
			auto statements  = t || tgf_parser::statement;
			auto directives  = statements || tgf_parser::directive;
			for (const auto& d : directives()) collect_cc_names(d);
			cc = predefined_char_classes<C, T>(cc_names, nts, diag);
			for (const auto& d : directives()) directive(d);
			auto productions = statements || tgf_parser::production;
			for (const auto& pr : productions()) production(pr);
			if (!diag) return;
			std::set<size_t> defined, referenced;
			for (const auto& p : ps) {
				if (p.first.nt()) defined.insert(p.first.n());
				for (const auto& cj : p.second)
				for (const auto& lt : cj)
				for (const auto& l : lt)
					if (l.nt()) referenced.insert(l.n());
			}
			for (auto nt : referenced) {
				if (defined.count(nt)) continue;
				if (cc.is_fn(nt)) continue;
				auto name = nts.get(nt);
				if (name.size() >= 2 && name[0] == '_'
					&& name[1] == '_') continue;
				// a @dynamic nonterminal has no alternatives yet
				if (opt.dynamic.count(name)) continue;
				diag->warning(
					messages::unproductive_nonterminal,
					{{ label::name, name }});
			}
		}
		int parse(const char* s, size_t l, size_t line,
			idni::diagnostics::result<grammar<C, T>>& res,
			tgf_parser::parse_options po = {})
		{
			po.start = tgf_parser::start_statement;
			auto& p = tgf_parser::instance();
			auto r = p.parse(s, l, po);
			if (!r.found) {
				auto verbosity = po.error_verbosity;
				auto msg = r.parse_error.to_str(verbosity, line);
				res.error(code::parse_error, msg,
					{{ label::loc,  r.parse_error.loc },
					 { label::line, line + r.parse_error.line },
					 { label::col,  r.parse_error.col }});
				return 1;
			}
			build(trv(r.get_shaped_tree2()), &res.report(), s);
			return 0;
		}
		grammar<C, T> g() {
			//std::cout << "opt.trim_terminals: " << opt.trim_terminals << "\n";
			//std::cout << "opt.inline_char_classes: " << opt.inline_char_classes << "\n";
			//std::cout << "opt.to_trim: ";
			//for (auto& n : opt.to_trim) std::cout << n << " ";
			//std::cout << "\n";
			//std::cout << "opt.to_trim_children: ";
			//for (auto& n : opt.to_trim_children) std::cout << n << " ";
			//std::cout << "\n";
			//std::cout << "opt.to_inline: ";
			//for (auto& n : opt.to_inline) std::cout << n << " ";
			//std::cout << "\n";
			return grammar<C, T>(nts, ps, start == nul
				? prods_t(nts("start")) : start, cc, opt);
		}
	private:
		size_t id = 0;
		size_t node2nt(const trv& t) {
			return nts.get(dir_arg_text(t));
		}
	// ---- directive helpers (relaxed grammar) ----

	// Get the directive name from a directive node.
	std::string dir_name(const trv& t) {
		auto tok = t | tgf_parser::directive_token;
		auto nm = tok | tgf_parser::directive_name;
		if (nm.has_value())
			return dir_arg_text(nm | trv::only_child);
		for (auto& d :
			(tok || tgf_parser::directive_name)())
			return dir_arg_text(d | trv::only_child);
		return {};
	}

	// Extract terminal text from a traverser. Falls back to
	// span-based extraction for leaf nodes (dir_sym, escape_char,
	// etc.) that have no terminal children.
	std::string dir_arg_text(const trv& t) {
		if (!t.has_value()) return {};
		auto txt = t | trv::terminals;
		if (!txt.empty()) return txt;
		if (source_) {
			auto& sp = t.value_tree().value.second;
			if (sp[0] < sp[1])
				return std::string(source_ + sp[0],
					sp[1] - sp[0]);
		}
		return {};
	}

	// Collect all sym args from cmd blocks and dir_pairs.
	std::vector<std::string> dir_sym_args(const trv& t) {
		std::vector<std::string> v;
		// from directive_cmd blocks
		for (auto& cmd : (t || tgf_parser::directive_cmd)())
		for (auto& s : (cmd || tgf_parser::sym)())
			v.push_back(dir_arg_text(s));
		// from dir_pair blocks
		for (auto& p : (t || tgf_parser::dir_pair)())
		for (auto& lst : (p || tgf_parser::dir_list)())
		for (auto& c : (lst | trv::children)())
		{
			auto txt = c | trv::terminals;
			if (txt == "," || txt == ";") continue;
			if ((c | trv::nonterminal)
				!= tgf_parser::dir_arg) continue;
			txt = dir_arg_text(c | trv::only_child);
			if (!txt.empty()) v.push_back(txt);
		}
		return v;
	}


	// ---- directive processing ----

	void collect_cc_names(const trv& t) {
		std::string name = dir_name(t);
		if (name.empty() || name != "use") return;
		auto args = dir_sym_args(t);
		// find "char class" or "char classes" and collect everything after
		for (size_t i = 0; i + 1 < args.size(); ++i)
			if (args[i] == "char"
				&& (args[i+1] == "class"
					|| args[i+1] == "classes"))
			{
				for (size_t j = i + 2;
					j < args.size(); ++j)
					if (!args[j].empty())
					    cc_names.push_back(args[j]);
				return;
			}
	}

	void directive(const trv& t) {
		std::string name = dir_name(t);
		if (name.empty() || name.size() <= 1) return;

		if      (name == "use")        use_dir(t);
		else if (name == "start")      start_dir(t);
		else if (name == "trim")       trim_dir(t);
		else if (name == "inline")     inline_dir(t);
		else if (name == "disable")    disable_dir(t);
		else if (name == "ambiguous")  ambiguous_dir(t);
		else if (name == "enable")     enable_dir(t);
		else if (name == "highlight")  highlight_dir(t);
		else if (name == "dynamic")    dynamic_dir(t);
		else if (diag) diag->warning(messages::unknown_directive,
			{{ label::name, name }});
	}

	// ---- per-directive handlers ----

	void use_dir(const trv& /*t*/) {
		// cc_names already collected by collect_cc_names pre-pass
	}

	void start_dir(const trv& t) {
		auto a = t | tgf_parser::dir_pair | tgf_parser::dir_list
			   | tgf_parser::dir_arg;
		if (!a.has_value()) return;
		auto s = dir_arg_text(a | trv::only_child);
		if (s.empty()) return;
		start = prods_t(nts(s));
	}

	void trim_dir(const trv& t) {
		auto args = dir_sym_args(t);
		if (args.size() >= 2 && args[0] == "all"
			&& args[1] == "terminals")
			trim_all_terminals(t);
		else if (args.size() >= 1 && args[0] == "children")
			trim_children(t);
		else
			trim_simple(t);
	}

	void trim_simple(const trv& t) {
		auto args = dir_sym_args(t);
		for (auto& a : args)
			opt.shaping.to_trim.insert(nts.get(a));
	}

	void trim_all_terminals(const trv& t) {
		opt.shaping.trim_terminals = true;
		auto args = dir_sym_args(t);
		// check for optional "except children of" keywords
		bool has_except = false;
		for (size_t i = 0; i + 2 < args.size(); ++i)
			if (args[i] == "except"
				&& args[i+1] == "children"
				&& args[i+2] == "of")
				has_except = true;
		if (has_except)
			for (auto& p :
				(t || tgf_parser::dir_pair)())
			for (auto& lst :
				(p || tgf_parser::dir_list)())
			for (auto& a :
				(lst || tgf_parser::dir_arg)())
			opt.shaping
			 .dont_trim_terminals_of
			 .insert(nts.get(
				dir_arg_text(a | trv::only_child)));
	}

	void trim_children(const trv& t) {
		auto args = dir_sym_args(t);
		bool terminals = (args.size() >= 2
			&& args[1] == "terminals");
		auto& target = terminals
			? opt.shaping.to_trim_children_terminals
			: opt.shaping.to_trim_children;
		for (auto& p :
			(t || tgf_parser::dir_pair)())
		for (auto& lst :
			(p || tgf_parser::dir_list)())
		for (auto& a :
			(lst || tgf_parser::dir_arg)())
			target.insert(nts.get(
				dir_arg_text(a | trv::only_child)));
	}

	void inline_dir(const trv& t) {
		auto args = dir_sym_args(t);
		for (size_t i = 0; i + 1 < args.size(); ++i)
			if (args[i] == "char"
				&& (args[i+1] == "class"
					|| args[i+1] == "classes"))
			{
				opt.shaping.inline_char_classes
					= true;
			}
		for (auto& p :
			(t || tgf_parser::dir_pair)())
		for (auto& lst :
			(p || tgf_parser::dir_list)())
		for (auto& a :
			(lst || tgf_parser::dir_arg)())
		{
			auto child =
				a | trv::only_child;
			if ((child
				| trv::nonterminal)
				== tgf_parser::tree_path)
			{
				std::vector<size_t> path;
				for (auto& s :
				 (child
				  || tgf_parser::dir_sym)())
				path.push_back(
				  nts.get(dir_arg_text(s)));
				if (!path.empty())
					opt.shaping.to_inline
					   .insert(
					    std::move(path));
			}
			else {
				auto txt = dir_arg_text(child);
				if (txt != "char" && txt != "class"
					&& txt != "classes")
				opt.shaping.to_inline
					.insert({ nts.get(txt) });
			}
		}
	}

	void disable_dir(const trv& t) {
		auto args = dir_sym_args(t);
		for (auto& a : args) {
			if (a == "disambiguation") {
				opt.auto_disambiguate = false;
			} else if (diag) diag->warning(
				messages::unknown_directive_argument,
				{{ label::name, a }});
		}
	}

	void ambiguous_dir(const trv& t) {
		auto args = dir_sym_args(t);
		for (auto& a : args) {
			auto id = nts.get(a);
			opt.nodisambig_list.insert(id);
		}
	}

	void enable_dir(const trv& t) {
		auto args = dir_sym_args(t);
		for (auto& a : args) {
			if (a == "disambiguation") {
				opt.auto_disambiguate = true;
			} else opt.enabled_guards.insert(a);
		}
	}

	void highlight_dir(const trv& t) {
		for (auto& p : (t || tgf_parser::dir_pair)()) {
			std::vector<trv> lists;
			for (auto& lst :
				(p || tgf_parser::dir_list)())
				lists.push_back(lst);
			if (lists.size() < 2) continue;
			std::string type_name;
			for (auto& a :
				(lists[0] || tgf_parser::dir_arg)())
				type_name = dir_arg_text(
					a | trv::only_child);
			if (type_name.empty()) continue;
			std::vector<std::string> nt_names;
			for (auto& a :
				(lists[1] || tgf_parser::dir_arg)()) {
				auto n = dir_arg_text(
					a | trv::only_child);
				if (!n.empty()) {
					nt_names.push_back(n);
				}
			}
			if (!nt_names.empty())
				opt.highlights.emplace_back(
					std::move(type_name),
					std::move(nt_names));
		}
	}

	// value of one dir_arg: a plain sym, or a (possibly escaped) string
	std::basic_string<C> dynamic_value(const trv& t) {
		auto c = t | trv::only_child;
		auto nt = c | trv::nonterminal;
		if (nt == tgf_parser::dir_sym)
			return from_str<C>(dir_arg_text(c));
		if (nt != tgf_parser::terminal_string) return {};
		std::basic_string<C> r{};
		for (auto& ch : (c | trv::children)()) {
			if ((ch | trv::nonterminal) == tgf_parser::unescaped_s)
				r += from_str<C>(dir_arg_text(ch));
			else {
				auto txt = (ch | tgf_parser::escaped_s)
					| trv::terminals;
				// escaped_s children may be span-only
				if ((txt.empty() || txt.size() == 1)
					&& source_)
				{
					auto& sp = ch.value_tree()
						.value.second;
					if (sp[0] < sp[1])
						txt.assign(
							source_ + sp[0],
							sp[1] - sp[0]);
				}
				r += unescape(txt, idni::escapes::tgf_string);
			}
		}
		return r;
	}

	// rejoin command syms the sep/'_' ambiguity split apart.
	std::vector<std::string> dynamic_cmd_words(const trv& t) {
		std::vector<std::string> words;
		size_t prev_end = 0;
		bool have_prev = false;
		for (auto& cmd : (t || tgf_parser::directive_cmd)())
		for (auto& s : (cmd || tgf_parser::sym)()) {
			auto txt = dir_arg_text(s);
			auto& sp = s.value_tree().value.second;
			if (have_prev && source_ && sp[0] == prev_end + 1
				&& (source_[prev_end] == '_'
					|| source_[prev_end] == '-'))
			{
				words.back() += source_[prev_end];
				words.back() += txt;
			} else words.push_back(txt);
			prev_end = sp[1];
			have_prev = true;
		}
		return words;
	}

	void dynamic_dir(const trv& t) {
		std::vector<std::string> words = dynamic_cmd_words(t);

		std::vector<std::vector<trv>> pairs;
		for (auto& p : (t || tgf_parser::dir_pair)()) {
			std::vector<trv> args;
			for (auto& lst : (p || tgf_parser::dir_list)())
			for (auto& c : (lst | trv::children)()) {
				if ((c | trv::nonterminal)
					!= tgf_parser::dir_arg) continue;
				args.push_back(c);
			}
			pairs.push_back(std::move(args));
		}

		auto declare = [&](const std::basic_string<C>& name) {
			if (!name.empty()) opt.dynamic[name];
		};
		auto add_values = [&](const std::basic_string<C>& name,
			const std::vector<trv>& args)
		{
			if (name.empty()) return;
			auto& kept = opt.dynamic[name];
			std::set<std::basic_string<C>> seen(
				kept.begin(), kept.end());
			for (auto& a : args)
				if (auto v = dynamic_value(a);
					seen.insert(v).second)
						kept.push_back(v);
		};
		auto pair_name = [&](const std::vector<trv>& args) {
			if (args.empty()) return std::basic_string<C>{};
			return from_str<C>(
				dir_arg_text(args.front() | trv::only_child));
		};
		// a bare declaration pair names exactly one nonterminal;
		// use ';' to separate names, not ','
		auto declare_pair = [&](const std::vector<trv>& args) {
			if (args.size() > 1) {
				if (diag) diag->error(code::invalid_argument,
					messages::dynamic_names_need_semicolon);
				return;
			}
			declare(pair_name(args));
		};

		size_t pi = 0;
		if (!words.empty() && words.back() == "defaults"
			&& words.size() >= 2)
		{
			auto name = from_str<C>(words[words.size() - 2]);
			if (pi < pairs.size()) add_values(name, pairs[pi++]);
			else declare(name);
			for (; pi < pairs.size(); ++pi)
				declare_pair(pairs[pi]);
		} else {
			for (auto& w : words) declare(from_str<C>(w));
			for (auto& args : pairs) declare_pair(args);
		}
	}
		void production(const trv& t) {
			//print_node(std::cout, t.value()) << "\n";
			prods_t sym(nts(dir_arg_text(t | tgf_parser::sym)));
			std::string guard(dir_arg_text(
				t | tgf_parser::production_guard
					| tgf_parser::sym));
			if (guard.size()) {
				sym.back().guard = guard;
				// DBG(std::cout << "sym: (" << sym << ") guard: " << guard << "\n";)
			}
			alternation(sym, t | tgf_parser::alternation);
		}
		void alternation(const prods_t& sym, const trv& t) {
			//print_node(std::cout << "alternation: ", t.value()) << "\n";
			for (auto& c : (t || tgf_parser::conjunction)())
				ps(sym, conjunction(sym, c));
		}
		prods_t optional(const prods_t& sym, const prods_t& t)
		{
			auto nn = get_new_name(sym);
			return ps(nn, t), ps(nn, nul), nn;
		}
		// Repetitions expand left-recursively: the Earley parser
		// completes a left-recursive list in linear time, a
		// right-recursive one in about n^3 (a 1 200-character run of
		// single-character items took ~30 s). Same language; the
		// helper is inlined, so the parsed tree is the same either way.
		prods_t repeat(const prods_t& sym, const prods_t& t) {
			auto nn = get_new_name(sym);
			return ps(nn, t | (nn + t)), nn;
		}
		prods_t none_or_repeat(const prods_t& sym, const prods_t& t) {
			auto nn = get_new_name(sym);
			return ps(nn, (nn + t) | nul), nn;
		}
		prods_t group(const prods_t& sym, const trv& t) {
			auto nn = get_new_name(sym);
			alternation(nn.to_lit(), t | tgf_parser::alternation);
			return nn;
		}
		prods_t optional_group(const prods_t& sym, const trv& t)
		{
			auto nn = get_new_name(sym);
			alternation(nn.to_lit(), t | tgf_parser::alternation);
			return ps(nn, nul), nn;
		}
		prods_t repeat_group(const prods_t& sym, const trv& t) {
			auto nn = get_new_name(sym);
			auto nr = get_new_name(sym);
			alternation(nr.to_lit(), t | tgf_parser::alternation);
			return ps(nn, nr | (nn + nr)), nn;
		}
		std::basic_string<C> unescape(
			const std::string& s,
			const idni::escapes::profile& p
				= idni::escapes::tgf_string)
		{
			auto dec = idni::escapes::decode(s, p);
			if (!dec.has_value()) {
				if (diag) diag->append(std::move(dec.report()));
				return {};
			}
			return from_str<C>(std::move(dec).value());
		}
		prods_t terminal_char(const trv& t) {
			auto c = t | tgf_parser::unescaped_c;
			if (c.has_value()) {
				auto txt = c | trv::terminals;
				// escape_char/unescaped_c may be span-only
				// leaves; extract from source position
				if (txt.empty() && source_) {
					auto& sp = c.value_tree().value.second;
					if (sp[0] < sp[1])
						txt.assign(source_ + sp[0],
							sp[1] - sp[0]);
				}
				return prods_t(txt);
			}
			c = t | tgf_parser::escaped_c;
			auto txt = c | trv::terminals;
			// escaped_c children (escape_char etc.) may be
			// span-only; extract full escape sequence from
			// the escaped_c span
			if ((txt.empty() || txt.size() == 1) && source_) {
				auto& sp = c.value_tree().value.second;
				if (sp[0] < sp[1])
					txt.assign(source_ + sp[0],
						sp[1] - sp[0]);
			}
			return prods_t(unescape(txt,
				idni::escapes::tgf_char));
		}
		prods_t terminal_string(const trv& t) {
			prods_t r{};
			for (auto& ch : (t | trv::children)()) {
				if ((ch | trv::nonterminal) == tgf_parser::unescaped_s) {
					auto txt = ch | trv::terminals;
					// unescaped_s may be a span-only leaf
					if (txt.empty() && source_) {
						auto& sp = ch.value_tree()
							.value.second;
						if (sp[0] < sp[1])
							txt.assign(
								source_ + sp[0],
								sp[1] - sp[0]);
					}
					r = r + prods_t(txt);
				}
				else {
					auto txt = (ch | tgf_parser::escaped_s)
						| trv::terminals;
					// escaped_s children may be span-only;
					// extract from escaped_s span
					if ((txt.empty() || txt.size() == 1)
						&& source_)
					{
						auto& sp = ch.value_tree()
							.value.second;
						if (sp[0] < sp[1])
							txt.assign(
								source_ + sp[0],
								sp[1] - sp[0]);
					}
					r = r + prods_t(unescape(txt,
						idni::escapes::tgf_string));
				}
			}
			return r;
		}
		prods_t terminal_hex(const trv& t) {
			auto digits = dir_arg_text(t | tgf_parser::hex_bytes);
			std::string bytes;
			if (digits.size() == 1) {
				bytes.push_back(
					(char)idni::escapes::hex_val(digits[0]));
			} else {
				for (size_t i = 0; i + 1 < digits.size(); i += 2) {
					int hi = idni::escapes::hex_val(digits[i]);
					int lo = idni::escapes::hex_val(digits[i+1]);
					bytes.push_back(
						(char)((hi << 4) | lo));
				}
			}
			return prods_t(from_str<C>(bytes));
		}
		prods_t terminal(const trv& t) {
			//print_node(std::cout << "terminal: ", t.value()) << "\n";
			auto c = t | trv::only_child;
			auto nt = c | trv::nonterminal;
			if (nt == tgf_parser::terminal_char)
				return terminal_char(c);
			if (nt == tgf_parser::terminal_hex)
				return terminal_hex(c);
			return terminal_string(c);
		}
		prods_t term(const prods_t& sym, const trv& t) {
			//print_node(std::cout << "term: ", t.value()) << "\n";
			auto x = t | trv::only_child;
			//std::cout << "term: " << (x | trv::nonterminal) << std::endl;
			switch (x | trv::nonterminal) {
			case tgf_parser::sym: {
				auto s = dir_arg_text(x);
				if (s == "null") return nul;
				return prods_t(nts(s));
			}
			case tgf_parser::terminal:       return terminal(x);
			case tgf_parser::group:          return group(sym, x);
			case tgf_parser::optional_group: return optional_group(sym, x);
			case tgf_parser::repeat_group:   return repeat_group(sym, x);
			default: assert(false);
			}
			return {};
		}
		prods_t shorthand_rule(const prods_t& sym, const trv& t) {
			//print_node(std::cout << "shorthand_rule: ", t.value()) << "\n";
			auto f = t | tgf_parser::factor;
			auto s = dir_arg_text(t | tgf_parser::sym);
			auto nt = nts(s);
			ps(nt, factor(sym, f));
			return nt;
		}
		prods_t factor(const prods_t& sym, const trv& t) {
			//print_node(std::cout << "factor: ", t.value()) << "\n";
			auto f = t | trv::only_child;
			auto nt = f | trv::nonterminal;
			if (nt == tgf_parser::shorthand_rule)
				return shorthand_rule(sym, f);
			if (nt == tgf_parser::term) return term(sym, f);
			auto trm = term(sym, f | tgf_parser::term);
			switch (nt) {
			case tgf_parser::optional:       return optional(sym, trm);
			case tgf_parser::repeat:         return repeat(sym, trm);
			case tgf_parser::none_or_repeat: return none_or_repeat(sym, trm);
			case tgf_parser::neg:            return ~trm;
			default: assert(false);
			}
			return {};
		}
		prods_t concatenation(const prods_t& sym, const trv& t){
			//print_node(std::cout << "concatenation: ", t.value()) << "\n";
			prods_t r{};
			for (auto& f : (t || tgf_parser::factor)())
				r = r + factor(sym, f);
			return r;
		}
		prods_t conjunction(const prods_t& sym,	const trv& t) {
			//print_node(std::cout << "conjunction: ", t.value()) << "\n";
			prods_t r{};
			for (auto& c : (t || tgf_parser::concatenation)())
				r = r & concatenation(sym, c);
			return r;
		}
		prods_t get_new_name(const prods_t& sym) {
			std::stringstream ss;
			ss << "__E_" << sym.to_lit().to_std_string()
								<< "_" << id++;
			//std::cout << "new name: " << id << " " << to_std_string(ss.str()) << "\n";
			return prods_t(nts(from_str<C>(ss.str())));
		}
	};
};

} // idni namespace
#endif // __IDNI__PARSER__TGF_H__
