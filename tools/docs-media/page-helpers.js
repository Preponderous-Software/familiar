// Page helpers for the docs captures (tools/docs-media/media.json), loaded by
// darkroom as an init script into every page it opens. They only use the hooks
// the smoke test uses (window.familiar, Module.FS) and the save format in
// src/save/save.cpp; nothing here is part of the game.
//
//   __docs.st()               the game's status (familiar.status(), parsed)
//   __docs.ready()            true once the game is up
//   __docs.saveAt('7:40 pm')  saves the night as it is, moves the save's clock
//                             to that time (re-signing it with SHA-256, as the
//                             game does), stops the page saving over it, and
//                             leaves it for a `reload` step to load
//   __docs.loadSave(base64)   puts a save (a tests/fixtures file) in the slot
//                             for a `reload` step to load
//   __docs.damage()           saves twice, then flips one byte of the latest
//                             save, as the smoke test does: a reload refuses it
//                             and loads the one before
//   __docs.pose(yaw, tilt)    turns the view to an absolute yaw and tilts it
//                             up by tilt (radians; the hooks level the view)
window.__docs = (() => {
  const SLOT = '/familiar-save/default';
  const HEADER = 8 + 4 + 8;          // magic, version, payload length
  const TICK_AT = HEADER + 8;        // World::seed (u64), then World::tick (i64)
  const TICKS_PER_MINUTE = 30 * 3;   // 30 ticks a second, a game minute every 3 s
  const START = 17 * 60 + 20;        // the night starts at 5:20 pm
  const minuteOf = (text) => {
    const m = /^(\d+):(\d\d)\s*(am|pm)$/.exec(text.trim());
    if (!m) throw new Error('bad time ' + text);
    let h = Number(m[1]) % 12;
    if (m[3] === 'pm') h += 12;
    else if (h < 12) h += 24;  // after midnight
    return h * 60 + Number(m[2]);
  };
  const sync = () => new Promise((res, rej) => Module.FS.syncfs(false, (e) => (e ? rej(e) : res())));
  return {
    st: () => JSON.parse(window.familiar.status()),
    ready: () => !!(window.familiar && window.familiarStatus),
    async saveAt(time) {
      if (!window.familiar.save()) throw new Error('the game did not save');
      const bytes = Module.FS.readFile(SLOT + '/familiar.save');
      const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
      view.setBigInt64(TICK_AT, BigInt((minuteOf(time) - START) * TICKS_PER_MINUTE), true);
      const body = bytes.subarray(0, bytes.length - 32);
      bytes.set(new Uint8Array(await crypto.subtle.digest('SHA-256', body)), bytes.length - 32);
      window.familiar.save = () => 0;  // the page saves as it unloads; keep this one
      try { Module.FS.unlink(SLOT + '/familiar.save.prev'); } catch (e) {}
      Module.FS.writeFile(SLOT + '/familiar.save', bytes);
      await sync();
    },
    async loadSave(b64) {
      window.familiar.save = () => 0;
      try { Module.FS.unlink(SLOT + '/familiar.save.prev'); } catch (e) {}
      Module.FS.writeFile(SLOT + '/familiar.save', Uint8Array.from(atob(b64), (c) => c.charCodeAt(0)));
      await sync();
    },
    async damage() {
      if (!window.familiar.save() || !window.familiar.save()) throw new Error('the game did not save');
      window.familiar.save = () => 0;
      const bytes = Module.FS.readFile(SLOT + '/familiar.save');
      bytes[Math.floor(bytes.length / 2)] ^= 0x01;
      Module.FS.writeFile(SLOT + '/familiar.save', bytes);
      await sync();
    },
    pose(yaw, pitch) {
      const s = JSON.parse(window.familiar.status());
      // familiar.look takes mouse pixels; the session turns 0.0025 rad a pixel.
      window.familiar.look((yaw - s.yaw) / 0.0025, -(pitch || 0) / 0.0025);
    },
  };
})();
