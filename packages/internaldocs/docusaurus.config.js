// @ts-check

const lightCodeTheme = require('./src/theme/CodeBlock/highlighting-light.js');
const darkCodeTheme = require('./src/theme/CodeBlock/highlighting-dark.js');

// eslint-disable-next-line import/first
import remarkMath from 'remark-math';
// eslint-disable-next-line import/first
import rehypeKatex from 'rehype-katex';

const config = {
  title: 'React Native Audio API — Internals',
  favicon: 'img/favicon.ico',

  url: 'http://localhost:3000',
  baseUrl: '/',

  organizationName: 'software-mansion',
  projectName: 'react-native-audio-api',

  onBrokenLinks: 'throw',
  onBrokenMarkdownLinks: 'throw',
  onBrokenAnchors: 'throw',

  i18n: {
    defaultLocale: 'en',
    locales: ['en'],
  },

  presets: [
    [
      'classic',
      {
        docs: {
          routeBasePath: '/',
          breadcrumbs: false,
          sidebarCollapsible: false,
          sidebarPath: require.resolve('./sidebars.js'),
          remarkPlugins: [remarkMath],
          rehypePlugins: [rehypeKatex],
        },
        blog: false,
        theme: {
          customCss: './src/css/custom.css',
        },
      },
    ],
    [require.resolve('@swmansion/t-rex-ui/preset'), { llms: false }],
  ],

  plugins: [
    async function compileTrexUiThemeJsx() {
      const path = require('path');
      const trexThemeDir = path.join(
        path.dirname(require.resolve('@swmansion/t-rex-ui/preset')),
        'theme'
      );

      return {
        name: 'internaldocs-trex-theme-jsx',
        configureWebpack() {
          const webpack = require('webpack');
          return {
            plugins: [
              new webpack.ProvidePlugin({
                React: 'react',
              }),
            ],
            module: {
              rules: [
                {
                  test: /\.(js|jsx)$/,
                  include: [trexThemeDir],
                  use: {
                    loader: 'babel-loader',
                    options: {
                      presets: [
                        ['@babel/preset-react', { runtime: 'automatic' }],
                      ],
                    },
                  },
                },
              ],
            },
          };
        },
      };
    },
  ],

  stylesheets: [
    {
      href: 'https://cdn.jsdelivr.net/npm/katex@0.13.24/dist/katex.min.css',
      type: 'text/css',
      integrity:
        'sha384-odtC+0UGzzFL/6PNoE8rX/SPcQDXBJ+uRepguP4QkPCm2LBxH3FA3y+fKSiJ+AmM',
      crossorigin: 'anonymous',
    },
  ],

  clientModules: [require.resolve('./src/clientModules/injectSidebarTags.js')],

  markdown: {
    mermaid: true,
  },
  themes: ['@docusaurus/theme-mermaid'],

  themeConfig: {
    navbar: {
      hideOnScroll: true,
      title: 'Internals',
      logo: {
        alt: 'react-native-audio-api logo',
        src: 'img/logo-hero.svg',
        srcDark: 'img/logo-hero.svg',
      },
      items: [],
    },
    footer: {
      links: [],
      copyright: 'Local internal documentation. Not published.',
    },
    prism: {
      additionalLanguages: ['bash', 'cmake'],
      theme: lightCodeTheme,
      darkTheme: darkCodeTheme,
    },
    // t-rex-ui still mounts Algolia hooks; values are unused (local site, no search).
    algolia: {
      appId: 'local',
      apiKey: 'local',
      indexName: 'local',
      externalUrlRegex: '^$',
    },
  },
};

module.exports = config;
