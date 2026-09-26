#!/usr/bin/env node
/**
 * tgf_wasm_parity.js - Run TGF commands through the tgf_node WASM module
 * and emit captured output to stdout.
 *
 * Usage:
 *   node tests/tgf_wasm_parity.js <grammar_file> <cmd1> <cmd2> ...
 *
 * Output: one block per command, separated by a marker line.
 */
'use strict';

const path = require('path');

async function main() {
	const args = process.argv.slice(2);
	if (args.length < 2) {
		console.error('Usage: tgf_wasm_parity.js <grammar_file> <cmd>...');
		process.exit(2);
	}

	const grammarFile = args[0];
	const commands    = args.slice(1);

	const absGrammar = path.resolve(grammarFile);

	const buildDir = path.resolve(__dirname, '..', '..', 'build', 'release-wasm');
	process.chdir(buildDir);
	const tgfNodePath = path.join(buildDir, 'tgf_node.js');
	const createTGF = require(tgfNodePath);
	const TGF = await createTGF();

	TGF.FS.mkdir('/host');
	TGF.FS.mount(TGF.NODEFS, { root: '/' }, '/host');

	const vfsPath = '/host' + absGrammar;
	try {
		TGF.FS.stat(vfsPath);
	} catch(e) {
		console.error(`Grammar file not accessible: ${grammarFile}`);
		console.error(`(vfs path: ${vfsPath})`);
		process.exit(1);
	}

	const node = new TGF.tgf_repl(vfsPath);
	if (!node.good()) {
		console.error('tgf_repl failed to load grammar:');
		console.error(node.diagnostics());
		process.exit(1);
	}

	for (const cmd of commands) {
		// Rewrite `load "..."` paths to VFS /host/... prefix
		let vfsCmd = cmd;
		let vfsPrefix = '';
		const m = cmd.match(/^(l|load)\s+"([^"]+)"/);
		if (m) {
			const filePath = m[2];
			if (!filePath.startsWith('/host/')) {
				const abs = path.resolve(filePath);
				vfsPrefix = '/host';
				vfsCmd = `load "${vfsPrefix}${abs}"`;
			}
		}
		let out = node.eval_capture(vfsCmd);
		// Strip /host prefix from output for parity with native
		if (vfsPrefix) out = out.replaceAll(vfsPrefix, '');
		process.stdout.write(`=== ${cmd} ===\n`);
		process.stdout.write(out);
		if (!out.endsWith('\n')) process.stdout.write('\n');
	}
}

main().catch(err => {
	console.error(err.message || err);
	process.exit(1);
});
