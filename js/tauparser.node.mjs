#!/usr/bin/env node

// ES module smoke test for tauparser.esm.mjs. The same checks as
// tauparser.node.js, loaded with import instead of require.

import tauparserModule from './tauparser.esm.mjs';

const grammar = '@use char classes digit.\n\nstart => num.\nnum   => digit+.\n';

function check(cond, label) {
	if (!cond) {
		console.error('FAIL ' + label);
		process.exit(1);
	}
	console.log('OK   ' + label);
}

check(typeof tauparserModule === 'function', 'the module is a factory');
const tauparser = await tauparserModule();
const version = tauparser.version();
check(version.startsWith('tauparser-js '), 'version: ' + version);

const parser = new tauparser.parser();
check(parser.from_tgf_string(grammar), 'the grammar loads');
const ok = parser.parse('42');
check(ok.found() && ok.get_bintree().text() === '42', 'parse("42") is found');
check(!parser.parse('4x').found(), 'parse("4x") is not found');
console.log('SMOKE TEST PASSED');
