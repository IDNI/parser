 #include "bintree.h"

namespace idni {

std::unordered_map<tref, htree::wp> htree::M;

htree::sp htree::get(tref h) {
    DBG(assert(h != NULL);)
    if (h == NULL) return null();
    auto res = M.emplace(h, wp()); //done with one search
    sp ret; 
    if (res.second) 
        res.first->second = ret = sp(new htree(h));
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
