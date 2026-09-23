'use strict';

const LOAD_TIMEOUT_MS = 180000;
const POLL_INTERVAL_MS = 500;

function getScreenText() {
	const term = window.term;
	if (!term) return '';
	const buf = term.buffer.active;
	const lines = [];
	for (let i = 0; i < buf.length; i++) {
		const l = buf.getLine(i);
		if (l) lines.push(l.translateToString().trimEnd());
	}
	return lines.filter(l => l.length > 0);
}

async function readScreen(page) {
	return page.evaluate(getScreenText);
}

async function readScreenText(page) {
	const lines = await readScreen(page);
	return lines.join('\n');
}

async function waitForPrompt(page, timeoutMs = LOAD_TIMEOUT_MS) {
	const deadline = Date.now() + timeoutMs;
	while (Date.now() < deadline) {
		const lines = await readScreen(page);
		const last = lines.length ? lines[lines.length - 1] : '';
		if (last.includes('tgf>')) return true;
		await new Promise(r => setTimeout(r, POLL_INTERVAL_MS));
	}
	return false;
}

async function waitForWasm(page) {
	await page.waitForFunction(
		() => typeof Module !== 'undefined' && typeof FS !== 'undefined',
		{ timeout: 30000 });
}

async function focusTerminal(page) {
	await page.evaluate(() => window.term && window.term.focus());
}

module.exports = {
	LOAD_TIMEOUT_MS,
	getScreenText,
	readScreen,
	readScreenText,
	waitForPrompt,
	waitForWasm,
	focusTerminal,
};
