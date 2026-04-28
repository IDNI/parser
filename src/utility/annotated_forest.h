// annotated_forest.h — per-node mutable labels on top of forest<NodeT>
// Enables EL ABox concept labels, fragment flags, and arbitrary per-node metadata.

#ifndef __IDNI__UTILITY__ANNOTATED_FOREST_H__
#define __IDNI__UTILITY__ANNOTATED_FOREST_H__

#include <map>
#include "forest.h"

namespace idni {

template <typename NodeT, typename LabelT>
struct annotated_forest {
    using node      = typename forest<NodeT>::node;
    using nodes     = typename forest<NodeT>::nodes;
    using nodes_set = typename forest<NodeT>::nodes_set;
    using enter_t   = typename forest<NodeT>::enter_t;
    using exit_t    = typename forest<NodeT>::exit_t;
    using revisit_t = typename forest<NodeT>::revisit_t;
    using ambig_t   = typename forest<NodeT>::ambig_t;

    forest<NodeT>& f;
    std::map<node, LabelT> labels;

    explicit annotated_forest(forest<NodeT>& f_) : f(f_) {}

    LabelT&       label(const node& n)       { return labels[n]; }
    const LabelT& label(const node& n) const { return labels.at(n); }
    bool has_label(const node& n)      const { return labels.count(n) > 0; }

    // For each edge (parent -> child), call update_fn(label(parent), label(child))
    // and store result at child. Returns true if any label changed.
    template <typename UpdateFn>
    bool propagate(UpdateFn&& update_fn) {
        bool changed = false;
        f.traverse(f.root(),
            [](const node&) {},                         // enter: no-op
            [&](const node& n, const nodes_set& csets) {
                if (!has_label(n)) return;
                for (auto& cset : csets)
                    for (auto& child : cset) {
                        LabelT old_lbl = has_label(child) ? label(child) : LabelT{};
                        LabelT new_lbl = update_fn(label(n), old_lbl);
                        if (!(new_lbl == old_lbl)) {
                            labels[child] = std::move(new_lbl);
                            changed = true;
                        }
                    }
            },
            [](const node&) { return false; },          // no revisit
            [](const node&, const nodes_set& ns) { return ns; }  // no ambig filter
        );
        return changed;
    }

    // Forward traversal — delegates to forest::traverse.
    // Use the concrete std::function aliases so callers can pass generic lambdas.
    bool traverse(const node& root,
                  enter_t  cb_enter,
                  exit_t   cb_exit    = [](const node&, const nodes_set&){},
                  revisit_t cb_revisit = [](const node&){ return false; },
                  ambig_t  cb_ambig   = [](const node&, const nodes_set& ns){ return ns; }) {
        return f.traverse(root, cb_enter, cb_exit, cb_revisit, cb_ambig);
    }

    // Backward traversal — requires build_reverse_index() to have been called.
    template <typename cb_enter_t, typename cb_revisit_t>
    void traverse_backward(const nodes_set& starts,
                           cb_enter_t cb_enter,
                           cb_revisit_t cb_revisit) {
        f.traverse_backward(starts, cb_enter, cb_revisit);
    }

    template <typename cb_enter_t>
    void traverse_backward(const nodes_set& starts, cb_enter_t cb_enter) {
        f.traverse_backward(starts, cb_enter);
    }
};

} // idni namespace

#endif // __IDNI__UTILITY__ANNOTATED_FOREST_H__
