// Smoke test for the browser build in headless Chromium (Playwright).
//
//   node web/smoke/smoke.cjs <url> <out-dir> [--phone]
//
// Checks: the page starts with no page errors and shows How to play on a first
// visit; the canvas draws the town (not one flat colour); walking moves the
// player (W on a desktop, the stick on the phone profile) and looking turns
// them; Q (or the Cat button) turns Wren into a cat with a lower eye, and back.
// On a desktop the player also stands at the Okafors' door and knocks, and the
// candy count goes up after a joke. A costume is chosen first (3, skeleton).
// The cat (M3), on both profiles: the candy bag drops where Wren changes; at
// Mr. Hale's latch, inside his yard, E (or the action button, which then says
// Open) opens the gate; in the churchyard the cat sees the spirits, and Wren
// does not; back on the doorstep, Wren picks the bag up. Places are reached
// with the familiar.toSpot / toDoorstep page hooks, not by steering.
// Witchcraft (M4): on a desktop, E lights Grandpa Ike's lantern once the
// Okafors have been knocked at, Tab opens Wren's notebook, and candy buys a
// cinnamon stick at Abbas's Market; on the phone, the big button reads Light
// at Nana's window and lights her lantern, and the Notebook button opens the
// notebook. On the phone no two controls overlap, and all are on screen.
// The familiar.toPlace hook stands Wren at one of the night's places
// (sim/craft.h); every action is then done with the real keys and buttons.
// Then saves: a save survives a reload (IndexedDB) with the same place, time,
// costume, candy and bag and a "Loaded your save" message; and with the latest
// save damaged on purpose (one byte flipped), a reload refuses it, says so, and
// loads the save before it. Writes PNGs to <out-dir>. (Atman's smoke test, for
// Familiar.)
const fs = require('fs');
const path = require('path');
const zlib = require('zlib');
const { chromium, devices } = require('playwright');

const [url, outDir] = process.argv.slice(2);
const phone = process.argv.includes('--phone');
if (!url || !outDir) {
  console.error('usage: smoke.cjs <url> <out-dir> [--phone]');
  process.exit(2);
}
fs.mkdirSync(outDir, { recursive: true });

// Minimal PNG decoder (8-bit RGB/RGBA, non-interlaced: what Chromium writes).
function decodePng(buf) {
  let pos = 8, width = 0, height = 0, channels = 0;
  const idat = [];
  while (pos < buf.length) {
    const len = buf.readUInt32BE(pos);
    const type = buf.toString('ascii', pos + 4, pos + 8);
    const data = buf.subarray(pos + 8, pos + 8 + len);
    if (type === 'IHDR') {
      width = data.readUInt32BE(0);
      height = data.readUInt32BE(4);
      channels = { 2: 3, 6: 4 }[data[9]];
      if (data[8] !== 8 || !channels || data[12] !== 0) throw new Error('unsupported PNG');
    } else if (type === 'IDAT') idat.push(data);
    pos += 12 + len;
  }
  const raw = zlib.inflateSync(Buffer.concat(idat));
  const stride = width * channels;
  const out = Buffer.alloc(height * stride);
  for (let y = 0; y < height; y++) {
    const f = raw[y * (stride + 1)];
    for (let x = 0; x < stride; x++) {
      const v = raw[y * (stride + 1) + 1 + x];
      const a = x >= channels ? out[y * stride + x - channels] : 0;
      const b = y > 0 ? out[(y - 1) * stride + x] : 0;
      const c = x >= channels && y > 0 ? out[(y - 1) * stride + x - channels] : 0;
      let p;
      if (f === 0) p = 0;
      else if (f === 1) p = a;
      else if (f === 2) p = b;
      else if (f === 3) p = (a + b) >> 1;
      else {
        const pa = Math.abs(b - c), pb = Math.abs(a - c), pc = Math.abs(a + b - 2 * c);
        p = pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
      }
      out[y * stride + x] = (v + p) & 255;
    }
  }
  return { width, height, channels, data: out };
}

// Distinct colours (quantised to 4 bits per channel) in a PNG.
function colourCount(png) {
  const img = decodePng(png);
  const seen = new Set();
  for (let i = 0; i < img.data.length; i += img.channels)
    seen.add(((img.data[i] >> 4) << 8) | ((img.data[i + 1] >> 4) << 4) | (img.data[i + 2] >> 4));
  return seen.size;
}

