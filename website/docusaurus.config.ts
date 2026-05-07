import { themes as prismThemes } from 'prism-react-renderer';
import type { Config } from '@docusaurus/types';
import type * as Preset from '@docusaurus/preset-classic';

const config: Config = {
  title: 'mType',
  tagline: 'A statically typed, class-based language with a bytecode VM, JIT, and rich tooling',
  favicon: 'img/favicon.png',

  url: 'https://matan45.github.io',
  baseUrl: '/mType/',

  organizationName: 'matan45',
  projectName: 'mType',
  trailingSlash: false,

  onBrokenLinks: 'throw',
  onBrokenMarkdownLinks: 'warn',

  i18n: {
    defaultLocale: 'en',
    locales: ['en'],
  },

  presets: [
    [
      'classic',
      {
        docs: {
          sidebarPath: './sidebars.ts',
          editUrl: 'https://github.com/matan45/mType/edit/dev/website/',
          routeBasePath: 'docs',
        },
        blog: false,
        theme: {
          customCss: './src/css/custom.css',
        },
      } satisfies Preset.Options,
    ],
  ],

  themeConfig: {
    image: 'img/social-card.png',
    colorMode: {
      defaultMode: 'dark',
      respectPrefersColorScheme: true,
    },
    navbar: {
      title: 'mType',
      logo: {
        alt: 'mType Logo',
        src: 'img/logo.png',
      },
      items: [
        {
          type: 'docSidebar',
          sidebarId: 'docsSidebar',
          position: 'left',
          label: 'Docs',
        },
        {
          to: '/docs/getting-started/install',
          label: 'Get Started',
          position: 'left',
        },
        {
          to: '/docs/cli/reference',
          label: 'CLI',
          position: 'left',
        },
        {
          href: 'https://github.com/matan45/mType/releases',
          label: 'Releases',
          position: 'right',
        },
        {
          href: 'https://github.com/matan45/mType',
          label: 'GitHub',
          position: 'right',
        },
      ],
    },
    footer: {
      style: 'dark',
      links: [
        {
          title: 'Docs',
          items: [
            { label: 'Getting Started', to: '/docs/getting-started/install' },
            { label: 'Language Reference', to: '/docs/language/classes' },
            { label: 'Standard Library', to: '/docs/stdlib/overview' },
            { label: 'CLI Reference', to: '/docs/cli/reference' },
          ],
        },
        {
          title: 'Project',
          items: [
            { label: 'GitHub', href: 'https://github.com/matan45/mType' },
            { label: 'Standard Library Repo', href: 'https://github.com/matan45/mTypeLib' },
            { label: 'Releases', href: 'https://github.com/matan45/mType/releases' },
            { label: 'CI', href: 'https://github.com/matan45/mType/actions/workflows/ci.yml' },
          ],
        },
        {
          title: 'Tooling',
          items: [
            { label: 'VS Code Extension', href: 'https://github.com/matan45/mType/tree/dev/mtype-vscode-extension' },
            { label: 'Language Server', href: 'https://github.com/matan45/mType/tree/dev/languageserver' },
            { label: 'Package Manager (mtpm)', href: 'https://github.com/matan45/mType/tree/dev/packagemanager' },
          ],
        },
      ],
      copyright: `Copyright © ${new Date().getFullYear()} mType. MIT License. Built with Docusaurus.`,
    },
    prism: {
      theme: prismThemes.github,
      darkTheme: prismThemes.vsDark,
      additionalLanguages: ['bash', 'json', 'yaml', 'cpp', 'powershell'],
    },
    docs: {
      sidebar: {
        hideable: true,
        autoCollapseCategories: false,
      },
    },
  } satisfies Preset.ThemeConfig,
};

export default config;
