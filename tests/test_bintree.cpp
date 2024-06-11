#include "parser.h"
#include "btree.h"
#include <iostream>

int main()
{
    using namespace idni;
    using nd_t = char;
    nd_t n1,n2, n3, n4;
    n1 = 1;
    n2 = 2, n3 =3, n4= 4;
    {
    auto handle = tree<nd_t>::get(n1, {n2,n3});
    tree<nd_t>::get(handle).print();
   // std::cout<<handle.hnd<<std::endl;
    auto bn = tree<nd_t>::get(handle);
    auto h2 = tree<nd_t>::get(n2, {n1,n2, n3,n4});
    tree<nd_t>::get(h2).print();
    //auto h3 = tree<nd_t>::get(n4, {h2, handle});
    auto h3 = bintree<nd_t>::get('C', h2, handle);
    bintree<nd_t>::get(h3).print();
    tree<nd_t>::get(h3).print();
    }
    auto a = htree::null();


}