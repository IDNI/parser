#include "bintree.h"
#include <iostream>

int main()
{
    using namespace idni;
    using namespace idni2;
    using nd_t = char;
    nd_t n1,n2, n3, n4;
    n1 = '1';
    n2 = '2', n3 ='3', n4= '4';
    {
        nd_t child [] = {n2, n3};
        auto tid = tree<nd_t>::get(n1, child, 2);
        auto handle = tree<nd_t>::get(tid);
        tree<nd_t>::get(tid).print();
    }
    // above handle goes out of scope
    {
    nd_t arr [] = { n1,n2 ,n3,n4 };
    auto id = tree<nd_t>::get(n2, arr, 4);
    //get shared handle for id to avoid being gced.
    auto sh = tree<nd_t>::geth(id);
    //print though id
    tree<nd_t>::get(id).print();
    // prev tree handle that was out of scope is garbage out
    bintree<nd_t>::gc();
    
    // print through sh handle (different id) but same tree
    tree<nd_t>::get(sh).print();
    // create a new tree
    auto h3 = bintree<nd_t>::get('C', sh->get(), sh->get());
    tree<nd_t>::get(h3).print();
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
        tree<nd_t2>::get(tid).print();
    }
    // above handle goes out of scope
    {
    nd_t2 arr [] = { n1,n2 ,n3,n4 };
    auto id = tree<nd_t2>::get(n2, arr, sizeof(arr)/sizeof(nd_t2));
    //get shared handle for id to avoid being gced.
    auto sh = bintree<nd_t2>::geth(id);
    //print though id
    tree<nd_t2>::get(id).print();
    // prev tree handle that was out of scope is garbage out
    bintree<nd_t2>::gc();
    // print through sh handle (different id) but same tree
    tree<nd_t2>::get(sh).print();
    // create a new tree through bintree
    auto h3 = bintree<nd_t2>::get("C", sh->get(), sh->get());
    tree<nd_t2>::get(h3).print();
    // dont save its handle
    }

    bintree<nd_t2>::gc();

    }
}