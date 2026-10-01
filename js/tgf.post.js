// Run as a program (node tgf.js <args>, or the npm bin). A require() only
// gets the TGF factory and runs nothing.
if (typeof module !== 'undefined' && typeof require !== 'undefined'
		&& require.main === module) {
	const fs = require('fs');
	const path = require('path');

	// The wasm file system has its own /dev and /proc, so those stay. The host
	// /tmp takes the place of the wasm /tmp, so a grammar or a working
	// directory under /tmp is reachable.
	const keep = new Set(['dev', 'proc']);

	const mountHost = (FS, NODEFS) => {
		if (process.platform === 'win32') {
			// A drive letter has no place in a wasm path, so only the working
			// directory is reachable, at /cwd.
			FS.mkdirTree('/cwd');
			FS.mount(NODEFS, { root: process.cwd() }, '/cwd');
			FS.chdir('/cwd');
			return;
		}
		for (const name of fs.readdirSync('/')) {
			if (keep.has(name)) continue;
			const host = path.join('/', name);
			let stat;
			try { stat = fs.statSync(host); } catch { continue; }
			if (!stat.isDirectory()) continue;
			FS.mkdirTree(host);
			FS.mount(NODEFS, { root: host }, host);
		}
		FS.chdir(process.cwd());
	};

	TGF().then((tgf) => {
		mountHost(tgf.FS, tgf.NODEFS);
		let code;
		try {
			code = tgf.callMain(process.argv.slice(2));
		} catch (e) {
			// exit() in the program arrives as an ExitStatus, not as a value.
			if (e && e.name === 'ExitStatus') code = e.status;
			else throw e;
		}
		if (typeof code === 'number') process.exitCode = code;
	}).catch((err) => {
		console.error(err && err.message ? err.message : err);
		process.exit(1);
	});
}
