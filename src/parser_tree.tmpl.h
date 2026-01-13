// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#include "parser.h"
namespace idni {

//------------------------------------------------------------------------------

template <typename C, typename T>
tref parser<C, T>::tree::get() const { return base_t::get(); }

template <typename C, typename T>
const parser<C, T>::tree& parser<C, T>::tree::get(const tref id) {
	return static_cast<const tree&>(base_t::get(id));
}

template <typename C, typename T>
const parser<C, T>::tree& parser<C, T>::tree::get(const htref& h) {
	return static_cast<const tree&>(base_t::get(h));
}

template <typename C, typename T>
const htref parser<C, T>::tree::geth(tref id) {
	return base_t::geth(id);
}

template <typename C, typename T>
tref parser<C, T>::tree::get(const pnode& v, const tref* ch, size_t len) {
	return base_t::get(v, ch, len);
}

template <typename C, typename T>
tref parser<C, T>::tree::get(const pnode& v, const trefs& children) {
	return base_t::get(v, children);
}

template <typename C, typename T>
tref parser<C, T>::tree::get(const pnode& v, tref ch) {
	return base_t::get(v, ch);
}

template <typename C, typename T>
tref parser<C, T>::tree::get(const pnode& v, tref ch1, tref ch2) {
	return base_t::get(v, ch1, ch2);
}

template <typename C, typename T>
tref parser<C, T>::tree::get(const pnode& v) { return base_t::get(v); }

template <typename C, typename T>
tref parser<C, T>::tree::get(const pnode& v, const pnode* ch,
	size_t len)
{
	return base_t::get(v, ch, len);
}

template <typename C, typename T>
tref parser<C, T>::tree::get(const pnode& v,
	const std::vector<pnode>& ch)
{
	return base_t::get(v, ch);
}

template <typename C, typename T>
tref parser<C, T>::tree::get(const pnode& v, const pnode& child) {
	return base_t::get(v, child);
}

template <typename C, typename T>
tref parser<C, T>::tree::get(const pnode& v,
	const pnode& ch1, const pnode& ch2)
{
	return get(v, get(ch1), get(ch2));
}

template <typename C, typename T>
size_t parser<C, T>::tree::children_size() const {
	return base_t::children_size();
}

template <typename C, typename T>
bool parser<C, T>::tree::get_children(tref *child, size_t& len) const {
	return base_t::get_children(child, len);
}

template <typename C, typename T>
trefs parser<C, T>::tree::get_children() const {
	return base_t::get_children();
}

template <typename C, typename T>
tref_range<typename parser<C, T>::pnode> parser<C, T>::tree::children()
	const
{
	return base_t::children();
}

template <typename C, typename T>
tree_range<typename parser<C, T>::tree> parser<C, T>::tree::children_trees()
	const
{
	return tree_range<typename parser<C, T>::tree>(this->l);
}

template <typename C, typename T>
tref parser<C, T>::tree::child(size_t n) const {return base_t::child(n);}

template <typename C, typename T>
tref parser<C, T>::tree::first() const { return base_t::first(); }

template <typename C, typename T>
tref parser<C, T>::tree::second() const { return base_t::second(); }

template <typename C, typename T>
tref parser<C, T>::tree::third() const { return base_t::third(); }

template <typename C, typename T>
tref parser<C, T>::tree::only_child() const {
	return base_t::only_child();
}

template <typename C, typename T>
const parser<C, T>::tree& parser<C, T>::tree::operator[](size_t n) const {
	return child_tree(n);
}

template <typename C, typename T>
const parser<C, T>::tree& parser<C, T>::tree::child_tree(size_t n) const {
	return tree::get(base_t::child(n));
}

template <typename C, typename T>
const parser<C, T>::tree& parser<C, T>::tree::first_tree() const {
	return child_tree(0);
}

template <typename C, typename T>
const parser<C, T>::tree& parser<C, T>::tree::second_tree() const {
	return child_tree(1);
}

template <typename C, typename T>
const parser<C, T>::tree& parser<C, T>::tree::third_tree() const {
	return child_tree(2);
}

template <typename C, typename T>
const parser<C, T>::tree& parser<C, T>::tree::only_child_tree() const {
	return tree::get(base_t::only_child());
}

template <typename C, typename T>
const parser<C, T>::tree& parser<C, T>::tree::right_sibling_tree() const {
	tref s = this->right_sibling(); DBG(assert(s != nullptr);)
	return get(s);
}

template <typename C, typename T>
std::ostream& parser<C, T>::tree::print(std::ostream& o, size_t s) const {
	for (size_t i = 0; i < s; i++) o << "\t";
	o << this->value << "\n";
	for (const auto& ch : this->get_children()) get(ch).print(o, s + 1);
	return o;
}

template <typename C, typename T>
bool parser<C, T>::tree::is_nt() const { return this->value.first.nt(); }

template <typename C, typename T>
bool parser<C, T>::tree::is(size_t nt) const {
	return is_nt() && get_nt() == nt;
}

template <typename C, typename T>
bool parser<C, T>::tree::is_t() const { return !this->value.first.nt(); }

template <typename C, typename T>
std::string parser<C, T>::tree::get_terminals() const {
	std::stringstream ss;
	auto terminal_printer = [&ss](tref n) {
		if (n && get(n).is_t()) ss << get(n).get_t();
		return true;
	};
	pre_order<pnode>(get()).visit(terminal_printer);
	return ss.str();
}

template <typename C, typename T>
size_t parser<C, T>::tree::get_nt() const {
	if (is_nt()) return this->value.first.n();
	DBG(assert(false);)
	return 0;
}

template <typename C, typename T>
char parser<C, T>::tree::get_t() const {
	if (is_t()) return this->value.first.t();
	DBG(assert(false);)
	return 0;
}

template <typename C, typename T>
parser<C, T>::tree::traverser::traverser() : has_value_(false) {}
template <typename C, typename T>
parser<C, T>::tree::traverser::traverser(tref r) : has_value_(true), values_({ r }) {}
template <typename C, typename T>
parser<C, T>::tree::traverser::traverser(const trefs& n)
				: has_value_(n.size()), values_(n) {}
template <typename C, typename T>
bool parser<C, T>::tree::traverser::has_value() const { return has_value_; }

template <typename C, typename T>
parser<C, T>::tree::traverser::operator bool() const { return has_value(); }

template <typename C, typename T>
tref parser<C, T>::tree::traverser::value() const { return values_.front(); }

template <typename C, typename T>
const typename parser<C, T>::tree& parser<C, T>::tree::traverser::value_tree()
	const { return tree::get(values_.front()); }

template <typename C, typename T>
const typename parser<C, T>::tree& parser<C, T>::tree::traverser::operator[](
	size_t n) const { return value_tree()[n]; }

template <typename C, typename T>
const trefs& parser<C, T>::tree::traverser::values() const { return values_; }

template <typename C, typename T>
std::vector<typename parser<C, T>::tree::traverser>
	parser<C, T>::tree::traverser::traversers() const
{
	std::vector<traverser> tv;
	for (const auto& v : values_) tv.emplace_back(v);
	return tv;
}

template <typename C, typename T>
std::vector<typename parser<C, T>::tree::traverser>
	parser<C, T>::tree::traverser::operator()() const
{
	return traversers();
}

template <typename C, typename T>
typename parser<C, T>::tree::traverser
	parser<C, T>::tree::traverser::operator|(size_t nt) const
{
	if (!has_value()) return traverser();
	for (tref c : tree::get(value()).children()) {
		const auto& n = tree::get(c);
		if (n.is_nt() && n.get_nt() == nt) return { c };
	}
	return {};
}

template <typename C, typename T>
typename parser<C, T>::tree::traverser
	parser<C, T>::tree::traverser::operator||(size_t nt) const
{
	trefs r;
	for (tref v : values()) {
		for (tref c : tree::get(v).children()) {
			const auto& n = tree::get(c);
			if (n.is_nt() && n.get_nt() == nt) r.push_back(c);
		}
	}
	return traverser(r);
}

template <typename C, typename T>
template <typename result_type>
result_type parser<C, T>::tree::traverser::operator|(
	const extractor<result_type>& e) const
{
	return e(*this);
}

template <typename C, typename T>
template <typename result_type>
result_type parser<C, T>::tree::traverser::operator||(
	const extractor<result_type>& e) const
{
	return e(*this);
}

} // namespace idni