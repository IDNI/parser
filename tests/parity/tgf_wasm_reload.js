#!/usr/bin/env node
/**
 * tgf_wasm_reload.js - Check tgf_repl.reload(file): load a first grammar,
 * reload a second grammar file, then parse with the second.
 *
 * Usage:
 *   node tgf_wasm_reload.js <grammar_a> <grammar_b>
 *
 * grammar_a must reject the input the check parses; grammar_b must accept
 * it, so the output after the reload proves the second grammar is in use.
 */
'use strict';

const path = require('path');

function fail(message) {
	console.error('tgf_wasm_reload: ' + message);
	process.exit(1);
}

async function main() {
	if (process.argv.length < 4) {
		console.error('usage: tgf_wasm_reload.js <grammar_a> <grammar_b>');
		process.exit(2);
	}
	const grammarA = path.resolve(process.argv[2]);
	const grammarB = path.resolve(process.argv[3]);

	const buildDir = path.resolve(__dirname, '..', '..', 'build', 'emscripten');
	process.chdir(buildDir);
	const createTGF = require(path.join(buildDir, 'tgf_node.js'));
	const TGF = await createTGF();

	TGF.FS.mkdir('/host');
	TGF.FS.mount(TGF.NODEFS, { root: '/' }, '/host');

	const repl = new TGF.tgf_repl('/host' + grammarA);
	if (!repl.good())
		fail('cannot load ' + grammarA + ': ' + repl.diagnostics());

	// grammar A cannot parse a JSON object, so no JSON symbol shows up
	const before = repl.eval_capture('p {"a":1}');
	if (before.includes('object_pair'))
		fail('grammar A parsed a JSON object: ' + JSON.stringify(before));

	if (repl.reload('/host' + grammarB) !== true)
		fail('reload(' + grammarB + ') returned false');
	if (!repl.good())
		fail('not good after reload: ' + repl.diagnostics());
	if (repl.filename() !== '/host' + grammarB)
		fail('filename after reload is ' + repl.filename());

	// grammar B parses the same input, so its own symbols appear
	const after = repl.eval_capture('p {"a":1}');
	if (!after.includes('object_pair'))
		fail('grammar B did not parse after reload: '
			+ JSON.stringify(after));

	console.log('tgf_wasm_reload: OK');
}

main().catch((err) => {
	console.error('tgf_wasm_reload: ' + (err.stack || err.message || err));
	process.exit(1);
});
