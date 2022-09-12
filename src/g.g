start           => productions ws.
productions     => ws production productions_rest.
productions_rest=> ws production productions_rest | null.
production      => sym ws production_sep ws alts constraints ws '.'.
production_sep  => "=>".
alts            => alt alts_rest.
alts_rest       => ws alt_separator ws alt alts_rest | null.
alt_separator   => '|'.
alt             => nt_nt_alt | nt_t_alt | t_nt_alt | t_t_alt.
t_t_alt         => t_t_factor  | t_t_factor ws t_t_alt  | t_t_factor ws nt_t_alt.
t_nt_alt        => t_t_factor ws t_nt_alt | t_t_factor ws nt_nt_alt.
nt_t_alt        => nt_t_factor | nt_nt_factor ws_required nt_t_alt |
			nt_nt_factor ws t_t_alt |
			nt_t_factor ws nt_t_alt |
			nt_t_factor ws t_t_alt.
nt_nt_alt       => nt_nt_factor | nt_nt_factor ws_required nt_nt_alt |
			nt_nt_factor ws t_nt_alt |
			nt_t_factor ws nt_nt_alt |
			nt_t_factor ws t_nt_alt.
factor_group    => '(' ws alt ws ')'.
factor_optional => '[' ws alt ws ']'.
factor_plus     => '{' ws alt ws '}'.
factor_unot     => terminal ws unot | factor_group ws unot.
t_t_factor      => terminal | factor_group | factor_plus | factor_optional |
			factor_unot.
nt_t_factor     => nt_factor_unot.
nt_factor_unot  => nonterminal ws unot.
nt_nt_factor    => nonterminal.
unot            => '*' | '+'.
terminal        => quoted_char | string.
nonterminal     => sym.
constraints     => ws ',' ws constraint constraints | null.
constraint      => preference.
preference      => pref | priority.
pref            => "_pref".
priority        => integer.
integer         => digits.
digits          => digit digits_rest.
digits_rest     => digit digits_rest | null.
sym             => chars.
chars           => alpha chars1 | '_' chars1.
chars1          => alnum chars1 | '_' chars1 | null.
printable_chars => printable printable_chars1.
printable_chars1=> printable printable_chars1 | null.
ws_comment      => '#' eol | '#' printable_chars eol.
ws_required     => space ws | ws_comment ws.
ws              => ws_required | null.
eol             => '\r' | '\n' | eof.
string          => '"' string_chars '"'.
string_chars    => string_char string_chars1.
string_chars1   => string_char string_chars1 | null.
string_char     => char0 | "'" | "\\\"".
quoted_char     => "'" char "'" | quoted_char_esc.
quoted_char_esc => "'\\" printable "'".
char            => char0 | "\\'" | '"'.
char0           => alnum | space |
	'!' | '#' | '$' | '%' | '&' | '(' | ')' | '*' | '+' | ',' |
	'-' | '.' | '/' | ':' | ';' | '<' | '=' | '>' | '?' | '@' |
	'[' | '\\'| ']' | '^' | '_' | '{' | '|' | '}' | '~'.
