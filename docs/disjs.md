[back to index](../README.md#classes-and-structs)

# disjs

```
template <typename C = char, typename T = C> class disjs;
```

`disjs` represent a disjunction of `conjs` (disjunction of conjunctions of literal sequences). Any conjunction can match an input to satisfy a production rule.

Class has the same API as `std::set<conjs<C, T>>`
