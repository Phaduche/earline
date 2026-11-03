// Earline service worker: shows alerts and brings the app back when one is tapped
self.addEventListener('install', () => self.skipWaiting());
self.addEventListener('activate', e => e.waitUntil(self.clients.claim()));

self.addEventListener('notificationclick', e => {
  e.notification.close();
  e.waitUntil((async () => {
    const all = await self.clients.matchAll({ type: 'window', includeUncontrolled: true });
    const app = all.find(c => c.url.includes(self.registration.scope));
    if (app) return app.focus();
    return self.clients.openWindow('./');
  })());
});

// Placeholder for server push later (e.g. laptop -> server -> phone)
self.addEventListener('push', e => {
  const d = e.data ? e.data.json() : { title: 'Turtle neck detected', body: 'Check your posture.' };
  e.waitUntil(self.registration.showNotification(d.title, { body: d.body, icon: 'icon-192.png', tag: 'earline-posture', renotify: true }));
});
