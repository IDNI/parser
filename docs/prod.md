[back to index](../README.md#classes-and-structs)

# prod

```
template <typename C = char, typename T = C> class prod;
```

`prod` represents a production rule of a grammar.

Class has the same API as `std::pair<lit<C, T>, disjs<C, T>>`.

`first` pair member is a rule's head literal and the `second` pair member is rule's body in a form of a disjunction `disjs`.
