 #include "bintree.h"

namespace idni {

std::unordered_map<int_t, htree::wp> htree::M;

htree::sp htree::get(int_t h) {
    DBG(assert(h >= -1);)
    if (h == -1) return null();
    auto res = M.emplace(h, wp()); //done with one search
    if (res.second)
        return res.first->second = nsp(new htree(h));
    assert(!res.first->second.expired());
    return res.first->second.lock(); 
}

void htree::dump() {
	std::cout<<"-----\n";
	std::cout<<"S:"<<M.size()<<"\n";
	for (auto &x : M)
		std::cout<<x.first << " " <<&x.second <<
		" "<< x.second.lock()->get() <<"\n";
	std::cout<<"-----\n";
}

}
