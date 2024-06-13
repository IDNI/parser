[back to index](../README.md#classes-and-structs)

# conjs

```
template <typename C = char, typename T = C> class conjs;
```

`conjs` represent a conjunction of `lits` (literal sequences). If there are more `lits` in conjunction all of them has to match the input to satisfy a production rule.

Class has the same API as `std::set<lits<C, T>>`
