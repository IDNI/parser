#include "utility/tree.h"
#include <iostream>

using namespace idni;

-void post_order_recursive(tref root, std::vector<int>& res) {
    if (!root) return;
    const auto& node = bintree<int>::get(root);
    post_order_recursive(node.l, res);
    post_order_recursive(node.r, res);
    res.push_back(node.value);
}

void morris_post_order_traverse(tref root, std::vector<int>& res) {
    if (!root) return;
    
    morris_post_order<int> traverser(root);
    auto visitor = [&res](tref node) -> bool {
        if (node != nullptr) {
            res.push_back(bintree<int>::get(node).value);
        }
        return true;
    };
    
    traverser.search(visitor);
}

bool test_morris_traversal() {
    bool all_passed = true;
    
    auto run_test = [&all_passed](const std::string& name, tref root) {
        std::cout << "Test: " << name << '\n';
        
        std::vector<int> rec_res, morris_res;
        
        post_order_recursive(root, rec_res);
        morris_post_order_traverse(root, morris_res);
        
        std::cout << "Recursive: ";
        for (auto v : rec_res) std::cout << v << " ";
        std::cout << '\n';
        
        std::cout << "Morris: ";
        for (auto v : morris_res) std::cout << v << " ";
        std::cout << '\n';
        
        bool passed = (rec_res == morris_res);
        std::cout << (passed ? "PASSED" : "FAILED") << "\n\n";
        
        all_passed &= passed;
    };
    
    // Empty tree
    run_test("Empty", nullptr);
    
    /*
     * Single node tree:
     * 1
     */
    tref single = bintree<int>::get(1, nullptr, nullptr);
    run_test("Single", single);
    
    /*
     * Simple tree:
     *   1
     *  / \
     * 2   3
     */
    tref simple = bintree<int>::get(1, 
                    bintree<int>::get(2, nullptr, nullptr),
                    bintree<int>::get(3, nullptr, nullptr));
    run_test("Simple", simple);
    
    /*
     * Left-skewed tree:
     * 1
     * |
     * 2
     * |
     * 3
     * |
     * 4
     */
    tref l4 = bintree<int>::get(4, nullptr, nullptr);
    tref n3 = bintree<int>::get(3, l4, nullptr);
    tref n2 = bintree<int>::get(2, n3, nullptr);
    tref l_skew = bintree<int>::get(1, n2, nullptr);
    run_test("LeftSkew", l_skew);
    
    /*
     * Right-skewed tree:
     * 1
     *  \
     *   2
     *    \
     *     3
     *      \
     *       4
     */
    tref r4 = bintree<int>::get(4, nullptr, nullptr);
    tref r3 = bintree<int>::get(3, nullptr, r4);
    tref r2 = bintree<int>::get(2, nullptr, r3);
    tref r_skew = bintree<int>::get(1, nullptr, r2);
    run_test("RightSkew", r_skew);
    
    /*
     * Complex tree:
     *         1
     *       /   \
     *      2     3
     *     / \   / \
     *    4   5 6   7
     *   /   / \     \
     *  8   9  10     11
     *     / \       /
     *    12 13     14
     *        \    / \
     *        15  16 17
     */
    tref n8 = bintree<int>::get(8, nullptr, nullptr);
    tref n12 = bintree<int>::get(12, nullptr, nullptr);
    tref n13 = bintree<int>::get(13, nullptr, nullptr);
    tref n16 = bintree<int>::get(16, nullptr, nullptr);
    tref n17 = bintree<int>::get(17, nullptr, nullptr);
    
    tref n9 = bintree<int>::get(9, n12, n13);
    tref n10 = bintree<int>::get(10, nullptr, nullptr);
    tref n14 = bintree<int>::get(14, n16, n17);
    
    tref n4 = bintree<int>::get(4, n8, nullptr);
    tref n5 = bintree<int>::get(5, n9, n10);
    tref n6 = bintree<int>::get(6, nullptr, nullptr);
    tref n11 = bintree<int>::get(11, n14, nullptr);
    tref n7 = bintree<int>::get(7, nullptr, n11);
    
    tref n2c = bintree<int>::get(2, n4, n5);
    tref n3c = bintree<int>::get(3, n6, n7);
    
    tref complex = bintree<int>::get(1, n2c, n3c);
    run_test("Complex", complex);
    
    std::cout << "Result: " << (all_passed ? "ALL PASSED" : "SOME FAILED") << "\n";
    return all_passed;
}


int main() {
    using nd_t = char;
    nd_t n1,n2, n3, n4;
    n1 = '1';
    n2 = '2', n3 ='3', n4= '4';
    {
        nd_t child [] = {n2, n3};
        auto tid = tree<nd_t>::get(n1, child, 2);
        // auto handle = tree<nd_t>::get(tid);
        tree<nd_t>::get(tid).print(std::cout);
    }
    // above handle goes out of scope
    {
    nd_t arr [] = { n1,n2 ,n3,n4 };
    auto id = tree<nd_t>::get(n2, arr, 4);
    //get shared handle for id to avoid being gced.
    auto sh = tree<nd_t>::geth(id);
    //print though id
    tree<nd_t>::get(id).print(std::cout);
    // prev tree handle that was out of scope is garbage out
    bintree<nd_t>::gc();
    
    // print through sh handle (different id) but same tree
    tree<nd_t>::get(sh).print(std::cout);
    // create a new tree
    auto h3 = bintree<nd_t>::get('C', sh->get(), sh->get());
    tree<nd_t>::get(h3).print(std::cout);
    // dont save its handle
    }

    bintree<nd_t>::gc(); 

    // now using string
    {
    using nd_t2 = std::string;
    nd_t2 n1,n2, n3, n4;
    n1 = "1";
    n2 = "2", n3 ="3", n4= "4";
    {
        nd_t2 child [] = {n2, n3};
        auto tid = tree<nd_t2>::get(n1, child, sizeof(child)/sizeof(nd_t2));
        auto handle = tree<nd_t2>::get(tid);
        tree<nd_t2>::get(tid).print(std::cout);
    }
    // above handle goes out of scope
    {
    nd_t2 arr [] = { n1,n2 ,n3,n4 };
    auto id = tree<nd_t2>::get(n2, arr, sizeof(arr)/sizeof(nd_t2));
    //get shared handle for id to avoid being gced.
    auto sh = bintree<nd_t2>::geth(id);
    //print though id
    tree<nd_t2>::get(id).print(std::cout);
    // prev tree handle that was out of scope is garbage out
    bintree<nd_t2>::gc();
    // print through sh handle (different id) but same tree
    tree<nd_t2>::get(sh).print(std::cout);
    // create a new tree through bintree
    auto h3 = bintree<nd_t2>::get("C", sh->get(), sh->get());
    tree<nd_t2>::get(h3).print(std::cout);
    // dont save its handle
    }

    bintree<nd_t2>::gc();

    test_morris_traversal();

    }

}