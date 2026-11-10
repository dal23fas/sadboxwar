// Headless check of the viewer: loads a real .efsave + content JSON into index.html with Playwright,
// asserts the rendered DOM and writes a screenshot. Usage: node check_viewer.mjs <save.efsave> <content.json> <out.png>
import { chromium } from 'playwright';
import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';

const [,, savePath, contentPath, outPng] = process.argv;
const save = JSON.parse(readFileSync(savePath, 'utf8'));
const content = JSON.parse(readFileSync(contentPath, 'utf8'));
const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1400, height: 1000 } });
const errors = [];
page.on('pageerror', (e) => errors.push(String(e)));
await page.goto('file://' + resolve(new URL('.', import.meta.url).pathname, 'index.html'));
await page.evaluate(([s, c]) => window.efViewerLoad(s, c), [save, content]);
const meta = await page.textContent('#meta');
const sections = await page.$$eval('section h2', (hs) => hs.map((h) => h.textContent));
const missions = await page.$$eval('section .tag.ok', (t) => t.length);
await page.screenshot({ path: outPng, fullPage: true });
await browser.close();
console.log('meta:', meta);
console.log('sections:', sections.join(' | '));
console.log('completed-mission tags:', missions, '· page errors:', errors.length);
if (errors.length || !meta.includes('tick') || sections.length < 6) { console.error('VIEWER CHECK FAILED', errors); process.exit(1); }
console.log('viewer check: PASS →', outPng);
