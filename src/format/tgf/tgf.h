// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__TGF_H__
#define __IDNI__PARSER__TGF_H__
#include <algorithm>
#include <fstream>
#include <memory>
#include <streambuf>
#include <optional>
#include <set>
#include <string_view>
#include <vector>

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

	/// Parse TGF from a string, creating and owning the symbol table.
	static result from_string(const std::basic_string<C>& s,
				  bool measure = false)
	{
		return from_string(std::shared_ptr<nonterminals<C, T>>{},
			s, measure);
	}
	/// Parse TGF from a string with a caller-provided symbol table.
	static result from_string(nonterminals<C, T>& nts_,
				  const std::basic_string<C>& s,
				  bool measure = false)
	{
		return from_string(
			std::shared_ptr<nonterminals<C, T>>(
				std::shared_ptr<void>(), &nts_), s, measure);
	}
	/// Parse TGF from a string with a shared symbol table. A null @p nts_
	/// makes the grammar create and own one.
	static result from_string(std::shared_ptr<nonterminals<C, T>> nts_,
				  const std::basic_string<C>& s,
				  bool measure = false)
	{
		if (!nts_) nts_ = std::make_shared<nonterminals<C, T>>();
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
				grammar_builder b(std::move(nts_));
				// build() / predefined_char_classes() take a raw
				// report*, so errors written there do NOT trigger
				// the result<> value-drop invariant. Gate emplace
				// on has_error() — this is the canonical pattern;
				// see the comment on predefined_char_classes in
				// src/grammar.tmpl.h.
				b.build(trv(n), &R.report(), s.c_str());
				if (!R.report().has_error()) {
					// a directive name the grammar lacks fails the load
					auto gr = b.g();
					if (!R.report().has_error())
						R.emplace(std::move(gr));
				}
			}
		}
		return R;
	}
	/// Parse TGF from a file, creating and owning the symbol table.
	static result from_file(const std::string& filename,
				bool measure = false)
	{
		return from_file(std::shared_ptr<nonterminals<C, T>>{},
			filename, measure);
	}
	/// Parse TGF from a file with a caller-provided symbol table.
	static result from_file(nonterminals<C, T>& nts_,
				const std::string& filename,
				bool measure = false)
	{
		return from_file(
			std::shared_ptr<nonterminals<C, T>>(
				std::shared_ptr<void>(), &nts_), filename, measure);
	}
	/// Parse TGF from a file with a shared symbol table. A null @p nts_
	/// makes the grammar create and own one.
	static result from_file(std::shared_ptr<nonterminals<C, T>> nts_,
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
		return from_string(std::move(nts_),
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
		std::shared_ptr<nonterminals<C, T>> nts_handle;
		/// char class names coming from @use char class ..., in the order
		/// of the file, so that the class nonterminals keep their ids
		std::vector<std::string> cc_names{};
		/// @use char class <class> as <name>, by the class
		std::map<std::string, std::string> cc_renames{};
		/// every <class> as <name> of @use, in the order of the file
		std::vector<std::pair<std::string, std::string>> cc_aliases{};
		/// a renamed class listed again, which has its load error
		std::set<std::string> cc_again{};
		/// plain (non-glob) @highlight pattern names seen so far.
		std::set<std::string> highlight_plain_names{};
		/// @dynamic names seen so far, which have no production yet
		std::set<std::string> dynamic_names{};
		/// The name of the first @variants profile of the file.
		std::string first_variant{};
		/// The directives of this load, so a load never writes into the
		/// repository of the thread.
		idni::options_repository own_;
		char_class_fns<T> cc;
		idni::diagnostics::report* diag = nullptr;
		const char* source_ = nullptr;
		grammar_builder(nonterminals<C, T>& nts_)
			: nts(nts_),
			  nts_handle(std::shared_ptr<nonterminals<C, T>>(
				std::shared_ptr<void>(), &nts_)) {}
		grammar_builder(std::shared_ptr<nonterminals<C, T>> nts_)
			: nts(*nts_), nts_handle(std::move(nts_)) {}
		grammar_builder(nonterminals<C, T>& nts_, const trv& t,
			idni::diagnostics::report* diag = nullptr)
			: nts(nts_),
			  nts_handle(std::shared_ptr<nonterminals<C, T>>(
				std::shared_ptr<void>(), &nts_)) { build(t, diag); }
		void build(const trv& t,
			idni::diagnostics::report* diag = nullptr,
			const char* source = nullptr) {
			this->diag = diag;
			source_ = source;
			// each directive writes its option into own_ at once, and the
			// bind in g() pushes the values into the grammar
			keep(own_.declare(grammar_option_set));
			auto statements  = t || tgf_parser::statement;
			auto directive_nodes = statements || tgf_parser::directive;
			for (const auto& d : directive_nodes()) collect_cc_names(d);
			collect_cc_renames();
			cc = predefined_char_classes<C, T>(cc_names, cc_renames,
				nts, diag);
			std::vector<std::string> cc_entries;
			for (const auto& cc_name : cc_names)
				if (auto it = cc_renames.find(cc_name);
					it != cc_renames.end())
						cc_entries.push_back(cc_name + std::string(
							detail::char_class_as) + it->second);
				else cc_entries.push_back(cc_name);
			if (!cc_entries.empty())
				edit_list("char-classes", cc_entries);
			for (const auto& d : directive_nodes()) directive(d);
			// the first profile is the default when no @variant selects one
			if (!first_variant.empty()
				&& own_.get<std::string>("variant").empty())
					set_option("variant", first_variant);
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
			// a renamed class has only its new name in this grammar
			std::set<std::string> old_names, cc_locals(cc_again);
			for (const auto& cc_name : cc_names)
				if (!cc_renames.count(cc_name)) cc_locals.insert(cc_name);
				else if (!cc_again.count(cc_name))
					old_names.insert(cc_name);
			for (const auto& [cc_name, local] : cc_aliases)
				cc_locals.insert(local);
			for (auto nt : referenced) {
				if (defined.count(nt)) continue;
				if (cc.is_fn(nt)) continue;
				auto name = nts.get(nt);
				if (old_names.count(name)) {
					diag->error(
						idni::diagnostics::code::invalid_argument,
						messages::char_class_old_name,
						{{ label::name, name }});
					continue;
				}
				// a @use name that is no class got its load error
				if (cc_locals.count(name)) continue;
				if (name.size() >= 2 && name[0] == '_'
					&& name[1] == '_') continue;
				// a @dynamic nonterminal has no alternatives yet
				if (dynamic_names.count(name)) continue;
				diag->warning(
					messages::unproductive_nonterminal,
					{{ label::name, name }});
			}
			std::set<std::string> defined_names;
			for (auto nt : defined) defined_names.insert(nts.get(nt));
			for (auto& name : highlight_plain_names) {
				if (defined_names.count(name)) continue;
				if (cc_locals.count(name)) continue;
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
			// a presplit load of no statement never runs build()
			keep(own_.declare(grammar_option_set));
			grammar<C, T> gr(nts_handle, ps,
				prods_t(nts(own_.get<std::string>("start"))), cc, {},
				diag);
			// the bind pushes the directives through the codec, which
			// reports a name the grammar does not have
			{
				idni::options_scope scope(own_);
				keep(gr.bind_options());
				gr.unbind_options();
			}
			return gr;
		}
		/// Merge @p r into diag. True when it has no error.
		bool keep(idni::diagnostics::result<void> r) {
			const bool ok = !r.has_error();
			if (diag) diag->append(std::move(r).report());
			return ok;
		}
		void set_option(std::string_view name, option_value v) {
			keep(own_.set(name, std::move(v)));
		}
		/// Add @p names to the list option @p name, or remove them.
		void edit_list(std::string_view name,
			const std::vector<std::string>& names, bool add = true)
		{
			if (!own_.find(name)) return;
			auto list = own_.get<option_list>(name);
			for (const auto& n : names) {
				std::erase(list, n);
				if (add) list.push_back(n);
			}
			set_option(name, std::move(list));
		}
	private:
		size_t id = 0;
		size_t node2nt(const trv& t) {
			return nts.get(dir_arg_text(t));
		}
		/// True when name is a token type of the @highlight legend.
		static bool is_highlight_token_type(const std::string& name) {
			for (auto& t : highlight_token_types)
				if (t == name) return true;
			return false;
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

	// Raw source text under a node's span for subtrees whose terminals are trimmed.
	std::string span_text(const trv& t) {
		if (!t.has_value() || !source_) return {};
		auto& sp = t.value_tree().value.second;
		return sp[0] < sp[1]
			? std::string(source_ + sp[0], sp[1] - sp[0])
			: std::string{};
	}

	// Collect all sym args from cmd blocks and dir_pairs.
	/// An alias of a dir_list gives its class and goes to \p aliases, or
	/// is skipped without it.
	std::vector<std::string> dir_sym_args(const trv& t,
		std::vector<std::pair<std::string, std::string>>* aliases
			= nullptr)
	{
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
			auto a = c | trv::only_child;
			if ((a | trv::nonterminal) == tgf_parser::dir_alias) {
				auto syms = (a || tgf_parser::dir_sym)();
				if (!aliases || syms.size() != 2) continue;
				aliases->emplace_back(dir_arg_text(syms[0]),
					dir_arg_text(syms[1]));
				v.push_back(aliases->back().first);
				continue;
			}
			txt = dir_arg_text(a);
			if (!txt.empty()) v.push_back(txt);
		}
		return v;
	}


	// ---- directive processing ----

	void collect_cc_names(const trv& t) {
		std::string name = dir_name(t);
		if (name.empty() || name != "use") return;
		std::vector<std::pair<std::string, std::string>> aliases;
		auto args = dir_sym_args(t, &aliases);
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
				cc_aliases.insert(cc_aliases.end(),
					aliases.begin(), aliases.end());
				return;
			}
	}

	// A class renamed two times, or listed beside its rename, is an error.
	// The class stays once in cc_names, at its first place.
	void collect_cc_renames() {
		for (const auto& [cc_name, local] : cc_aliases)
			if (!cc_renames.emplace(cc_name, local).second)
				cc_again.insert(cc_name);
		std::set<std::string> seen;
		std::vector<std::string> names;
		for (const auto& cc_name : cc_names) {
			if (!cc_renames.count(cc_name)) names.push_back(cc_name);
			else if (seen.insert(cc_name).second)
				names.push_back(cc_name);
			else cc_again.insert(cc_name);
		}
		cc_names = std::move(names);
		for (const auto& cc_name : cc_again)
			if (diag) diag->error(
				idni::diagnostics::code::invalid_argument,
				messages::char_class_listed_again,
				{{ label::name, cc_name }});
			else std::cerr << messages::char_class_listed_again
				<< ": " << cc_name << '\n';
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
		else if (name == "variants")   variants_dir(t);
		else if (name == "variant")    variant_dir(t);
		else if (diag) diag->warning(messages::unknown_directive,
			{{ label::name, name }});
	}

	// ---- per-directive handlers ----

	// @variants name: g1, g2; other: g3. defines one profile per pair
	void variants_dir(const trv& t) {
		auto bad_form = [this](const std::string& text) {
			if (diag) diag->error(code::invalid_argument,
				messages::invalid_variant_profile,
				{{ label::name, text }});
		};
		// a word before the first pair has no colon
		auto cmds = (t || tgf_parser::directive_cmd)();
		if (!cmds.empty()) return bad_form(span_text(cmds.front()));
		auto pairs = (t || tgf_parser::dir_pair)();
		if (pairs.empty()) return bad_form(span_text(t));
		for (auto& p : pairs) {
			auto lists = (p || tgf_parser::dir_list)();
			std::vector<std::string> names, guards;
			if (lists.size() == 2) {
				for (auto& a : (lists[0] || tgf_parser::dir_arg)())
					names.push_back(dir_arg_text(
						a | trv::only_child));
				for (auto& a : (lists[1] || tgf_parser::dir_arg)())
					guards.push_back(dir_arg_text(
						a | trv::only_child));
			}
			if (names.size() != 1 || guards.empty()) {
				bad_form(span_text(p));
				continue;
			}
			const std::string& name = names.front();
			const std::string head = name + "=";
			if (std::ranges::any_of(own_.get<option_list>("variants"),
				[&head](const std::string& e) {
					return e.starts_with(head); }))
			{
				if (diag) diag->error(code::invalid_argument,
					messages::variant_defined_twice,
					{{ label::name, name }});
				continue;
			}
			std::vector<std::string> entries;
			for (const auto& g : guards) entries.push_back(head + g);
			edit_list("variants", entries);
			if (first_variant.empty()) first_variant = name;
		}
	}

	// @variant name. selects a profile
	void variant_dir(const trv& t) {
		std::vector<std::string> args = dynamic_cmd_words(t);
		for (auto& p : (t || tgf_parser::dir_pair)())
		for (auto& lst : (p || tgf_parser::dir_list)())
		for (auto& a : (lst || tgf_parser::dir_arg)())
			args.push_back(dir_arg_text(a | trv::only_child));
		if (args.size() != 1) {
			if (diag) diag->error(code::invalid_argument,
				messages::invalid_variant_profile,
				{{ label::name, span_text(t) }});
			return;
		}
		set_option("variant", args.front());
	}

	void use_dir(const trv& /*t*/) {
		// the pre-pass collects the classes and writes char-classes
	}

	void start_dir(const trv& t) {
		auto a = t | tgf_parser::dir_pair | tgf_parser::dir_list
			   | tgf_parser::dir_arg;
		if (!a.has_value()) return;
		auto s = dir_arg_text(a | trv::only_child);
		if (s.empty()) return;
		set_option("start", s);
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
		edit_list("trim", args);
	}

	void trim_all_terminals(const trv& t) {
		set_option("trim-terminals", true);
		auto args = dir_sym_args(t);
		// check for optional "except children of" keywords
		bool has_except = false;
		for (size_t i = 0; i + 2 < args.size(); ++i)
			if (args[i] == "except"
				&& args[i+1] == "children"
				&& args[i+2] == "of")
				has_except = true;
		if (!has_except) return;
		std::vector<std::string> names;
		for (auto& p :
			(t || tgf_parser::dir_pair)())
		for (auto& lst :
			(p || tgf_parser::dir_list)())
		for (auto& a :
			(lst || tgf_parser::dir_arg)())
			names.push_back(dir_arg_text(a | trv::only_child));
		edit_list("dont-trim-terminals-of", names);
	}

	void trim_children(const trv& t) {
		auto args = dir_sym_args(t);
		bool terminals = (args.size() >= 2
			&& args[1] == "terminals");
		std::vector<std::string> names;
		for (auto& p :
			(t || tgf_parser::dir_pair)())
		for (auto& lst :
			(p || tgf_parser::dir_list)())
		for (auto& a :
			(lst || tgf_parser::dir_arg)())
			names.push_back(dir_arg_text(a | trv::only_child));
		edit_list(terminals ? "trim-children-terminals"
			: "trim-children", names);
	}

	void inline_dir(const trv& t) {
		auto args = dir_sym_args(t);
		for (size_t i = 0; i + 1 < args.size(); ++i)
			if (args[i] == "char"
				&& (args[i+1] == "class"
					|| args[i+1] == "classes"))
			{
				set_option("inline-char-classes", true);
			}
		std::vector<std::string> entries;
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
				std::string path;
				for (auto& s :
				 (child
				  || tgf_parser::dir_sym)())
				{
					if (!path.empty()) path += " > ";
					path += dir_arg_text(s);
				}
				if (!path.empty())
					entries.push_back(std::move(path));
			}
			else {
				auto txt = dir_arg_text(child);
				if (txt != "char" && txt != "class"
					&& txt != "classes")
					entries.push_back(txt);
			}
		}
		if (!entries.empty()) edit_list("inline", entries);
	}

	void disable_dir(const trv& t) {
		for (auto& a : dir_sym_args(t)) {
			// productions is a keyword, not a guard
			if (a == "productions") continue;
			if (a == "disambiguation") {
				set_option("auto-disambiguate", false);
			} else edit_list("enabled-productions", { a }, false);
		}
	}

	void ambiguous_dir(const trv& t) {
		auto args = dir_sym_args(t);
		edit_list("nodisambig-list", args);
	}

	void enable_dir(const trv& t) {
		for (auto& a : dir_sym_args(t)) {
			// productions is a keyword, not a guard
			if (a == "productions") continue;
			if (a == "disambiguation") {
				set_option("auto-disambiguate", true);
			} else edit_list("enabled-productions", { a });
		}
	}

	void highlight_dir(const trv& t) {
		for (auto& p : (t || tgf_parser::dir_pair)()) {
			std::vector<trv> lists;
			for (auto& lst :
				(p || tgf_parser::dir_list)())
				lists.push_back(lst);
			if (lists.size() == 1) {
				std::vector<std::string> args;
				for (auto& a :
					(lists[0] || tgf_parser::dir_arg)())
					args.push_back(dir_arg_text(
						a | trv::only_child));
				if (args.size() == 1 && args[0] == "auto") {
					set_option("highlight-auto", true);
					continue;
				}
				if (diag) for (auto& a : args)
					diag->warning(
						messages::unknown_directive_argument,
						{{ label::name, a }});
				continue;
			}
			std::string type_name;
			for (auto& a :
				(lists[0] || tgf_parser::dir_arg)())
				type_name = dir_arg_text(
					a | trv::only_child);
			if (type_name.empty()) continue;
			if (!is_highlight_token_type(type_name)) {
				if (diag) diag->warning(
					messages::unknown_highlight_type,
					{{ label::type_name, type_name }});
				continue;
			}
			std::vector<std::string> entries;
			for (auto& a :
				(lists[1] || tgf_parser::dir_arg)()) {
				auto child = a | trv::only_child;
				auto nt = child | trv::nonterminal;
				std::string n;
				if (nt == tgf_parser::tree_path
					|| nt == tgf_parser::treemr_pattern)
					n = span_text(child);
				else if (nt == tgf_parser::terminal_string)
					n = '"' + idni::to_std_string(
						dynamic_value(a)) + '"';
				else n = dir_arg_text(child);
				if (n.empty()) continue;
				entries.push_back(type_name + ": " + n);
				if (nt == tgf_parser::dir_sym
					&& n.find('*') == std::string::npos)
					highlight_plain_names.insert(n);
			}
			if (!entries.empty()) edit_list("highlight", entries);
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
			if (name.empty()) return;
			dynamic_names.insert(idni::to_std_string(name));
			edit_list("dynamic", { idni::to_std_string(name) + "=" });
		};
		auto add_values = [&](const std::basic_string<C>& name,
			const std::vector<trv>& args)
		{
			if (name.empty()) return;
			const std::string head = idni::to_std_string(name) + "=";
			dynamic_names.insert(idni::to_std_string(name));
			std::vector<std::string> entries;
			for (auto& a : args) {
				auto v = dynamic_value(a);
				// an empty value would make the nonterminal nullable
				if (v.empty()) {
					if (diag) diag->error(code::invalid_argument,
						messages::dynamic_empty_value,
						{{ label::name,
							idni::to_std_string(name) }});
					continue;
				}
				entries.push_back(head + idni::to_std_string(v));
			}
			edit_list("dynamic", entries);
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
					auto hi = idni::escapes::hex_val(digits[i]);
					auto lo = idni::escapes::hex_val(digits[i+1]);
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
