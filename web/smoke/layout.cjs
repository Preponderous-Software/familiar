// Phone layout check for the browser build in headless Chromium (Playwright).
//
//   node web/smoke/layout.cjs <url> <out-dir> [--shots] [device name ...]
//
// For each phone profile (portrait and landscape), walks the page through the
// states that change the touch controls: choosing a costume, standing at a
// lantern, an apple tree and the market (M4), the notebook, a door, being
// asked for a trick, the cat (at a latch and among the spirits),
// and back by the candy bag. In each state it checks that
// every visible button is inside the screen, at least 44 by 44 CSS pixels, and
// that no button overlaps another button or the words the game shows (the HUD
// panel, the prompt and the message). It also checks that the words name the
// buttons, not the keys. Writes layout.json to <out-dir>, and with --shots a
// PNG per device and state too (slow under SwiftShader, so CI leaves it off).
const fs = require('fs');
const path = require('path');
const { chromium, devices } = require('playwright');

const [url, outDir, ...rest] = process.argv.slice(2);
const shots = rest.includes('--shots');
const names = rest.filter((a) => a !== '--shots');
if (!url || !outDir) {
  console.error('usage: layout.cjs <url> <out-dir> [device name ...]');
  process.exit(2);
}
fs.mkdirSync(outDir, { recursive: true });

const DEFAULT_DEVICES = [
  'iPhone SE', 'iPhone SE landscape',
  'iPhone 13', 'iPhone 13 landscape',
  'Pixel 7', 'Pixel 7 landscape',
  'Galaxy S5', 'Galaxy S5 landscape',  // a small Android, 360 by 640
];
const MIN = 44;

// Every visible control and the overlays it must not cover, in CSS pixels.
function measure() {
  const vis = (el) => {
    if (!el) return false;
    const cs = getComputedStyle(el);
    if (cs.display === 'none' || cs.visibility === 'hidden') return false;
    for (let p = el; p; p = p.parentElement) if (getComputedStyle(p).display === 'none') return false;
    const r = el.getBoundingClientRect();
    return r.width > 0 && r.height > 0;
  };
  const box = (el) => { const r = el.getBoundingClientRect(); return { x: r.left, y: r.top, w: r.width, h: r.height }; };
  const name = (el) => '#' + (el.id || (el.parentElement.id + '>' + el.textContent.trim()));
  const buttons = Array.from(document.querySelectorAll('button')).filter(vis)
    .filter((b) => !b.closest('.dialog'))
    .map((b) => ({ name: name(b), text: b.textContent.trim(), ...box(b) }));
  const words = ['panel', 'prompt', 'msg'].map((id) => document.getElementById(id))
    .filter((el) => vis(el) && el.textContent.trim())
    .map((el) => ({ name: '#' + el.id, text: el.textContent.trim(), ...box(el) }));
  const top = document.getElementById('top').getBoundingClientRect();
  const topButtons = Array.from(document.querySelectorAll('#top button')).filter(vis).map((b) => b.getBoundingClientRect().top);
  const oneRow = topButtons.every((t) => Math.abs(t - topButtons[0]) < 2);
  return { vw: innerWidth, vh: innerHeight, scrollW: document.documentElement.scrollWidth, buttons, words, oneRow, topH: top.height };
}

function overlap(a, b) {
  const x = Math.min(a.x + a.w, b.x + b.w) - Math.max(a.x, b.x);
  const y = Math.min(a.y + a.h, b.y + b.h) - Math.max(a.y, b.y);
  return x > 0.5 && y > 0.5;
}

