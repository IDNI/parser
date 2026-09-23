#!/usr/bin/env node
/**
 * test_syntax_highlighter.mjs - Test TGF syntax highlighter WASM module.
 *
 * Usage:
 *   node tests/syntax_highlighter/test_syntax_highlighter.mjs <extension_root>
 *
 * <extension_root> is editors/vscode (or a copy of it). It must contain
 * wasm/syntax_highlighter.js, grammars/<langId>.tgf, grammars.json and
 * package.json.
 */

'use strict';

import { createRequire } from 'module';
import { readFileSync, existsSync } from 'fs';
import { resolve } from 'path';
const require = createRequire(import.meta.url);

const args = process.argv.slice(2);
if (args.length < 1) {
	console.error('Usage: test_syntax_highlighter.mjs <extension_root>');
	process.exit(2);
}

const EXT_ROOT  = resolve(args[0]);
const WASM_PATH = EXT_ROOT + '/wasm/syntax_highlighter.js';
const GRAMMARS_JSON = EXT_ROOT + '/grammars.json';
const PACKAGE_JSON  = EXT_ROOT + '/package.json';

if (!existsSync(WASM_PATH)) {
	console.error('WASM not found:', WASM_PATH);
	process.exit(1);
}
if (!existsSync(GRAMMARS_JSON)) {
	console.error('grammars.json not found:', GRAMMARS_JSON);
	process.exit(1);
}

const grammars = JSON.parse(readFileSync(GRAMMARS_JSON, 'utf8'));
const pkg = JSON.parse(readFileSync(PACKAGE_JSON, 'utf8'));

const HL = await require(WASM_PATH)();

let passed = 0, failed = 0;
function check(desc, cond) {
	if (cond) { console.log(`  PASS: ${desc}`); passed++; }
	else      { console.log(`  FAIL: ${desc}`); failed++; }
}

// --- test: every key of grammars.json is a contributed language ---
console.log('--- package.json contributes every grammars.json language ---');
const contributedIds = new Set(
	(pkg.contributes && pkg.contributes.languages || []).map(l => l.id));
for (const langId of Object.keys(grammars))
	check(`${langId} contributed in package.json`, contributedIds.has(langId));

// --- test: bundled grammar file exists for every grammars.json language ---
console.log('\n--- bundled grammar files ---');
for (const langId of Object.keys(grammars))
	check(`grammars/${langId}.tgf exists`,
		existsSync(resolve(EXT_ROOT, 'grammars', `${langId}.tgf`)));

// --- test: grammar compilation (skip grammars whose file doesn't exist) ---
console.log('\n--- grammar compilation ---');
const compiled = {}; // langId -> { hl, path }
for (const langId of Object.keys(grammars)) {
	const grammarPath = resolve(EXT_ROOT, 'grammars', `${langId}.tgf`);
	if (!existsSync(grammarPath)) {
		console.log(`  SKIP: ${langId} (${grammarPath} does not exist)`);
		continue;
	}
	const src = readFileSync(grammarPath, 'utf8');
	const hl = new HL.syntax_highlighter(src);
	if (hl.good()) {
		console.log(`  PASS: ${langId} (${grammarPath}, ${src.length} bytes)`);
		passed++;
		compiled[langId] = { hl, path: grammarPath };
	} else {
		console.log(`  FAIL: ${langId} - ${hl.diagnostics()}`);
		failed++;
		hl.delete();
	}
}

// --- test: tokenization, one sample per compiled grammar ---
console.log('\n--- tokenization ---');
const samples = {
	'tgf':          'start => expr.\nexpr => digit+.\n',
	'tgf-test':     '# Tests the csv.tgf grammar with the header, comma and crlf productions.\n\nstart : "a,b\\r\\n1,2", "\\"a\\",b\\r\\n1,2".\n\nstart > file > ^ header record $ : "a,b\\r\\n1,2".\n',
	'tgf-csv':      'name,age\nAlice,30\n',
	'tgf-json':     '{"key": [1, 2]}',
	'tgf-treemr':   null,
	'tgf-tgf_repl': 'parse "hi". set tt on. get s',
};
for (const [langId, { hl }] of Object.entries(compiled)) {
	const input = samples[langId];
	if (input == null) {
		console.log(`  SKIP: ${langId} (no sample defined)`);
		continue;
	}
	const raw = hl.get_tokens(input);
	const ct = raw.size();
	raw.delete();
	check(`${langId} -> ${ct / 5} tokens`, ct > 0);
}

// --- test: nonterminal classification agrees with tgf.tgf's @highlight block ---
// checks each nonterminal's reported highlight type against tgf.tgf's own @highlight block
console.log('\n--- nt classification (tgf.tgf) ---');
if (compiled['tgf']) {
	const hl = compiled['tgf'].hl;
	const ntCount = hl.nt_count();
	const checks = [
		['dir_sym', 'keyword'],
		['sym', 'variable'],
		['comment', 'comment'],
		['terminal_string', 'string'],
		['terminal_char', 'string'],
		['escaped_s', 'number'],
		['escaped_c', 'number'],
		['terminal_hex', 'number'],
		['alternation', 'operator'],
		['directive_token', 'type'],
	];
	for (const [name, expect] of checks) {
		let id = -1;
		for (let i = 0; i < ntCount; i++) {
			if (hl.get_nt_name(i) === name) { id = i; break; }
		}
		const type = id === -1 ? '(not found)' : hl.get_nt_type(id);
		check(`nt "${name}" -> ${expect} (got ${type})`, type === expect);
	}
} else {
	console.log('  SKIP: tgf.tgf did not compile');
}

// --- test: nonterminal classification agrees with tgf.test.tgf's @highlight block ---
console.log('\n--- nt classification (tgf.test.tgf) ---');
if (compiled['tgf-test']) {
	const hl = compiled['tgf-test'].hl;
	const ntCount = hl.nt_count();
	const checks = [
		['comment', 'comment'],
		['entry_terminator', 'delimiter'],
	];
	for (const [name, expect] of checks) {
		let id = -1;
		for (let i = 0; i < ntCount; i++) {
			if (hl.get_nt_name(i) === name) { id = i; break; }
		}
		const type = id === -1 ? '(not found)' : hl.get_nt_type(id);
		check(`nt "${name}" -> ${expect} (got ${type})`, type === expect);
	}
} else {
	console.log('  SKIP: tgf.test.tgf did not compile');
}

for (const { hl } of Object.values(compiled)) hl.delete();

// --- result ---
console.log(`\n${passed} passed, ${failed} failed`);
process.exit(failed > 0 ? 1 : 0);
