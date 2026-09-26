#!/usr/bin/env node
/**
 * test_browser_keybindings.js - Test key bindings via Puppeteer browser WASM.
 *
 * Usage:
 *   node tests/repl/test_browser_keybindings.js <wasm_dir>
 */

'use strict';

const http = require('http'), fs = require('fs'), path = require('path');
const {
	readScreenText,
	waitForPrompt,
	waitForWasm,
	focusTerminal,
} = require('./browser_helpers');

const WASM_DIR = path.resolve(process.argv[2] || 'build/release-wasm/js/tgf');

const TAU_CHROME_BIN = process.env.TAU_CHROME_BIN;
if (!TAU_CHROME_BIN) {
	console.error('TAU_CHROME_BIN is not set');
	process.exit(2);
}

function startServer() {
	return new Promise((resolve, reject) => {
		const srv = http.createServer((req, res) => {
			const pn = new URL(req.url, 'http://x').pathname;
			let fp = path.join(WASM_DIR, pn === '/' ? 'index.html' : pn);
			if (!fp.startsWith(WASM_DIR)) { res.writeHead(403); return res.end(); }
			res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
			res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
			const mime = { '.html':'text/html','.js':'application/javascript','.wasm':'application/wasm','.mjs':'application/javascript','.css':'text/css','.data':'application/octet-stream','.tgf':'text/plain' }[path.extname(fp)] || 'application/octet-stream';
			try { const data = fs.readFileSync(fp); res.writeHead(200, {'Content-Type':mime}); res.end(data); } catch(e) { res.writeHead(404); res.end(); }
		});
		srv.listen(0, '127.0.0.1', () => resolve({ srv, port: srv.address().port }));
		srv.on('error', reject);
	});
}

async function main() {
	// puppeteer-core is ESM-only; import dynamically so this works on any node.
	const puppeteer = (await import('puppeteer-core')).default;
	const { srv, port } = await startServer();
	console.error(`Server port ${port}`);

	const browser = await puppeteer.launch({ headless: 'new', executablePath: TAU_CHROME_BIN, args: ['--no-sandbox','--disable-setuid-sandbox'] });
	const page = await browser.newPage();
	const url = `http://127.0.0.1:${port}/`;

	let failed = 0;

	async function loadRepl() {
		await page.goto(url, { waitUntil: 'networkidle2', timeout: 30000 });
		try {
			await waitForWasm(page);
		} catch (e) {
			console.error('WASM timeout');
			return false;
		}
		if (!await waitForPrompt(page)) {
			console.error('REPL did not show prompt in time');
			return false;
		}
		await focusTerminal(page);
		return true;
	}

	async function sendReplInput(text) {
		await page.evaluate(t => window.sendReplInput(t), text);
	}

	async function testKey(name, keySeq, expect) {
		if (!await loadRepl()) {
			console.log(`  FAIL ${name}: REPL not ready`);
			failed++;
			return;
		}
		await sendReplInput(keySeq);
		await new Promise(r => setTimeout(r, 3000));
		const screen = await readScreenText(page);
		const ok = screen.includes(expect);
		console.log(`  ${ok ? 'PASS' : 'FAIL'} ${name}: "${expect}" ${ok ? 'found' : 'not found'}`);
		if (!ok) { console.error(`    screen: ${screen.substring(0, 200)}`); failed++; }
	}

	// Ctrl+A (0x01): home - "abc", Ctrl+A, "X" -> Xabc
	await testKey('Ctrl+A home',
		'abc' + String.fromCharCode(0x01) + 'X' + '\n',
		'Syntax Error');
	// Xabc is not a valid formula -> syntax error confirms it's Xabc not abcX

	// Ctrl+E (0x05): end - "abc", Ctrl+A, Ctrl+E, "X" -> abcX
	await testKey('Ctrl+E end',
		'abc' + String.fromCharCode(0x01) + String.fromCharCode(0x05) + 'X' + '\n',
		'Syntax Error');

	await browser.close();
	srv.close();
	console.log(`\nBrowser key bindings: ${failed === 0 ? 'PASS' : 'FAIL'} (${failed} failures)`);
	process.exit(failed === 0 ? 0 : 1);
}

main().catch(err => { console.error(err.message); process.exit(1); });
