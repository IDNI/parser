# whitespace and comments
ws_comment   => '#' eol | '#' printable_chars eol.
ws_required  => space ws | ws_comment ws.
ws           => ws_required | null.
eol          => '\r' | '\n' | eof.
printable_chars => printable printable_chars1.
printable_chars1=> printable printable_chars1 | null.

# integer
sign         => '+' | '-'.
integer      => digit integer_rest.
integer_rest => digit integer_rest | null.

# expression
expr         => expr_op | term.
term         => term_op | factor.
expr_op      => expr ws addsub ws term.
term_op      => term ws muldiv ws factor.
addsub       => '+' | '-'.
muldiv       => '*' | '/'.
factor       => '(' ws expr ws ')' | sign ws factor | integer.

# statements
statement    => ws expr.
statements   =>     statement statements1.
statements1  => eol statement statements1 | null.
start        => statements ws | null.