(async () => {
  const browser = await chromium.launch({ args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
  const base = phone ? devices['iPhone 13'] : { viewport: { width: 1280, height: 800 } };
  // "headless" in the UA keeps arcade from counting a smoke run as a play.
  const context = await browser.newContext({ ...base, userAgent: (base.userAgent || 'Mozilla/5.0') + ' familiar-smoke headless' });
  const page = await context.newPage();
  const errors = [];
  page.on('pageerror', (e) => errors.push(String(e)));
  page.on('console', (m) => { if (m.type() === 'error') errors.push('console: ' + m.text()); });
  const fail = async (why) => {
    console.error('FAIL: ' + why);
    await page.screenshot({ path: path.join(outDir, 'failure.png') }).catch(() => {});
    await browser.close();
    process.exit(1);
  };
  // Opens the page (again, for a reload) and waits for the game.
  const open = async (reload) => {
    if (reload) await page.reload({ waitUntil: 'commit' });
    else await page.goto(url, { waitUntil: 'commit' });
    try {
      await page.waitForFunction(() => window.familiar && window.familiarStatus, null, { timeout: 90000 });
    } catch (e) {
      await fail('the game never became ready' + (reload ? ' after a reload' : '') + ': ' + errors.join(' | '));
    }
  };
  await open(false);
  const status = async () => JSON.parse(await page.evaluate(() => window.familiar.status()));
  const until = async (pred, ms = 20000) => {
    const end = Date.now() + ms;
    let st = await status();
    while (!pred(st) && Date.now() < end) {
      await page.waitForTimeout(200);
      st = await status();
    }
    return st;
  };

  // On a phone, every visible control is on screen and none overlaps another.
  const checkLayout = async (when) => {
    if (!phone) return;
    const boxes = await page.evaluate(() => {
      const out = [];
      document.querySelectorAll('#top button, #pad button, #costumes button').forEach((b) => {
        const r = b.getBoundingClientRect();
        if (r.width > 0 && r.height > 0 && getComputedStyle(b).display !== 'none' && b.offsetParent)
          out.push({ id: b.id || b.textContent, x: r.left, y: r.top, w: r.width, h: r.height });
      });
      return { out, vw: innerWidth, vh: innerHeight };
    });
    for (const a of boxes.out) {
      if (a.x < 0 || a.y < 0 || a.x + a.w > boxes.vw + 0.5 || a.y + a.h > boxes.vh + 0.5) await fail(`${when}: ${a.id} is off screen`);
      for (const b of boxes.out) {
        if (a === b) continue;
        if (a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h) await fail(`${when}: ${a.id} overlaps ${b.id}`);
      }
    }
    console.log(`ok: ${boxes.out.length} controls on screen, none overlapping (${when})`);
  };

  // As the cat: open Mr. Hale's latch from inside his yard with `use` (E, or
  // a tap on the action button), then go among the stones and see the spirits.
  const catChecks = async (use) => {
    await page.evaluate(() => window.familiar.toSpot(0));
    s = await until((st) => /latch/.test(st.prompt) && st.use === 'Open');
    console.log(`in Mr. Hale's yard: (${s.x}, ${s.z}), prompt ${s.prompt}, button ${s.use}`);
    if (!/latch/.test(s.prompt) || s.use !== 'Open') await fail('inside Mr. Hale\'s yard the cat was offered no latch: ' + s.prompt + ' / ' + s.use);
    // The page relabels its buttons from the status every 250 ms.
    if (phone && !(await page.waitForFunction(() => document.getElementById('use').textContent.trim() === 'Open', null, { timeout: 10000 }).catch(() => false)))
      await fail('the action button does not say Open at the latch: ' + (await page.locator('#use').innerText()));
    await use();
    s = await until((st) => st.haleGate);
    if (!s.haleGate) await fail('the cat did not open Mr. Hale\'s gate');
    await page.evaluate(() => window.familiar.toSpot(2));
    s = await until((st) => st.spiritsSeen > 0);
    console.log(`in the churchyard: ${s.spiritsShown} spirits shown, ${s.spiritsSeen} met; ${s.message}`);
    if (s.spiritsShown !== 4 || s.spiritsSeen < 1) await fail('the cat did not see the spirits in the churchyard');
    if (!/nods to you|smiles at you|tips his cap|taps his foot/.test(s.message)) await fail('meeting a spirit said nothing: ' + s.message);
    await page.waitForTimeout(1200);
    await page.screenshot({ path: path.join(outDir, 'familiar-spirits.png'), timeout: 180000 });
  };

  if (!(await page.locator('#help').isVisible())) await fail('How to play was not shown on a first visit');
  await page.locator('#helpClose').click();
  let s = await status();
  console.log(`ready: ${s.clock} at (${s.x}, ${s.z})`);
  if (s.clock !== '5:20 pm') await fail('the night should start at 5:20 pm, not ' + s.clock);
  if (!s.choosing) await fail('a new night should offer a costume');
  await page.locator(phone ? '#costumes [data-costume="3"]' : 'body').click({ timeout: 5000 }).catch(() => {});
  if (!phone) await page.keyboard.press('Digit3');
  s = await until((st) => st.costume === 3);
  if (s.costume !== 3) await fail('choosing the skeleton costume did not take');
  await page.waitForTimeout(1500);
  const shot = await page.screenshot({ path: path.join(outDir, 'familiar.png'), timeout: 180000 });
  const colours = colourCount(shot);
  console.log(`screenshot: ${colours} distinct colours`);
  if (colours < 12) await fail('the canvas looks blank (' + colours + ' colours)');

  const start = { x: s.x, z: s.z, yaw: s.yaw };
  const box = await page.locator('#canvas').boundingBox();
  if (phone) {
    await checkLayout('choosing a costume');
    if (!(await page.locator('#panel').isVisible())) await fail('the HUD panel is not shown on a narrow page');
    const panel = await page.locator('#panel').innerText();
    if (!/Candy/.test(panel)) await fail('the HUD panel does not show the candy: ' + panel);
    const cdp = await context.newCDPSession(page);
    const sx = box.x + box.width * 0.2, sy = box.y + box.height * 0.6;
    const touch = (type, pts) => cdp.send('Input.dispatchTouchEvent', { type, touchPoints: pts });
    await touch('touchStart', [{ x: sx, y: sy, id: 1 }]);
    for (let i = 1; i <= 6; i++) await touch('touchMove', [{ x: sx, y: sy - 10 * i, id: 1 }]);
    s = await until((st) => Math.hypot(st.x - start.x, st.z - start.z) > 0.3);
    await touch('touchEnd', []);
    console.log(`after the stick: at (${s.x}, ${s.z})`);
    if (Math.hypot(s.x - start.x, s.z - start.z) <= 0.3) await fail('the stick did not walk the player');
    const lx = box.x + box.width * 0.75, ly = box.y + box.height * 0.4;
    await touch('touchStart', [{ x: lx, y: ly, id: 2 }]);
    for (let i = 1; i <= 5; i++) await touch('touchMove', [{ x: lx + 12 * i, y: ly, id: 2 }]);
    await touch('touchEnd', []);
    s = await until((st) => Math.abs(st.yaw - start.yaw) > 0.05);
    if (Math.abs(s.yaw - start.yaw) <= 0.05) await fail('a drag on the right half did not turn the view');
    await page.waitForTimeout(400);

    // Nana's window lantern with the big button, which now reads Light.
    await page.evaluate(() => window.familiar.toPlace(0));
    s = await until((st) => st.use === 'Light');
    await page.waitForFunction(() => document.getElementById('use').textContent.trim() === 'Light', null, { timeout: 10000 })
      .catch(() => {});
    const label = (await page.locator('#use').innerText()).trim();
    console.log(`at Nana's window: button ${label}, prompt ${s.prompt}`);
    if (label !== 'Light') await fail('at Nana\'s window the big button should read Light, not ' + label);
    await checkLayout('at the lantern');
    await page.locator('#use').tap();
    s = await until((st) => st.lanterns >= 1);
    if (s.lanterns < 1) await fail('tapping Light did not light Nana\'s window lantern');
    await page.waitForTimeout(800);
    await page.screenshot({ path: path.join(outDir, 'familiar-lantern.png'), timeout: 180000 });
    await page.locator('#bookBtn').tap();
    await page.waitForTimeout(300);
    if (!(await page.locator('#notebook').isVisible())) await fail('the Notebook button did not open the notebook');
    const book = await page.locator('#bookPages').innerText();
    if (!/Samhain/.test(book) || !/\[S1\]/.test(book)) await fail('the notebook lacks the Samhain page and its sources: ' + book.slice(0, 200));
    await page.screenshot({ path: path.join(outDir, 'familiar-notebook.png'), timeout: 180000 });
    await page.locator('#bookClose').tap();
    if (await page.locator('#notebook').isVisible()) await fail('Close did not close the notebook');
    console.log('ok: Light lit Nana\'s lantern; the notebook opened with the Samhain page');
    await page.locator('#catBtn').tap();
    s = await until((st) => st.cat);
    if (!s.cat) await fail('the Cat button did not turn Wren into a cat');
    await page.waitForTimeout(1500);
    await page.screenshot({ path: path.join(outDir, 'familiar-cat.png'), timeout: 180000 });
    if (!s.bagDown) await fail('turning into a cat did not leave the candy bag on the ground');
    await catChecks(async () => { await page.locator('#use').tap(); });
    await page.locator('#catBtn').tap();
    s = await until((st) => !st.cat);
    if (s.cat) await fail('the Be Wren button did not turn the cat back');
    if (s.spiritsShown !== 0) await fail('Wren can see the spirits (' + s.spiritsShown + ')');
  } else {
    await page.keyboard.down('KeyW');
    s = await until((st) => Math.hypot(st.x - start.x, st.z - start.z) > 0.3);
    await page.keyboard.up('KeyW');
    console.log(`after W: at (${s.x}, ${s.z})`);
    if (Math.hypot(s.x - start.x, s.z - start.z) <= 0.3) await fail('W did not walk the player');
    await page.evaluate(() => window.familiar.look(200, 0));
    s = await until((st) => Math.abs(st.yaw - start.yaw) > 0.1);
    if (Math.abs(s.yaw - start.yaw) <= 0.1) await fail('looking did not turn the view');
    await page.keyboard.press('KeyQ');
    s = await until((st) => st.cat);
    if (!s.cat) await fail('Q did not turn Wren into a cat');
    await page.waitForTimeout(1500);
    await page.screenshot({ path: path.join(outDir, 'familiar-cat.png'), timeout: 180000 });
    await page.keyboard.press('KeyQ');
    s = await until((st) => !st.cat);
    if (s.cat) await fail('Q did not turn the cat back into Wren');

    // Walking is checked above; SwiftShader's frames are too slow to steer
    // the 20 m to a door, so stand on the Okafors' doorstep (house 1) and
    // knock with E as a player would.
    await page.evaluate(() => window.familiar.toDoorstep(1));
    s = await until((st) => /knock/.test(st.prompt));
    console.log(`at the Okafors' door: (${s.x}, ${s.z}), prompt ${s.prompt}`);
    if (!/knock/.test(s.prompt)) await fail('standing on the Okafors\' doorstep gave no knock prompt: ' + s.prompt);
    await page.keyboard.press('KeyE');
    s = await until((st) => st.asking);
    if (!s.asking) await fail('Mrs. Okafor did not ask for a trick');
    await page.keyboard.press('KeyT');
    s = await until((st) => st.candy > 0);
    console.log(`after knocking: candy ${s.candy}, message ${s.message}`);
    if (s.candy <= 0) await fail('knocking at the Okafors gave no candy');
    await page.waitForTimeout(800);
    await page.screenshot({ path: path.join(outDir, 'familiar-knock.png'), timeout: 180000 });

    // The cat: the bag drops on the doorstep; the cat can't knock.
    await page.keyboard.press('KeyQ');
    s = await until((st) => st.cat && st.bagDown);
    if (!s.cat || !s.bagDown) await fail('turning into a cat did not leave the candy bag on the ground');
    await catChecks(async () => { await page.keyboard.press('KeyE'); });
    await page.keyboard.press('KeyQ');
    s = await until((st) => !st.cat);
    if (s.cat) await fail('Q did not turn the cat back into Wren in the churchyard');
    if (s.spiritsShown !== 0) await fail('Wren can see the spirits (' + s.spiritsShown + ')');
    await page.evaluate(() => window.familiar.toDoorstep(1));
    s = await until((st) => /pick up/.test(st.prompt));
    if (!/pick up/.test(s.prompt)) await fail('back at the bag there was no pick-up prompt: ' + s.prompt);
    const candy = s.candy;
    await page.keyboard.press('KeyE');
    s = await until((st) => !st.bagDown);
    if (s.bagDown || s.candy !== candy) await fail('E did not pick the bag up, candy and all');
    console.log('ok: the bag was picked up with ' + s.candy + ' candy');

    // Grandpa Ike's lantern, now the Okafors have been knocked at.
    await page.evaluate(() => window.familiar.toPlace(1));
    s = await until((st) => /light Grandpa Ike/i.test(st.prompt));
    if (!/light Grandpa Ike/i.test(s.prompt)) await fail('no prompt to light Grandpa Ike\'s lantern: ' + s.prompt);
    await page.keyboard.press('KeyE');
    s = await until((st) => st.lanterns >= 1);
    if (s.lanterns < 1) await fail('E did not light Grandpa Ike\'s lantern');
    await page.waitForTimeout(800);
    await page.screenshot({ path: path.join(outDir, 'familiar-lantern.png'), timeout: 180000 });
    // Tab opens Wren's notebook, which has the guising page now.
    await page.keyboard.press('Tab');
    await page.waitForTimeout(300);
    if (!(await page.locator('#notebook').isVisible())) await fail('Tab did not open the notebook');
    const book = await page.locator('#bookPages').innerText();
    if (!/Guising/.test(book)) await fail('the notebook lacks the guising page: ' + book.slice(0, 200));
    await page.keyboard.press('Tab');
    await page.waitForTimeout(300);
    if (await page.locator('#notebook').isVisible()) await fail('Tab did not close the notebook');
    // Candy is money: knock at Abbas's Market, then buy a cinnamon stick.
    await page.evaluate(() => window.familiar.toDoorstep(8));
    s = await until((st) => /knock/.test(st.prompt));
    const before = s.candy;
    await page.keyboard.press('KeyE');
    s = await until((st) => st.candy > before);
    s = await until((st) => /buy a cinnamon stick/.test(st.prompt));
    if (!/buy a cinnamon stick/.test(s.prompt)) await fail('the market offers no cinnamon: ' + s.prompt);
    const purse = s.candy;
    await page.keyboard.press('KeyE');
    s = await until((st) => st.cinnamon);
    console.log(`at the market: candy ${purse} -> ${s.candy}, cinnamon ${s.cinnamon}`);
    if (!s.cinnamon || s.candy !== purse - 3) await fail('buying cinnamon did not cost three candy');
    console.log('ok: E lit Grandpa Ike\'s lantern; Tab opened the notebook; candy bought a cinnamon stick');
  }
  // Saves. Save now (and wait until IndexedDB has it), reload, and the night
  // is where and when it was.
  const saveNow = async (why) => {
    await page.evaluate(() => {
      document.getElementById('saveNote').textContent = '';
      if (document.pointerLockElement) document.exitPointerLock();
    });
    await page.waitForTimeout(300);
    // A real click on the Save button; if headless Chromium will not deliver
    // one (it has reported the top bar as intercepting it after pointer
    // lock), say what is under the button and fire the button's click.
    await page.locator('#saveBtn').click({ timeout: 10000 }).catch(async (e) => {
      const under = await page.evaluate(() => {
        const r = document.getElementById('saveBtn').getBoundingClientRect();
        const el = document.elementFromPoint(r.x + r.width / 2, r.y + r.height / 2);
        return { rect: [r.x, r.y, r.width, r.height], under: el ? el.id || el.tagName : null, scroll: [scrollX, scrollY] };
      });
      console.log('(the Save button did not take a real click: ' + JSON.stringify(under) + '; firing its click)');
      await page.locator('#saveBtn').dispatchEvent('click');
    });
    await page.waitForFunction(() => /Saved in this browser/.test(document.getElementById('saveNote').textContent), null,
      { timeout: 20000 }).catch(() => {});
    const note = await page.locator('#saveNote').innerText();
    if (!/Saved in this browser/.test(note)) await fail(why + ': the save note did not confirm: ' + note);
  };
  const same = (a, b) => Math.hypot(a.x - b.x, a.z - b.z) < 0.25 && a.candy === b.candy && a.costume === b.costume &&
    a.jokes === b.jokes && a.knocked === b.knocked && a.cat === b.cat && a.bagDown === b.bagDown && a.spiritsSeen === b.spiritsSeen &&
    a.haleGate === b.haleGate && a.lychgate === b.lychgate;
  const before = await status();  // the save's tick is between this and the next
  await saveNow('saving');
  const saved = await status();
  await open(true);
  let back = await until((st) => /Loaded your save/.test(st.message), 10000);
  console.log(`after a reload: at (${back.x}, ${back.z}) at ${back.clock}, candy ${back.candy}, costume ${back.costume}; "${back.message}"`);
  if (!same(back, saved)) await fail('the save did not survive a reload: ' + JSON.stringify({ saved, back }));
  // (The page saves again as it unloads, so the time may be a little later than the Save button's.)
  if (back.tick < before.tick || back.tick - saved.tick > 30 * 10)
    await fail(`the time of night did not survive a reload (saved between ${before.tick} and ${saved.tick}; back at ${back.tick})`);
  if (!/Loaded your save/.test(back.message)) await fail('no "Loaded your save" message after a reload: ' + back.message);
  if (await page.locator('#help').isVisible()) await fail('How to play came back after it was closed once');
  await page.screenshot({ path: path.join(outDir, 'familiar-reloaded.png'), timeout: 180000 });

  // A damaged save is refused and the one before it loads. Save twice (so the
  // one before is this night too), stop the page saving as it unloads, flip
  // one byte of the latest save in IndexedDB, and reload.
  await saveNow('saving before the damage');
  await saveNow('saving again before the damage');
  const kept = await status();
  const flipped = await page.evaluate(async () => {
    window.familiar.save = () => 0;  // the page's unload save would replace the damaged file
    const file = '/familiar-save/default/familiar.save';
    const bytes = Module.FS.readFile(file);
    const at = Math.floor(bytes.length / 2);
    bytes[at] ^= 0x01;
    Module.FS.writeFile(file, bytes);
    await new Promise((resolve, reject) => Module.FS.syncfs(false, (err) => (err ? reject(err) : resolve())));
    return { at, length: bytes.length };
  });
  console.log(`flipped byte ${flipped.at} of ${flipped.length} in the latest save`);
  await open(true);
  back = await until((st) => /refused/.test(st.message), 10000);
  console.log(`after a reload with the save damaged: "${back.message}"`);
  if (!/refused/.test(back.message) || !/Loaded the one before it/.test(back.message))
    await fail('a damaged save was not refused with a message: ' + back.message);
  if (!same(back, kept)) await fail('the save before the damaged one did not load: ' + JSON.stringify({ kept, back }));

  // The loading shell finished; and without WebGL 2 it shows the trailer (or
  // its still) and says why, and the engine never downloads.
  {
    await page.waitForFunction(() => { const r = document.querySelector('.sk-loader'); return r && r.hidden; }, null, { timeout: 10000 }).catch(() => {});
    const shell = await page.evaluate(() => {
      const r = document.querySelector('.sk-loader');
      return { hidden: !!(r && r.hidden), state: window.StrataLoader && StrataLoader.state };
    });
    if (!shell.hidden || shell.state !== 'done') await fail('the loading shell did not finish: ' + JSON.stringify(shell));
    console.log('ok: the loading shell showed and is gone');
    const bare = await browser.newContext({ ...base, userAgent: (base.userAgent || 'Mozilla/5.0') + ' familiar-smoke headless' });
    await bare.addInitScript(() => {
      const get = HTMLCanvasElement.prototype.getContext;
      HTMLCanvasElement.prototype.getContext = function (t, o) { return t === 'webgl2' ? null : get.call(this, t, o); };
    });
    const p2 = await bare.newPage();
    const engine = [];
    p2.on('request', (r) => { if (/familiar\.(wasm|js)$/.test(r.url())) engine.push(r.url()); });
    await p2.goto(url, { waitUntil: 'commit' });
    await p2.waitForFunction(() => window.StrataLoader && StrataLoader.state === 'fallback', null, { timeout: 30000 });
    const fb = await p2.evaluate(() => {
      const r = document.querySelector('.sk-loader');
      return { text: r.innerText, media: !!r.querySelector('video, img'), wide: document.documentElement.scrollWidth > innerWidth };
    });
    if (!/WebGL 2/.test(fb.text) || fb.wide) await fail('without WebGL 2 the shell did not say why: ' + JSON.stringify(fb));
    if (engine.length) await fail('without WebGL 2 the engine still downloaded: ' + engine.join(', '));
    console.log('ok: without WebGL 2 the shell ' + (fb.media ? 'shows the trailer and ' : '') + 'says why, and nothing of the engine downloads');
    // A page screenshot waits for a still frame, which a software-drawn page may not give in time.
    await p2.screenshot({ path: path.join(outDir, 'no-webgl2.png'), timeout: 15000 })
      .catch((e) => console.log('(no-WebGL 2 screenshot skipped: ' + String(e.message || e).split('\n')[0] + ')'));
    await bare.close();
  }

  if (errors.length) await fail('page errors: ' + errors.join(' | '));
  console.log('ok');
  await browser.close();
})();
