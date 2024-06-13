[back to index](../README.md#classes-and-structs)

# parser::pnode

`pnode` is a structure used as a forest node. It maps nodes to pointers to make sure the forest does not contain duplicities.

It has the same api as `std::pair<lit<C, T>, std::array<size_t, 2>>` containing the literal and it's position span in the input.
