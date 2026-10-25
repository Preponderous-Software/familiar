// Smoke test for the browser build in headless Chromium (Playwright).
//
//   node web/smoke/smoke.cjs <url> <out-dir> [--phone]
//
// Checks: the page starts with no page errors and shows How to play on a first
// visit; the canvas draws the town (not one flat colour); walking moves the
// player (W on a desktop, the stick on the phone profile) and looking turns
// them; Q (or the Cat button) turns Wren into a cat with a lower eye, and back.
// On a desktop the player also walks to the Okafors' door and knocks, and the
// candy count goes up. Writes PNGs to <out-dir>. (Atman's smoke test, for
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
  await page.goto(url, { waitUntil: 'commit' });
  try {
    await page.waitForFunction(() => window.familiar && window.familiarStatus, null, { timeout: 90000 });
  } catch (e) {
    await fail('the game never became ready: ' + errors.join(' | '));
  }
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

  if (!(await page.locator('#help').isVisible())) await fail('How to play was not shown on a first visit');
  await page.locator('#helpClose').click();
  let s = await status();
  console.log(`ready: ${s.clock} at (${s.x}, ${s.z})`);
  if (s.clock !== '5:20 pm') await fail('the night should start at 5:20 pm, not ' + s.clock);
  await page.waitForTimeout(1500);
  const shot = await page.screenshot({ path: path.join(outDir, 'familiar.png'), timeout: 180000 });
  const colours = colourCount(shot);
  console.log(`screenshot: ${colours} distinct colours`);
  if (colours < 12) await fail('the canvas looks blank (' + colours + ' colours)');

  const start = { x: s.x, z: s.z, yaw: s.yaw };
  const box = await page.locator('#canvas').boundingBox();
  if (phone) {
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
    await page.locator('#catBtn').tap();
    s = await until((st) => st.cat);
    if (!s.cat) await fail('the Cat button did not turn Wren into a cat');
    await page.waitForTimeout(1500);
    await page.screenshot({ path: path.join(outDir, 'familiar-cat.png'), timeout: 180000 });
    await page.locator('#catBtn').tap();
    s = await until((st) => !st.cat);
    if (s.cat) await fail('the Be Wren button did not turn the cat back');
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

    // To the Okafors' door, steering with the same look() call, hurrying.
    // Hurry along the street, then walk slowly through the gate and up the
    // path (SwiftShader frames are slow, so steering is coarse).
    const route = [[-66.0, -2.5, true, 0.8], [-48.0, -2.5, true, 0.6], [-48.0, -4.0, false, 0.4], [-48.0, -9.6, false, 1.0]];
    for (const [tx, tz, hurry, near] of route) {
      const end = Date.now() + 120000;
      if (hurry) await page.keyboard.down('ShiftLeft');
      await page.keyboard.down('KeyW');
      for (;;) {
        s = await status();
        const dx = tx - s.x, dz = tz - s.z;
        if (Math.hypot(dx, dz) < near || Date.now() > end) break;
        let d = Math.atan2(dx, -dz) - s.yaw;
        while (d > Math.PI) d -= 2 * Math.PI;
        while (d < -Math.PI) d += 2 * Math.PI;
        await page.evaluate((px) => window.familiar.look(px, 0), d / 0.0025);
        await page.waitForTimeout(50);
      }
      await page.keyboard.up('KeyW');
      if (hurry) await page.keyboard.up('ShiftLeft');
      if (Math.hypot(tx - s.x, tz - s.z) >= near) await fail(`could not walk to (${tx}, ${tz}); at (${s.x}, ${s.z})`);
    }
    await page.keyboard.press('KeyE');
    s = await until((st) => st.candy > 0);
    console.log(`after knocking: candy ${s.candy}, message ${s.message}`);
    if (s.candy <= 0) await fail('knocking at the Okafors gave no candy');
    await page.waitForTimeout(800);
    await page.screenshot({ path: path.join(outDir, 'familiar-knock.png'), timeout: 180000 });
  }
  if (errors.length) await fail('page errors: ' + errors.join(' | '));
  console.log('ok');
  await browser.close();
})();
