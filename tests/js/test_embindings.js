#!/usr/bin/env node
/**
 * Exercises the tauparser embind class API (lit, prods, grammar, parser,
 * tree, parse_result, parse_options, shaping_options, tgf_repl) directly.
 * Usage: node tests/js/test_embindings.js
 */
'use strict';

const assert = require('node:assert');
const path = require('path');

function vec_to_array(v) {
	const n = v.size();
	const out = [];
	for (let i = 0; i < n; ++i) out.push(v.get(i));
	return out;
}

// embind may surface a C++ char as a JS number (code point) or as a
// one-character string depending on toolchain version; normalize both.
function char_code(c) {
	return typeof c === 'string' ? c.charCodeAt(0) : c;
}

async function main() {
	const buildDir = path.resolve(__dirname, '..', '..', 'build', 'release-wasm');
	process.chdir(buildDir);
	const createTauparser = require(path.join(buildDir, 'tauparser.js'));
	const tauparser = await createTauparser();

	const tinySrc = '@use char classes digit.\n\nstart => num.\nnum   => digit+.\n';
	const ambigSrc = '@use char classes digit.\n\nstart => num.\nnum   => digit | digit digit.\n';
	const missingPath = '/no/such/file/does/not/exist.tgf';

	// ---- free functions: version, test_isprint ----

	const ver = tauparser.version();
	assert.ok(ver.startsWith('tauparser-js '), `unexpected version: ${ver}`);
	assert.ok(ver.length > 'tauparser-js '.length, 'version has no suffix');

	assert.strictEqual(tauparser.test_isprint('A'.charCodeAt(0)), true);
	assert.strictEqual(tauparser.test_isprint(0), false);
	assert.strictEqual(tauparser.test_isprint(10), false);

	// ---- free functions: load_grammar, parse_grammar_str ----
	// load_grammar's success branch is covered later, once a grammar file
	// has been written into the virtual filesystem.

	const loadBad = tauparser.load_grammar(missingPath);
	assert.ok(loadBad.startsWith('0\n'),
		`load_grammar(missing file) should report failure: ${loadBad}`);

	const parseOk = tauparser.parse_grammar_str(tinySrc);
	assert.ok(parseOk.startsWith('1\n'), `parse_grammar_str(valid) failed: ${parseOk}`);

	const parseBad = tauparser.parse_grammar_str('not @@@ a valid tgf file (((');
	assert.ok(parseBad.startsWith('0\n'),
		`parse_grammar_str(invalid) should report failure: ${parseBad}`);
	assert.ok(parseBad.length > 2, 'parse_grammar_str(invalid) has no diagnostics');

	// ---- lit: default (null) literal ----

	const nullLit = new tauparser.lit();
	assert.strictEqual(nullLit.is_null(), true);
	assert.strictEqual(nullLit.is_nt(), false);
	assert.strictEqual(nullLit.is_terminal(), false);
	nullLit.delete();

	// ---- prods: from_char, from_lit, from_string, to_lit round trips ----

	const litA = tauparser.prods.from_char('a'.charCodeAt(0)).to_lit();
	assert.strictEqual(litA.is_terminal(), true);
	assert.strictEqual(litA.is_nt(), false);
	assert.strictEqual(char_code(litA.t()), 'a'.charCodeAt(0));
	assert.strictEqual(litA.to_string(), "'a'");

	const litARound = tauparser.prods.from_lit(litA).to_lit();
	assert.strictEqual(litARound.to_string(), "'a'");

	// from_string builds a concatenation; to_lit() on a multi-symbol prods
	// returns the trailing literal of the sequence, not the first one.
	const litXY = tauparser.prods.from_string('xy').to_lit();
	assert.strictEqual(litXY.is_terminal(), true);
	assert.strictEqual(litXY.to_string(), "'y'");

	// to_lit() on a default-constructed (empty) prods has no trailing
	// literal to return, so it comes back as a null literal.
	const emptyLit = new tauparser.prods().to_lit();
	assert.strictEqual(emptyLit.is_null(), true);
	assert.strictEqual(emptyLit.is_nt(), false);
	assert.strictEqual(emptyLit.is_terminal(), false);

	// ---- grammar: manual construction of "start => 'a' 'b' | 'c'." ----

	const g = new tauparser.grammar();
	const a = tauparser.prods.from_char('a'.charCodeAt(0));
	const b = tauparser.prods.from_char('b'.charCodeAt(0));
	const c = tauparser.prods.from_char('c'.charCodeAt(0));
	const body = tauparser.prods_or(tauparser.prods_concat(a, b), c);
	g.add_rule('start', body);
	assert.strictEqual(g.build('start'), true);
	assert.strictEqual(g.good(), true);
	assert.ok(g.size() >= 1, 'built grammar has no productions');

	const startId = g.nt('start');
	assert.strictEqual(typeof startId, 'number');
	assert.strictEqual(g.nt_name(startId), 'start');
	assert.strictEqual(g.nt_lit('start').nt(), startId);
	assert.strictEqual(g.nt_lit('start').is_nt(), true);
	assert.strictEqual(g.nt_lit_by_id(startId).nt(), startId);

	const startRef = g.lit('start').to_lit();
	assert.strictEqual(startRef.is_nt(), true);
	assert.strictEqual(startRef.nt(), startId);

	const startLit = g.start_literal();
	assert.strictEqual(startLit.is_nt(), true);
	assert.strictEqual(startLit.nt(), startId);

	const prod0 = g.production_to_string(0);
	assert.strictEqual(typeof prod0, 'string');
	assert.ok(prod0.length > 0, 'production_to_string(0) is empty');

	const internal = g.internal_grammar();
	assert.ok(internal.includes('start'), 'internal_grammar() missing start symbol');

	assert.deepStrictEqual(vec_to_array(g.unreachable('start')), [],
		'nothing should be unreachable from start yet');

	// parser.init(grammar) transfers ownership: the grammar_wrap loses its
	// grammar/nonterminals and good() becomes false afterwards.
	const p = new tauparser.parser();
	assert.strictEqual(p.init(g), true);
	assert.strictEqual(g.good(), false, 'grammar_wrap should be emptied by init()');
	assert.strictEqual(p.good(), true);

	const rAB = p.parse('ab');
	assert.strictEqual(rAB.good(), true);
	assert.strictEqual(rAB.found(), true);
	assert.strictEqual(rAB.get_input(), 'ab');
	assert.strictEqual(rAB.get_terminals(), 'ab');
	assert.strictEqual(rAB.error_string(), '');
	assert.strictEqual(rAB.has_single_parse_tree(), true);
	assert.strictEqual(rAB.is_ambiguous(), false);

	const treeAB = rAB.get_bintree();
	assert.strictEqual(treeAB.has_value(), true);
	assert.strictEqual(treeAB.is_nt(), true);
	assert.strictEqual(treeAB.is_terminal(), false);
	assert.strictEqual(treeAB.is_null(), false);
	assert.strictEqual(treeAB.nt(), startId);
	assert.strictEqual(treeAB.text(), 'ab');
	assert.deepStrictEqual(vec_to_array(treeAB.location()), [0, 2]);

	const childrenAB = vec_to_array(treeAB.children());
	assert.ok(childrenAB.length >= 1, 'root tree has no children');

	const rC = p.parse('c');
	assert.strictEqual(rC.found(), true);
	assert.strictEqual(rC.get_bintree().text(), 'c');

	// good() reflects the input stream's state, not whether a match was
	// found, so it stays true here; found() is the actual match indicator.
	const rBad = p.parse('ac');
	assert.strictEqual(rBad.found(), false);
	assert.strictEqual(rBad.good(), true);
	assert.ok(rBad.error_string().length > 0, 'failed parse has no error message');
	assert.strictEqual(rBad.get_bintree().has_value(), false);

	// ---- grammar: prods_not and unreachable productions ----

	const g2 = new tauparser.grammar();
	const c2 = tauparser.prods.from_char('c'.charCodeAt(0));
	g2.add_rule('start', tauparser.prods_not(c2));
	g2.add_rule('unused', tauparser.prods.from_char('z'.charCodeAt(0)));
	assert.strictEqual(g2.build('start'), true);

	// prods_not() allocates a synthetic nonterminal for the negated literal;
	// that node, not the plainly-unused 'unused' rule, is what unreachable()
	// reports here.
	const unreached = vec_to_array(g2.unreachable('start'));
	assert.strictEqual(unreached.length, 1);
	assert.strictEqual(g2.nt_name(unreached[0]), '__N_0');

	const p2 = new tauparser.parser();
	assert.strictEqual(p2.init(g2), true);

	const rNotC = p2.parse('a');
	assert.strictEqual(rNotC.found(), true, '"a" should match anything but c');

	const rIsC = p2.parse('c');
	assert.strictEqual(rIsC.found(), false, '"c" should be rejected by prods_not(c)');

	// ---- grammar: self-referencing rule; default auto-disambiguation
	// collapses "aa"'s two derivations down to one concrete tree ----
	// S => a S | S a | a

	const g3 = new tauparser.grammar();
	const litAforS = tauparser.prods.from_char('a'.charCodeAt(0));
	const sRef = g3.lit('S');
	const alt1 = tauparser.prods_concat(litAforS, sRef);
	const alt2 = tauparser.prods_concat(sRef, litAforS);
	g3.add_rule('S', tauparser.prods_or(tauparser.prods_or(alt1, alt2), litAforS));
	assert.strictEqual(g3.build('S'), true);

	const p3 = new tauparser.parser();
	assert.strictEqual(p3.init(g3), true);

	const rAmbig = p3.parse('aa');
	assert.strictEqual(rAmbig.found(), true);
	assert.strictEqual(rAmbig.is_ambiguous(), false,
		'default auto-disambiguation should collapse ambiguity');
	assert.strictEqual(rAmbig.has_single_parse_tree(), true);

	// ---- grammar.auto_disambiguate / add_nodisambig: keeping real
	// ambiguity alive, from TGF directives and from the manual API ----

	const ambTgfSrc =
		'@use char class any.\n' +
		'@disable auto disambiguation.\n' +
		'start => A | B.\n' +
		'A => \'x\'.\n' +
		'B => \'x\'.\n';
	const pTgfAmbig = new tauparser.parser();
	assert.strictEqual(pTgfAmbig.from_tgf_string(ambTgfSrc), true);
	const rTgfAmbig = pTgfAmbig.parse('x');
	assert.strictEqual(rTgfAmbig.found(), true);
	assert.strictEqual(rTgfAmbig.is_ambiguous(), true,
		'@disable auto disambiguation should leave the A/B choice ambiguous');
	assert.strictEqual(rTgfAmbig.has_single_parse_tree(), false);

	// same grammar, built through add_rule/prods and disambiguated off
	// through the new property instead of a TGF directive
	const gManualAmbig = new tauparser.grammar();
	gManualAmbig.add_rule('A', tauparser.prods.from_char('x'.charCodeAt(0)));
	gManualAmbig.add_rule('B', tauparser.prods.from_char('x'.charCodeAt(0)));
	gManualAmbig.add_rule('start',
		tauparser.prods_or(gManualAmbig.lit('A'), gManualAmbig.lit('B')));
	assert.strictEqual(gManualAmbig.auto_disambiguate, true,
		'auto_disambiguate should default to true');
	gManualAmbig.auto_disambiguate = false;
	assert.strictEqual(gManualAmbig.auto_disambiguate, false);
	assert.strictEqual(gManualAmbig.build('start'), true);

	const pManualAmbig = new tauparser.parser();
	assert.strictEqual(pManualAmbig.init(gManualAmbig), true);
	const rManualAmbig = pManualAmbig.parse('x');
	assert.strictEqual(rManualAmbig.found(), true);
	assert.strictEqual(rManualAmbig.is_ambiguous(), true,
		'auto_disambiguate=false should leave the A/B choice ambiguous');
	assert.strictEqual(rManualAmbig.has_single_parse_tree(), false);

	// same grammar again, auto_disambiguate left on but "start" exempted
	// by name through add_nodisambig
	const gExemptAmbig = new tauparser.grammar();
	gExemptAmbig.add_rule('A', tauparser.prods.from_char('x'.charCodeAt(0)));
	gExemptAmbig.add_rule('B', tauparser.prods.from_char('x'.charCodeAt(0)));
	gExemptAmbig.add_rule('start',
		tauparser.prods_or(gExemptAmbig.lit('A'), gExemptAmbig.lit('B')));
	gExemptAmbig.add_nodisambig('start');
	assert.strictEqual(gExemptAmbig.build('start'), true);

	const pExemptAmbig = new tauparser.parser();
	assert.strictEqual(pExemptAmbig.init(gExemptAmbig), true);
	const rExemptAmbig = pExemptAmbig.parse('x');
	assert.strictEqual(rExemptAmbig.found(), true);
	assert.strictEqual(rExemptAmbig.is_ambiguous(), true,
		'add_nodisambig("start") should keep "start" ambiguous even with auto_disambiguate on');
	assert.strictEqual(rExemptAmbig.has_single_parse_tree(), false);

	// ---- @ambiguous directive: same exemption, reached from TGF source ----

	const ambDirectiveSrc =
		'@use char class any.\n' +
		'@ambiguous start.\n' +
		'start => A | B.\n' +
		'A => \'x\'.\n' +
		'B => \'x\'.\n';
	const pDirectiveAmbig = new tauparser.parser();
	assert.strictEqual(pDirectiveAmbig.from_tgf_string(ambDirectiveSrc), true);
	const rDirectiveAmbig = pDirectiveAmbig.parse('x');
	assert.strictEqual(rDirectiveAmbig.found(), true);
	assert.strictEqual(rDirectiveAmbig.is_ambiguous(), true,
		'@ambiguous start should keep "start" ambiguous even with auto_disambiguate on');
	assert.strictEqual(rDirectiveAmbig.has_single_parse_tree(), false);

	// ---- grammar/parser: from_tgf_file failure branch (missing path) ----

	const g4bad = new tauparser.grammar();
	assert.strictEqual(g4bad.from_tgf_file(missingPath), false);
	assert.strictEqual(g4bad.good(), false);

	const p4bad = new tauparser.parser();
	assert.strictEqual(p4bad.from_tgf_file(missingPath), false);
	assert.strictEqual(p4bad.good(), false);

	// ---- parser_wrap: a failed reload after a good load must not leave a
	// dangling grammar/parser behind, and the object must stay usable ----

	const p4recover = new tauparser.parser();
	assert.strictEqual(p4recover.from_tgf_string(tinySrc), true);
	assert.strictEqual(p4recover.good(), true);
	const rBeforeFail = p4recover.parse('5');
	assert.strictEqual(rBeforeFail.found(), true);

	assert.strictEqual(
		p4recover.from_tgf_string('not @@@ a valid tgf file((('), false);
	assert.strictEqual(p4recover.good(), false,
		'a failed reload must leave the parser not good');

	// no crash on a following call, and it reports a clean "not good" result
	const rAfterFail = p4recover.parse('5');
	assert.strictEqual(rAfterFail.good(), false);
	assert.strictEqual(rAfterFail.found(), false);

	// the object remains usable: a subsequent valid load works normally
	assert.strictEqual(p4recover.from_tgf_string(tinySrc), true);
	assert.strictEqual(p4recover.good(), true);
	const rRecovered = p4recover.parse('9');
	assert.strictEqual(rRecovered.found(), true);
	assert.strictEqual(rRecovered.get_terminals(), '9');

	// from_tgf_file: same failed-reload treatment
	const p4fileRecover = new tauparser.parser();
	assert.strictEqual(p4fileRecover.from_tgf_string(tinySrc), true);
	assert.strictEqual(p4fileRecover.from_tgf_file(missingPath), false);
	assert.strictEqual(p4fileRecover.good(), false);
	assert.strictEqual(p4fileRecover.parse('1').good(), false);

	// ---- grammar_wrap: a failed reload after a good load must not leave a
	// stale grammar reachable, and the object must stay usable ----

	const g4recover = new tauparser.grammar();
	assert.strictEqual(g4recover.from_tgf_string(tinySrc), true);
	assert.strictEqual(g4recover.good(), true);

	assert.strictEqual(
		g4recover.from_tgf_string('not @@@ a valid tgf file((('), false);
	assert.strictEqual(g4recover.good(), false,
		'a failed reload must leave the grammar not good');

	// no crash on a following call, and it reports a clean "empty" grammar
	assert.strictEqual(g4recover.size(), 0);

	// the object remains usable: a subsequent valid load works normally
	assert.strictEqual(g4recover.from_tgf_string(tinySrc), true);
	assert.strictEqual(g4recover.good(), true);
	assert.strictEqual(g4recover.nt_name(g4recover.nt('start')), 'start');

	// from_tgf_file: same failed-reload treatment
	const tinyGrammarPath = '/tiny_reload.tgf';
	tauparser.FS.writeFile(tinyGrammarPath, tinySrc);

	const g4fileRecover = new tauparser.grammar();
	assert.strictEqual(g4fileRecover.from_tgf_string(tinySrc), true);
	assert.strictEqual(g4fileRecover.from_tgf_file(missingPath), false);
	assert.strictEqual(g4fileRecover.good(), false);
	assert.strictEqual(g4fileRecover.size(), 0);

	assert.strictEqual(g4fileRecover.from_tgf_file(tinyGrammarPath), true);
	assert.strictEqual(g4fileRecover.good(), true);
	assert.strictEqual(g4fileRecover.nt_name(g4fileRecover.nt('start')), 'start');

	// ---- parser.from_tgf_string + tree walking (find/find_all/first/second) ----

	const p5 = new tauparser.parser();
	assert.strictEqual(p5.from_tgf_string(tinySrc), true);

	const r5 = p5.parse('123');
	assert.strictEqual(r5.found(), true);
	assert.strictEqual(r5.get_terminals(), '123');

	const g5names = new tauparser.grammar();
	assert.strictEqual(g5names.from_tgf_string(tinySrc), true);
	const numId = g5names.nt('num');
	const startId5 = g5names.nt('start');

	const tree5 = r5.get_bintree();
	assert.strictEqual(tree5.nt(), startId5);
	assert.strictEqual(tree5.text(), '123');

	// "start => num." is a unary rule, so only_child() steps straight to num.
	const onlyChild = tree5.only_child();
	assert.strictEqual(onlyChild.has_value(), true);
	assert.strictEqual(onlyChild.nt(), numId);
	assert.strictEqual(onlyChild.text(), '123');

	// digit+ flattens into one child per matched digit.
	const digits = vec_to_array(onlyChild.children());
	assert.strictEqual(digits.length, 3);
	assert.deepStrictEqual(digits.map(d => d.text()), ['1', '2', '3']);

	// first()/second() are the raw bintree's first and second child, which
	// for this repetition are the first two matched digits as leaves.
	const first5 = onlyChild.first();
	assert.strictEqual(first5.has_value(), true);
	assert.strictEqual(first5.is_terminal(), true);
	assert.strictEqual(first5.text(), '1');

	const second5 = onlyChild.second();
	assert.strictEqual(second5.has_value(), true);
	assert.strictEqual(second5.is_terminal(), true);
	assert.strictEqual(second5.text(), '2');

	// find()/find_all() locate a nonterminal below the current node and
	// step into its only child; demonstrated on a unary "mid" wrapping a
	// terminal-producing "leaf", since num => digit+ is not itself unary.
	const gChain = new tauparser.grammar();
	gChain.add_rule('start', gChain.lit('mid'));
	gChain.add_rule('mid', gChain.lit('leaf'));
	gChain.add_rule('leaf', tauparser.prods.from_char('z'.charCodeAt(0)));
	const leafId = gChain.nt('leaf');
	const midId = gChain.nt('mid');
	assert.strictEqual(gChain.build('start'), true);

	const pChain = new tauparser.parser();
	assert.strictEqual(pChain.init(gChain), true);
	const treeChain = pChain.parse('z').get_bintree();

	const viaFind = treeChain.find(midId);
	assert.strictEqual(viaFind.has_value(), true);
	assert.strictEqual(viaFind.nt(), leafId);
	assert.strictEqual(viaFind.text(), 'z');

	const viaFindAll = vec_to_array(treeChain.find_all(midId));
	assert.strictEqual(viaFindAll.length, 1);
	assert.strictEqual(viaFindAll[0].nt(), leafId);
	assert.strictEqual(viaFindAll[0].text(), 'z');

	// ---- parse_options: property round trip and parse_with_options ----

	const po = new tauparser.parse_options();
	po.measure = true;
	assert.strictEqual(po.measure, true);
	po.debug = false;
	assert.strictEqual(po.debug, false);
	po.tree_path = 'forest';
	assert.strictEqual(po.tree_path, 'forest');
	po.tree_path = 'bintree';
	assert.strictEqual(po.tree_path, 'bintree');
	po.error_verbosity = 'detailed';
	assert.strictEqual(po.error_verbosity, 'detailed');
	po.error_verbosity = 'root-cause';
	assert.strictEqual(po.error_verbosity, 'root-cause');
	po.error_verbosity = 'basic';
	assert.strictEqual(po.error_verbosity, 'basic');
	po.start = numId;
	assert.strictEqual(po.start, numId);

	const rWithOpts = p5.parse_with_options('42', po);
	assert.strictEqual(rWithOpts.found(), true, 'parse with start=num should accept "42"');
	assert.strictEqual(rWithOpts.get_bintree().nt(), numId);

	// ---- shaping_options: property and to_trim set round trip ----

	const so = new tauparser.shaping_options();
	assert.strictEqual(so.trim_terminals, false);
	so.trim_terminals = true;
	assert.strictEqual(so.trim_terminals, true);
	assert.strictEqual(so.inline_char_classes, false);
	so.inline_char_classes = true;
	assert.strictEqual(so.inline_char_classes, true);
	assert.deepStrictEqual(vec_to_array(so.get_to_trim()), []);
	so.add_to_trim(numId);
	assert.deepStrictEqual(vec_to_array(so.get_to_trim()), [numId]);
	so.remove_to_trim(numId);
	assert.deepStrictEqual(vec_to_array(so.get_to_trim()), []);

	// ---- parse_options.shaping: setting it changes get_shaped_bintree() ----

	const shapeSrc = '@use char classes digit.\n\nstart => left right.\nleft   => digit.\nright  => digit.\n';
	const pShape = new tauparser.parser();
	assert.strictEqual(pShape.from_tgf_string(shapeSrc), true);

	const gShapeNames = new tauparser.grammar();
	assert.strictEqual(gShapeNames.from_tgf_string(shapeSrc), true);
	const rightId = gShapeNames.nt('right');

	const rUnshaped = pShape.parse('12');
	assert.strictEqual(rUnshaped.found(), true);
	assert.strictEqual(rUnshaped.get_bintree().text(), '12');
	assert.strictEqual(rUnshaped.get_shaped_bintree().text(), '12',
		'default (no-op) shaping should keep all the original text');

	const poShape = new tauparser.parse_options();
	const soShape = new tauparser.shaping_options();
	soShape.add_to_trim(rightId);
	poShape.shaping = soShape;

	const rShaped = pShape.parse_with_options('12', poShape);
	assert.strictEqual(rShaped.found(), true);
	assert.strictEqual(rShaped.get_bintree().text(), '12',
		'get_bintree() must stay unaffected by shaping');
	assert.strictEqual(rShaped.get_shaped_bintree().text(), '1',
		'get_shaped_bintree() should drop the trimmed "right" subtree');

	// ---- parse_result: diagnostics/report on a grammar with alternation ----

	const p6 = new tauparser.parser();
	assert.strictEqual(p6.from_tgf_string(ambigSrc), true);
	const r6 = p6.parse('1');
	assert.strictEqual(r6.found(), true);
	assert.strictEqual(typeof r6.report_string(), 'string');
	assert.strictEqual(typeof r6.has_diagnostics(), 'boolean');
	// get_shaped_bintree() with the default (no-op) shaping should still
	// contain all the original text.
	assert.strictEqual(r6.get_shaped_bintree().text(), '1');

	// ---- tgf_repl: constructor and reload() on a path that does not resolve ----

	const badRepl = new tauparser.tgf_repl(missingPath);
	assert.strictEqual(badRepl.good(), false);
	assert.strictEqual(badRepl.filename(), missingPath);
	assert.ok(badRepl.diagnostics().length > 0, 'failed load should produce diagnostics');

	// ---- runCommands: same missing-path failure branch ----

	const combinedBad = tauparser.runCommands(missingPath, ['grammar']);
	assert.ok(combinedBad.startsWith('ERROR'),
		`runCommands with a missing grammar should report an error: ${combinedBad}`);

	// ---- FS-backed success paths: write a grammar file into the virtual
	// filesystem, then exercise every file-path binding against it ----

	const fsGrammarPath = '/tiny.tgf';
	tauparser.FS.writeFile(fsGrammarPath, tinySrc);

	const gFile = new tauparser.grammar();
	assert.strictEqual(gFile.from_tgf_file(fsGrammarPath), true);
	assert.strictEqual(gFile.good(), true);
	assert.strictEqual(gFile.nt_name(gFile.nt('start')), 'start');

	const pFile = new tauparser.parser();
	assert.strictEqual(pFile.from_tgf_file(fsGrammarPath), true);
	assert.strictEqual(pFile.good(), true);
	const rFile = pFile.parse('99');
	assert.strictEqual(rFile.found(), true);
	assert.strictEqual(rFile.get_terminals(), '99');

	const loadGood = tauparser.load_grammar(fsGrammarPath);
	assert.ok(loadGood.startsWith('1\n'),
		`load_grammar(real file) should report success: ${loadGood}`);

	const combinedGood = tauparser.runCommands(fsGrammarPath, ['grammar', 'quit']);
	assert.ok(!combinedGood.startsWith('ERROR'),
		`runCommands with a real grammar should not error: ${combinedGood}`);
	assert.ok(combinedGood.includes('start'),
		`runCommands output missing start symbol: ${combinedGood}`);

	// ---- tgf_repl on a real file: good, filename, eval, eval_capture,
	// reload, reprompt ----

	const repl = new tauparser.tgf_repl(fsGrammarPath);
	assert.strictEqual(repl.good(), true,
		`tgf_repl should load a real grammar: ${repl.diagnostics()}`);
	assert.strictEqual(repl.filename(), fsGrammarPath);
	assert.strictEqual(repl.diagnostics(), '');

	const grammarOut = repl.eval_capture('grammar');
	assert.ok(grammarOut.includes('start'),
		`eval_capture('grammar') missing start symbol: ${grammarOut}`);

	const evalRc = repl.eval('grammar');
	assert.strictEqual(evalRc, 0);

	repl.reprompt();

	const otherGrammarPath = '/ambig.tgf';
	tauparser.FS.writeFile(otherGrammarPath, ambigSrc);
	assert.strictEqual(repl.reload(otherGrammarPath), true);
	assert.strictEqual(repl.good(), true);
	assert.strictEqual(repl.filename(), otherGrammarPath);

	const grammarOut2 = repl.eval_capture('grammar');
	assert.ok(grammarOut2.includes('num'),
		`eval_capture('grammar') after reload missing num symbol: ${grammarOut2}`);

	console.log('test_embindings.js: all assertions passed');
}

main().catch(err => {
	console.error(err.stack || err.message || err);
	process.exit(1);
});