function problems(m, state) {
  const out = [];
  if (m.scrollW > m.vw + 1) out.push(`the page scrolls sideways (${m.scrollW} > ${m.vw})`);
  if (!m.oneRow) out.push(`the top bar wraps onto a second row (${Math.round(m.topH)} px tall)`);
  for (const b of m.buttons) {
    if (b.w < MIN || b.h < MIN) out.push(`${b.name} is ${Math.round(b.w)}x${Math.round(b.h)}, under ${MIN} px`);
    if (b.x < -0.5 || b.y < -0.5 || b.x + b.w > m.vw + 0.5 || b.y + b.h > m.vh + 0.5)
      out.push(`${b.name} runs off the screen`);
  }
  for (let i = 0; i < m.buttons.length; i++)
    for (let j = i + 1; j < m.buttons.length; j++)
      if (overlap(m.buttons[i], m.buttons[j])) out.push(`${m.buttons[i].name} overlaps ${m.buttons[j].name}`);
  for (const b of m.buttons) for (const w of m.words) if (overlap(b, w)) out.push(`${b.name} covers ${w.name}`);
  for (let i = 0; i < m.words.length; i++)
    for (let j = i + 1; j < m.words.length; j++)
      if (overlap(m.words[i], m.words[j])) out.push(`${m.words[i].name} overlaps ${m.words[j].name}`);
  // On a touch screen the words name the buttons: no keys, no doubled names.
  for (const w of m.words.concat(m.buttons)) {
    if (/\(E\)|\(T\b|\(Q\b|\b[EQT]  |\bQ to\b|1-4|\bTab\b/.test(w.text)) out.push(`${w.name} names a key: "${w.text}"`);
    if (/Knock \(Knock\)/.test(w.text)) out.push(`${w.name} says "Knock (Knock)"`);
  }
  return out.map((p) => `${state}: ${p}`);
}

(async () => {
  const browser = await chromium.launch({ args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
  const list = names.length ? names : DEFAULT_DEVICES;
  const report = [];
  let bad = 0;
  for (const dev of list) {
    const d = devices[dev];
    if (!d) { console.error('no such device: ' + dev); process.exit(2); }
    const { defaultBrowserType, ...profile } = d;  // every profile runs in Chromium here
    const context = await browser.newContext({ ...profile, userAgent: profile.userAgent + ' familiar-smoke headless' });
    const page = await context.newPage();
    const errors = [];
    page.on('pageerror', (e) => errors.push(String(e)));
    await page.goto(url, { waitUntil: 'commit' });
    await page.waitForFunction(() => window.familiar && window.familiarStatus, null, { timeout: 90000 });
    await page.locator('#helpClose').tap({ force: true, timeout: 60000 });
    const status = async () => JSON.parse(await page.evaluate(() => window.familiar.status()));
    const until = async (pred, ms = 15000) => {
      const end = Date.now() + ms;
      let st = await status();
      while (!pred(st) && Date.now() < end) { await page.waitForTimeout(150); st = await status(); }
      return st;
    };
    const slug = dev.replace(/\s+/g, '-').toLowerCase();
    const found = [];
    const check = async (state) => {
      await page.waitForTimeout(400);
      const m = await page.evaluate(measure);
      found.push(...problems(m, state));
      if (shots) await page.screenshot({ path: path.join(outDir, `${slug}-${state}.png`), timeout: 120000, scale: 'css' }).catch(() => {});
      return m;
    };
    if (!(await status()).choosing) found.push('start: no costume offered');
    await check('costume');
    await page.locator('#costumes [data-costume="2"]').tap({ force: true, timeout: 60000 });
    await until((s) => s.costume === 2);
    // Witchcraft (M4): the big button's other names, the market, the notebook.
    await page.evaluate(() => window.familiar.toPlace(0));
    let st0 = await until((st) => st.use === 'Light');
    await check('lantern');
    await page.locator('#use').tap({ force: true, timeout: 60000 });
    st0 = await until((st) => st.lanterns >= 1);
    if (st0.lanterns < 1) found.push('lantern: tapping Light lit no lantern');
    await page.evaluate(() => window.familiar.toPlace(4));
    st0 = await until((st) => st.use === 'Pick');
    if (st0.use !== 'Pick') found.push('apple: the big button does not say Pick: ' + st0.use);
    await page.locator('#use').tap({ force: true, timeout: 60000 });
    st0 = await until((st) => st.apples >= 1);
    if (st0.apples < 1) found.push('apple: tapping Pick picked no apple');
    await page.evaluate(() => window.familiar.toDoorstep(8));
    st0 = await until((st) => /knock/.test(st.prompt));
    await page.locator('#use').tap({ force: true, timeout: 60000 });
    st0 = await until((st) => st.haggle || st.use === 'Buy');
    await check(st0.haggle ? 'haggle' : 'market');
    if (st0.haggle) {
      const offer = await page.waitForFunction(() => document.getElementById('trickBtn').textContent === 'Offer a joke', null, { timeout: 10000 }).catch(() => null);
      if (!offer) found.push('haggle: no Offer a joke button');
      await page.locator('#trickBtn').tap({ force: true, timeout: 60000 });
    } else if (st0.use === 'Buy') {
      await page.locator('#use').tap({ force: true, timeout: 60000 });
    } else found.push('market: neither Buy nor Offer a joke after knocking: ' + st0.prompt);
    st0 = await until((st) => st.cinnamon);
    if (!st0.cinnamon) found.push('market: no cinnamon stick by touch');
    await page.locator('#bookBtn').tap({ force: true, timeout: 60000 });
    {
      const b = await page.evaluate(() => {
        const r = document.getElementById('notebook').getBoundingClientRect();
        const c = document.getElementById('bookClose').getBoundingClientRect();
        return { top: r.top, bottom: r.bottom, vh: innerHeight, cTop: c.top, cBottom: c.bottom, cw: c.width, ch: c.height };
      });
      if (b.top < 0 || b.bottom > b.vh + 0.5) found.push('notebook: runs off the screen');
      if (b.cTop < b.top - 0.5 || b.cBottom > b.bottom + 0.5) found.push('notebook: Close is not in view');
      if (b.cw < MIN || b.ch < MIN) found.push(`notebook: Close is ${Math.round(b.cw)}x${Math.round(b.ch)}`);
      if (shots) await page.screenshot({ path: path.join(outDir, `${slug}-notebook.png`), timeout: 120000, scale: 'css' }).catch(() => {});
    }
    await page.locator('#bookClose').tap({ force: true, timeout: 60000 });
    await page.evaluate(() => window.familiar.toDoorstep(1));
    await until((s) => /knock/i.test(s.prompt));
    await check('door');
    await page.locator('#use').tap({ force: true, timeout: 60000 });
    let s = await until((st) => st.asking);
    if (!s.asking) found.push('door: tapping Knock did not bring a neighbour to the door');
    await check('trick');
    await page.locator('#trickBtn').tap({ force: true, timeout: 60000 });
    s = await until((st) => st.candy > 0);
    if (s.candy <= 0) found.push('trick: tapping Tell a joke gave no candy');
    await check('candy');
    await page.locator('#catBtn').tap({ force: true, timeout: 60000 });
    s = await until((st) => st.cat);
    if (!s.cat) found.push('cat: the Cat button did not turn Wren into a cat');
    await check('cat');
    // The cat (M3): a latch to open, then the spirits' long words.
    await page.evaluate(() => window.familiar.toSpot(0));
    s = await until((st) => st.use === 'Open');
    await check('latch');
    await page.locator('#use').tap({ force: true, timeout: 60000 });
    s = await until((st) => st.haleGate);
    if (!s.haleGate) found.push('latch: tapping Open did not open the gate');
    await page.evaluate(() => window.familiar.toSpot(2));
    s = await until((st) => st.spiritsSeen > 0);
    await check('spirits');
    await page.locator('#catBtn').tap({ force: true, timeout: 60000 });
    s = await until((st) => !st.cat);
    if (s.cat) found.push('spirits: Be Wren did not turn the cat back');
    await page.evaluate(() => window.familiar.toDoorstep(1));
    s = await until((st) => st.use === 'Pick up');
    await check('bag');
    await page.locator('#use').tap({ force: true, timeout: 60000 });
    s = await until((st) => !st.bagDown);
    if (s.bagDown) found.push('bag: tapping Pick up did not pick the bag up');
    await page.locator('#helpBtn').tap({ force: true, timeout: 60000 });
    {
      const h = await page.evaluate(() => {
        const r = document.getElementById('help').getBoundingClientRect();
        const b = document.getElementById('helpClose').getBoundingClientRect();
        return { top: r.top, bottom: r.bottom, vh: innerHeight, closeH: b.height, closeW: b.width };
      });
      if (h.top < 0 || h.bottom > h.vh + 0.5) found.push('help: How to play runs off the screen');
      if (h.closeH < MIN || h.closeW < MIN) found.push(`help: Play is ${Math.round(h.closeW)}x${Math.round(h.closeH)}`);
      if (shots) await page.screenshot({ path: path.join(outDir, `${slug}-help.png`), timeout: 120000, scale: 'css' }).catch(() => {});
    }
    if (errors.length) found.push('page errors: ' + errors.join(' | '));
    console.log(`${found.length ? 'FAIL' : 'ok  '} ${dev} (${profile.viewport.width}x${profile.viewport.height})`);
    for (const f of found) console.log('     ' + f);
    report.push({ device: dev, viewport: profile.viewport, problems: found });
    bad += found.length;
    await context.close();
  }
  fs.writeFileSync(path.join(outDir, 'layout.json'), JSON.stringify(report, null, 2));
  await browser.close();
  process.exit(bad ? 1 : 0);
})();
