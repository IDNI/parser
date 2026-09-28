// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__GRAMMAR_INSPECTOR_H__
#define __IDNI__PARSER__GRAMMAR_INSPECTOR_H__
#include "parser.h"

namespace idni {

/// Grammar inspector - friend access to grammar's private members.
/// Used by parser_gen
template <typename C, typename T>
struct grammar_inspector {
	grammar_inspector(const grammar<C, T>& g) : g(g) {}
	const grammar<C, T>& g;
	lit<C, T> start() const { return g.start; }
	nonterminals<C, T>& nts() const { return g.nts; }
	const std::vector<std::pair<lit<C, T>,
		std::vector<lits<C, T>>>>& G() const { return g.G; }
	const char_class_fns<T>& cc_fns() const { return g.cc_fns; }
	const std::map<size_t, size_t>& grdm() const { return g.grdm; }
	const std::vector<std::string>& guards() const { return g.guards; }
	/// Production indices of the derived class `A => ch` productions.
	const std::set<size_t>& cc_char_prod_ids() const
		{ return g.cc_char_prod_ids_; }
};

} // namespace idni

#endif // __IDNI__PARSER__GRAMMAR_INSPECTOR_H__
