#!/usr/bin/env node
// serve.js - static server for a built terminal page, with the COOP/COEP
// headers SharedArrayBuffer (pthreads) needs.
//
// Usage: node serve.js <dir> [port]

'use strict';

const http = require('http');
const fs = require('fs');
const path = require('path');

const args = process.argv.slice(2);
const dir = path.resolve(args[0] || '.');
let port = 8088;
for (const arg of args.slice(1)) {
  if (/^[0-9]+$/.test(arg)) port = parseInt(arg, 10);
}

const mime_types = {
  '.html': 'text/html', '.js': 'application/javascript',
  '.wasm': 'application/wasm', '.mjs': 'application/javascript',
  '.css': 'text/css', '.data': 'application/octet-stream',
  '.tgf': 'text/plain', '.tau': 'text/plain',
};

http.createServer((req, res) => {
  const pathname = new URL(req.url, 'http://x').pathname;
  const file_path = path.join(dir, pathname === '/' ? 'index.html' : pathname);
  // path.relative, not startsWith: a sibling directory sharing the root's
  // prefix (e.g. /a/page vs /a/page2) must not satisfy a prefix test.
  const rel = path.relative(dir, file_path);
  if (rel.startsWith('..') || path.isAbsolute(rel)) {
    res.writeHead(403); return res.end();
  }
  res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
  res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
  const mime = mime_types[path.extname(file_path)] || 'application/octet-stream';
  let body;
  try { body = fs.readFileSync(file_path); } catch (e) { res.writeHead(404); return res.end(); }
  res.writeHead(200, { 'Content-Type': mime });
  res.end(body);
}).listen(port, '0.0.0.0', () => console.log(`Serving ${dir} on http://localhost:${port}/`));
