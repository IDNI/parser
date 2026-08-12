// To view the license please visit
// https://github.com/IDNI/parser/blob/main/LICENSE.md

#ifndef __IDNI__PARSER__UTILITY__TREE_TYPES_H__
#define __IDNI__PARSER__UTILITY__TREE_TYPES_H__

#include <cstdint>
#include <vector>
#include <memory>
#include <utility>
#include <type_traits>

namespace idni {

// @brief tree node reference
using tref  = const int64_t*;
// @brief vector of tree node reference handles
using trefs = std::vector<tref>;

template <typename T> struct bintree;
template <typename T> struct tref_range;
template <typename T> struct tree_range;

// @brief tree handle wrapping a tref. htree::sp prevents tree from being gc-ed
struct htree {
	using sp = std::shared_ptr<htree>;
	using wp = std::weak_ptr<htree>;
	static const sp& null();
	inline tref get() const;
	inline bool operator==(const htree& r) const;
	inline bool operator< (const htree& r) const;
	//~htree();
private:
	tref hnd;
	explicit htree(tref id = nullptr);
	template <typename N> friend struct bintree;
};

using htref  = htree::sp;
using htrefs = std::vector<htref>;

// helper type traits to check callback properties
template<typename Cb>
using bool_accepts_tref = std::is_invocable_r<bool, Cb, tref>;

template<typename Cb>
using bool_accepts_tref_tref = std::is_invocable_r<bool, Cb, tref, tref>;

template<typename Cb>
using accepts_tref_tref = std::is_invocable<Cb, tref, tref>;

//------------------------------------------------------------------------------
// rewriter types

namespace rewriter {

using rule = std::pair<htref, htref>;
using rules = std::vector<rule>;

using library = rules;
using builder = rule;

} // namespace rewriter

} // namespace idni

#endif // __IDNI__PARSER__UTILITY__TREE_TYPES_H__
