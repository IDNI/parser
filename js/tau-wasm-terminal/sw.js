// sw.js - Service Worker that injects COOP/COEP headers for SharedArrayBuffer.
// Required by FTXUI's WASM path (-s PROXY_TO_PTHREAD).
self.addEventListener('install', () => self.skipWaiting());
self.addEventListener('activate', e => e.waitUntil(self.clients.claim()));
self.addEventListener('fetch', e => {
  if (e.request.mode != 'navigate' &&
    !e.request.url.includes('.worker.js')) {
    return;
  }

  e.respondWith((async () => {
    const response = await fetch(e.request);

    const newHeaders = new Headers(response.headers);
    newHeaders.set('Cross-Origin-Embedder-Policy', 'require-corp');
    newHeaders.set('Cross-Origin-Opener-Policy', 'same-origin');

    const moddedResponse = new Response(response.body, {
      status: response.status,
      statusText: response.statusText,
      headers: newHeaders,
    });

    return moddedResponse;
  })());
});
