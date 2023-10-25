## Functions

Parsing library provides several functions for:
- dealing with UTF-8 stored in `char` or `unsigned char`.
- conversion of UTF-8 to Unicode in `char32_t` and back.
- UTF-8 to char32_t decoder and char32_t to UTF-8 encoder usable for UTF-8 capable `parser<char, char32_t>`
- cctype based character class functions
- transforming a parsed forest into several formats (DOT, TML) to help with building grammars
- flattening children terminals of a forest node into string or int









// 5. A more advanced sample (say a calculator), that shows more steps like how to traverse parse trees