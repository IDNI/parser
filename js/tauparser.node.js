#!/usr/bin/env node

const tauparserModule = require('./tauparser.js');

if (typeof tauparserModule !== 'function') {
	console.error('Expected a function, got:', typeof tauparserModule);
} else {
	const tauparserFactory = tauparserModule;
	tauparserFactory().then(tauparser => {
		console.log('tauparserjs loaded');
		const version = tauparser.version;
		console.log('version: ', version());
		console.log('test: ', tauparser.test());
	});
}