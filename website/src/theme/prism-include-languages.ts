// Swizzle of prism-include-languages to register the `mtype` language.
//
// Canonical grammar source-of-truth lives at:
//   mtype-vscode-extension/syntaxes/mtype.tmGrammar.json
// (TextMate format, scope `source.mtype`).
//
// This file is a thin Prism-compatible translation kept intentionally small.
// When tokens change in the TextMate grammar, mirror the additions/removals here.

import siteConfig from '@generated/docusaurus.config';
import type * as PrismNamespace from 'prismjs';
import type { Optional } from 'utility-types';

export default function prismIncludeLanguages(
  PrismObject: typeof PrismNamespace,
): void {
  const {
    themeConfig: { prism },
  } = siteConfig;
  const { additionalLanguages } = prism as { additionalLanguages: string[] };

  // Register additional languages from config first.
  globalThis.Prism = PrismObject;
  additionalLanguages.forEach((lang) => {
    if (lang === 'php') {
      require('prismjs/components/prism-markup-templating.js');
    }
    require(`prismjs/components/prism-${lang}`);
  });

  // Register `mtype` — derived from mtype.tmGrammar.json (source.mtype).
  PrismObject.languages.mtype = {
    comment: [
      { pattern: /\/\*[\s\S]*?\*\//, greedy: true },
      { pattern: /\/\/.*/, greedy: true },
    ],
    'string-interpolation': {
      pattern: /\$"(?:[^"\\]|\\.)*"/,
      greedy: true,
      inside: {
        interpolation: {
          pattern: /\{[^}]*\}/,
          inside: { punctuation: /[{}]/ },
        },
        string: /[\s\S]+/,
      },
    },
    string: {
      pattern: /"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*'/,
      greedy: true,
    },
    annotation: {
      pattern: /@[A-Z][A-Za-z0-9_]*/,
      alias: 'attr-name',
    },
    keyword:
      /\b(?:abstract|annotation|async|await|break|case|catch|class|const|constructor|continue|default|do|else|extends|final|finally|for|function|if|implements|import|in|interface|match|new|null|private|protected|public|return|static|super|switch|this|throw|try|value|while|from|as)\b/,
    boolean: /\b(?:true|false)\b/,
    'builtin-type':
      /\b(?:int|float|bool|string|void|Object|Int|Float|Bool|String|Box|Promise|Class|Method|Field|Constructor|Annotation|Exception|RuntimeException|ArrayList|LinkedList|HashMap|HashSet|Stack|Queue|Stream|Vec2f|Vec3f|Vec4f|Matrix3f|Matrix4f|Quaternion)\b/,
    number:
      /\b0x[\da-f]+(?:\.[\da-f]+)?\b|\b\d+(?:\.\d+)?(?:[eE][+-]?\d+)?[fFlL]?/,
    'class-name': {
      pattern: /\b[A-Z][A-Za-z0-9_]*\b/,
      alias: 'class-name',
    },
    'generic-type': {
      pattern: /<[A-Z][A-Za-z0-9_, <>?]*>/,
      inside: {
        punctuation: /[<>,]/,
        'class-name': /\b[A-Z][A-Za-z0-9_]*\b/,
      },
    },
    function: {
      pattern: /\b[a-z][A-Za-z0-9_]*(?=\s*\()/,
    },
    operator:
      /->|=>|::|==|!=|<=|>=|&&|\|\||\+\+|--|\+=|-=|\*=|\/=|%=|<<|>>|[+\-*/%=<>!?]/,
    punctuation: /[{}[\]();,.:]/,
  };
}
