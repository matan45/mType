import type { SidebarsConfig } from '@docusaurus/plugin-content-docs';

const sidebars: SidebarsConfig = {
  docsSidebar: [
    'index',
    {
      type: 'category',
      label: 'Getting Started',
      collapsed: false,
      items: [
        'getting-started/install',
        'getting-started/quick-start',
        'getting-started/first-program',
      ],
    },
    {
      type: 'category',
      label: 'Language Reference',
      collapsed: false,
      items: [
        'language/primitives',
        'language/classes',
        'language/interfaces',
        'language/generics',
        'language/lambdas',
        'language/control-flow',
        'language/imports',
        'language/annotations',
        'language/async-await',
        'language/pattern-matching',
      ],
    },
    {
      type: 'category',
      label: 'Standard Library',
      collapsed: true,
      items: [
        'stdlib/overview',
        'stdlib/primitives',
        'stdlib/collections',
        'stdlib/stream',
        'stdlib/exceptions',
        'stdlib/math',
        'stdlib/net',
        'stdlib/reflect',
        'stdlib/json',
        'stdlib/mtest',
      ],
    },
    {
      type: 'category',
      label: 'CLI & Tooling',
      collapsed: true,
      items: [
        'cli/reference',
        'cli/projects',
        'cli/package-manager',
        'cli/vscode-extension',
        'cli/language-server',
        'cli/native-interop',
      ],
    },
    {
      type: 'category',
      label: 'Architecture',
      collapsed: true,
      items: [
        'architecture/overview',
        'architecture/pipeline',
        'architecture/vm',
        'architecture/jit',
      ],
    },
    {
      type: 'category',
      label: 'Reference',
      collapsed: true,
      items: [
        'reference/limitations',
        'reference/benchmarks',
        'reference/bench-baseline',
      ],
    },
    'contributing',
  ],
};

export default sidebars;
