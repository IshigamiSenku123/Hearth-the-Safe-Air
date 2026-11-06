importScripts('https://www.gstatic.com/firebasejs/10.12.0/firebase-app-compat.js');
importScripts('https://www.gstatic.com/firebasejs/10.12.0/firebase-messaging-compat.js');

firebase.initializeApp({
  apiKey: "YOUR_FIREBASE_API_KEY",
  authDomain: "PASTE_YOUR_AUTH_DOMAIN",
  databaseURL: "YOUR_DATABASE_URL",
  projectId: "PASTE_YOUR_PROJECT_ID",
  storageBucket: "PASTE_YOUR_STORAGE_BUCKET",
  messagingSenderId: "PASTE_YOUR_SENDER_ID",
  appId: "PASTE_YOUR_APP_ID"
});

const messaging = firebase.messaging();

messaging.onBackgroundMessage((payload) => {
  const n = payload.notification || {};
  self.registration.showNotification(n.title || '🚨 SafeAir Alert', {
    body: n.body || 'Hazard detected on your device',
    icon: './icon-192.png',
    badge: './icon-192.png',
    tag: 'safeair-bg',
    requireInteraction: true,
    vibrate: [300, 100, 300],
    actions: [
      { action: 'open', title: 'Open App' },
      { action: 'mute', title: 'Mute' }
    ],
    data: payload.data || {}
  });
});

self.addEventListener('notificationclick', (event) => {
  event.notification.close();
  if (event.action === 'mute') {
    event.waitUntil(Promise.resolve());
    return;
  }
  event.waitUntil(clients.openWindow('./safeair-app.html'));
});
