// Earline service worker: shows alerts as system notifications and handles taps on them
self.addEventListener('install', () => self.skipWaiting());
self.addEventListener('activate', e => e.waitUntil(self.clients.claim()));

self.addEventListener('notificationclick', e => {
  e.notification.close();
  e.waitUntil((async () => {
    const all = await self.clients.matchAll({ type: 'window', includeUncontrolled: true });
    const app = all.find(c => c.url.startsWith(self.registration.scope));
    // "Snooze 15 min" button: tell the open page to pause alerts, keep the user where they are
    if (e.action === 'snooze') {
      all.forEach(c => c.postMessage({ type: 'snooze', minutes: 15 }));
      return;
    }
    if (app) return app.focus();
    return self.clients.openWindow('./');
  })());
});

// Placeholder for server push later (e.g. laptop -> server -> phone)
self.addEventListener('push', e => {
  const d = e.data ? e.data.json() : { title: 'Turtle neck', body: 'Check your posture.' };
  e.waitUntil(self.registration.showNotification(d.title, { body: d.body, icon: 'icon-192.png', badge: 'icon-192.png', tag: 'earline', renotify: true }));
});
