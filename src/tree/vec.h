#include <vector>
#include <set>

size_t get_vec_len(size_t n);
const size_t* get_vec(size_t n);
size_t get_vec(const size_t n, const size_t* v);
std::vector<size_t> gc_vec(const std::set<size_t>& s); /// keep those in s
