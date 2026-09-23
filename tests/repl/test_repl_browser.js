#!/usr/bin/env node
/**
 * test_repl_browser.js - Browser parity: native tgf vs WASM browser REPL.
 *
 * Usage:
 *   node tests/repl/test_repl_browser.js <tgf_bin> <wasm_dir> <grammar_file> [cmd...]
 */

'use strict';

const http = require('http');
const fs   = require('fs');
const path = require('path');
const { execSync } = require('child_process');
const {
	readScreenText,
	waitForPrompt,
	waitForWasm,
	focusTerminal,
} = require('./browser_helpers');

const args = process.argv.slice(2);
if (args.length < 3) {
	console.error('Usage: test_repl_browser.js <tgf_bin> <wasm_dir> <grammar_file> [cmd...]');
	process.exit(2);
}

const TGF_BIN     = args[0];
const WASM_DIR    = path.resolve(args[1]);
const GRAMMAR_SRC = path.resolve(args[2]);
const COMMANDS    = args.length > 3 ? args.slice(3) : ['grammar', 'p x = 0 || y = 1', 'p x = 2', 'quit'];

const TAU_CHROME_BIN = process.env.TAU_CHROME_BIN;
if (!TAU_CHROME_BIN) {
	console.error('TAU_CHROME_BIN is not set');
	process.exit(2);
}

// --- Native tgf (pipe mode) ---
function runNative() {
	const input = COMMANDS.join('\n') + '\n';
	try {
		// tau.tgf takes ~40s to load; allow generous headroom over that.
		return execSync(`"${TGF_BIN}" "${GRAMMAR_SRC}" repl -X 2>&1`, { input, timeout: 120000, encoding: 'utf-8' });
	} catch (e) {
		return e.stdout || '';
	}
}

// --- HTTP server ---
function startServer(rootDir) {
	return new Promise((resolve, reject) => {
		const server = http.createServer((req, res) => {
			const pathname = new URL(req.url, 'http://x').pathname;
			let fp = path.join(rootDir, pathname === '/' ? 'index.html' : pathname);
			if (!fp.startsWith(rootDir)) { res.writeHead(403); return res.end(); }
			res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
			res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
			const mime = {
				'.html':'text/html', '.js':'application/javascript',
				'.wasm':'application/wasm', '.mjs':'application/javascript',
				'.css':'text/css', '.data':'application/octet-stream',
				'.tgf':'text/plain',
			}[path.extname(fp)] || 'application/octet-stream';
			let body;
			try { body = fs.readFileSync(fp); } catch (e) { res.writeHead(404); return res.end(); }
			res.writeHead(200, {'Content-Type': mime});
			res.end(body);
		});
		server.listen(0, '127.0.0.1', () => resolve({ server, port: server.address().port }));
		server.on('error', reject);
	});
}

// --- Browser WASM ---
async function runBrowser(port) {
	// puppeteer-core is ESM-only; import dynamically so this works on any node.
	const puppeteer = (await import('puppeteer-core')).default;
	const browser = await puppeteer.launch({
		headless: 'new',
		executablePath: TAU_CHROME_BIN,
		args: ['--no-sandbox', '--disable-setuid-sandbox'],
	});
	const page = await browser.newPage();

	await page.goto(`http://127.0.0.1:${port}/`, { waitUntil: 'networkidle2', timeout: 30000 });
	try {
		await waitForWasm(page);
	} catch (e) {
		console.error('WASM did not initialize');
		await browser.close();
		return '';
	}
	if (!await waitForPrompt(page)) {
		console.error('REPL did not show prompt in time');
		await browser.close();
		return '';
	}
	await focusTerminal(page);

	for (const cmd of COMMANDS) {
		await page.keyboard.type(cmd);
		await page.keyboard.press('Enter');
		await new Promise(r => setTimeout(r, 2000));
	}
	await new Promise(r => setTimeout(r, 3000));

	const screen = await readScreenText(page);

	await browser.close();
	return screen;
}

// --- Assertions ---
function assertContains(text, pattern, label) {
	if (!text.includes(pattern)) {
		console.error(`FAIL: ${label} - expected "${pattern}" not found`);
		return false;
	}
	console.error(`  ok: ${label}`);
	return true;
}

// --- Main ---
async function main() {
	console.error('Running native tgf...');
	const nativeRaw = runNative();

	const { server, port } = await startServer(WASM_DIR);
	console.error(`Server on port ${port}`);

	console.error('Running browser WASM...');
	const browserRaw = await runBrowser(port);
	server.close();

	// Strip ANSI for string matching
	const native  = nativeRaw.replace(/\x1b\[[0-9;]*m/g, '');
	const browser = browserRaw.replace(/\x1b\[[0-9;]*m/g, '');

	let ok = true;

	// Both must print grammar header
	ok &= assertContains(native, '@use char classes', 'native: grammar header');
	ok &= assertContains(browser, '@use char classes', 'browser: grammar header');

	// Both must parse "x = 0 || y = 1" successfully
	ok &= assertContains(native, 'parsed terminals', 'native: parsed terminals');
	ok &= assertContains(browser, 'parsed terminals', 'browser: parsed terminals');

	// Both must show failure for "x = 2"
	ok &= assertContains(native, 'Syntax Error', 'native: syntax error');
	ok &= assertContains(browser, 'Syntax Error', 'browser: syntax error');

	// Both must quit
	ok &= assertContains(native, 'Quit.', 'native: quit');
	ok &= assertContains(browser, 'Quit.', 'browser: quit');

	if (ok) {
		console.error('PASS: browser parity');
		process.exit(0);
	}
	console.error('--- browser screen ---');
	console.error(browser);
	console.error('--- native output ---');
	console.error(native);
	console.error('FAIL: browser parity');
	process.exit(1);
}

main().catch(err => {
	console.error(err.message || err);
	process.exit(1);
});
