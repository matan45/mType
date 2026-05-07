// Prebuild step: copies asset files from canonical locations in the repo
// into the website tree. Single-source-of-truth stays in the original
// locations; this script keeps the site in sync.
//
// Sources:
//   ../docs/{limitations,benchmarks,bench-baseline}.md   -> docs/reference/
//   ../mtype-vscode-extension/syntaxes/*.json            -> src/grammars/
//   ../mtype-vscode-extension/themes/*.json              -> src/themes-vscode/
//   ../logo.png                                          -> static/img/logo.png + favicon.png
//
// Run automatically via the npm `presync` / `prestart` / `prebuild` hooks.

import { promises as fs } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const websiteRoot = resolve(__dirname, '..');
const repoRoot = resolve(websiteRoot, '..');

async function ensureDir(dir) {
  await fs.mkdir(dir, { recursive: true });
}

async function copyFile(src, dest, label) {
  try {
    await ensureDir(dirname(dest));
    await fs.copyFile(src, dest);
    console.log(`[sync-assets] ${label}: ${src} -> ${dest}`);
  } catch (err) {
    if (err.code === 'ENOENT') {
      console.warn(`[sync-assets] SKIP ${label}: source not found at ${src}`);
    } else {
      throw err;
    }
  }
}

// MDX 3 (used by Docusaurus 3) treats `<` as the start of a JSX tag. The
// benchmark prose contains `<2%`, `Promise<Int>`, generic type references
// like `<HashMap<K,V>>`, etc. — all of which trip the parser.
//
// Escape any `<` that's followed by an identifier-start character (or `/X`
// for a closing tag), but only in regions that aren't fenced code blocks
// or inline code spans, where MDX preserves contents verbatim and an added
// `\` would render literally.
function escapeAngleBracketsOutsideCode(content) {
  // Split on (fenced ``` ... ``` | fenced ~~~ ... ~~~ | inline `...`).
  // Capture groups preserve delimiters so we can pass them through unchanged.
  const parts = content.split(
    /(```[\s\S]*?```|~~~[\s\S]*?~~~|`[^`\n]*`)/g,
  );
  return parts
    .map((part, i) => {
      if (i % 2 === 1) return part; // inside a code region — leave alone
      return part.replace(/<(?=\/?[A-Za-z0-9])/g, '\\<');
    })
    .join('');
}

async function copyDocs() {
  const sourceDir = join(repoRoot, 'docs');
  const destDir = join(websiteRoot, 'docs', 'reference');
  const files = ['limitations.md', 'benchmarks.md', 'bench-baseline.md'];
  // Map of cross-refs that need rewriting once these files are co-located.
  const linkRewrites = [
    [/\bdocs\/limitations\.md\b/g, './limitations.md'],
    [/\bdocs\/benchmarks\.md\b/g, './benchmarks.md'],
    [/\bdocs\/bench-baseline\.md\b/g, './bench-baseline.md'],
  ];
  for (const name of files) {
    const src = join(sourceDir, name);
    const dest = join(destDir, name);
    try {
      let raw = await fs.readFile(src, 'utf8');
      for (const [pattern, replacement] of linkRewrites) {
        raw = raw.replace(pattern, replacement);
      }
      raw = escapeAngleBracketsOutsideCode(raw);
      const stem = name.replace(/\.md$/, '');
      const title = stem
        .replace(/[-_]/g, ' ')
        .replace(/\b\w/g, (c) => c.toUpperCase());
      const frontmatter = [
        '---',
        `title: ${title}`,
        `slug: /reference/${stem}`,
        '---',
        '',
      ].join('\n');
      // Avoid duplicating front matter on rerun.
      const out = raw.startsWith('---\n') ? raw : frontmatter + raw;
      await ensureDir(destDir);
      await fs.writeFile(dest, out, 'utf8');
      console.log(`[sync-assets] docs reference: ${src} -> ${dest}`);
    } catch (err) {
      if (err.code === 'ENOENT') {
        console.warn(`[sync-assets] SKIP docs reference: source not found at ${src}`);
      } else {
        throw err;
      }
    }
  }
}

async function copyGrammar() {
  const src = join(repoRoot, 'mtype-vscode-extension', 'syntaxes', 'mtype.tmGrammar.json');
  const dest = join(websiteRoot, 'src', 'grammars', 'mtype.tmGrammar.json');
  await copyFile(src, dest, 'TextMate grammar');
}

async function copyThemes() {
  const sourceDir = join(repoRoot, 'mtype-vscode-extension', 'themes');
  const destDir = join(websiteRoot, 'src', 'themes-vscode');
  for (const name of ['mtype-dark-theme.json', 'mtype-light-theme.json']) {
    await copyFile(join(sourceDir, name), join(destDir, name), `VS Code theme ${name}`);
  }
}

async function copyLogo() {
  const src = join(repoRoot, 'logo.png');
  await copyFile(src, join(websiteRoot, 'static', 'img', 'logo.png'), 'logo');
  await copyFile(src, join(websiteRoot, 'static', 'img', 'favicon.png'), 'favicon');
}

async function main() {
  console.log('[sync-assets] starting');
  await copyDocs();
  await copyGrammar();
  await copyThemes();
  await copyLogo();
  console.log('[sync-assets] done');
}

main().catch((err) => {
  console.error('[sync-assets] failed:', err);
  process.exit(1);
});
