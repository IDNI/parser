[back to index](../README.md#classes-and-structs)

# parser::error

`prser<C, T>::error` is a structure returned by parser's method `parser<C, T>::get_error()`.

Call this if your input wasn't parsed successfully and you want to know more details about the error.

## attributes

### int_t loc

`loc` is a location of the error

### size_t line

`line` of the error

### size_t col

`col` is a column of the error

### std::vector<T> ctxt

Closest matching context

### T unexp

Terminal which was scanned in a place where it was not expected

### std::vector<exp_prod_t> expv

List of expected tokens and respective productions. Is a vector of `exp_prod_t` structs.

```
	typedef struct _exp_prod {
		std::string exp;
		std::string prod_nt;
		std::string prod_body;
		// back track information to higher derivations
		std::vector<_exp_prod> bktrk;
	} exp_prod_t;
```

## methods

### std::string to_str(info_lvl lvl);

This method produces a string containing human readable information about the error.

There are three `info_lvl`'s of details you can obtain:
```
	enum info_lvl {
		INFO_BASIC,
		INFO_DETAILED,
		INFO_ROOT_CAUSE
	};
```
Default is `INFO_ROOT_CAUSE`.


## example

See [basic arithmetic calculator](../examples/basic_arithmetic/main.cpp) example.

With `INFO_DETAILED` we get the following error for entering `"1-"` as an input

```
Syntax Error: Unexpected end of file at 1:3 (3): 1-
...expecting space due to (ws => • space ws [2,2])
...expecting '(' due to (factor => • '(' ws expr1 ws ')' [2,2])
...expecting digit due to (integer => • digit integer1 [2,2])
...expecting '-' due to (neg => • '-' ws factor [2,2])
...expecting '+' due to (pos => • '+' ws factor [2,2])
```

For input `"5@2"` we get
```
Syntax Error: Unexpected '@' at 1:2 (2): 5@
...expecting '+' due to (add => expr1 ws • '+' ws expr2 [0,1])
...expecting '-' due to (sub => expr1 ws • '-' ws expr2 [0,1])
...expecting '*' due to (mul => expr2 ws • '*' ws factor [0,1])
...expecting space due to (ws => • space ws [1,1])
...expecting '-' due to (div => expr2 ws • '-' ws factor [0,1])
...expecting digit due to (integer1 => • digit integer1 [1,1])
```

`INFO_BASIC` level prints just the first line (without lines containing what was expected)
