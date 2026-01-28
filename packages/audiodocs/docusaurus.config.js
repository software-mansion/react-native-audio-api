// @ts-check
// Note: type annotations allow type checking and IDEs autocompletion

const lightCodeTheme = require('./src/theme/CodeBlock/highlighting-light.js');
const darkCodeTheme = require('./src/theme/CodeBlock/highlighting-dark.js');

// eslint-disable-next-line import/first
import remarkMath from 'remark-math';
// eslint-disable-next-line import/first
import rehypeKatex from 'rehype-katex';
// This runs in Node.js - Don't use client-side code here (browser APIs, JSX...)

const webpack = require('webpack');

const config = {
  title: 'React Native Audio API',
  favicon: 'img/favicon.ico',

  // Set the production url of your site here
  url: 'https://docs.swmansion.com/',
  // Set the /<baseUrl>/ pathname under which your site is served
  // For GitHub pages deployment, it is often '/<projectName>/'
  baseUrl: '/react-native-audio-api/',

  // GitHub pages deployment config.
  // If you aren't using GitHub pages, you don't need these.
  organizationName: 'software-mansion', // Usually your GitHub org/user name.
  projectName: 'react-native-audio-api', // Usually your repo name.

  onBrokenLinks: 'throw',
  onBrokenMarkdownLinks: 'warn',

  // Even if you don't use internationalization, you can use this field to set
  // useful metadata like html lang. For example, if your site is Chinese, you
  // may want to replace "en" with "zh-Hans".
  i18n: {
    defaultLocale: 'en',
    locales: ['en'],
  },

  presets: [
    [
      'classic',
      {
        docs: {
          breadcrumbs: false,
          sidebarCollapsible: false,
          sidebarPath: require.resolve('./sidebars.js'),
          remarkPlugins: [remarkMath],
          rehypePlugins: [rehypeKatex],
          editUrl:
            'https://github.com/software-mansion/react-native-audio-api/edit/main/packages/audiodocs/docs',
        },
        blog: false,
        theme: {
          customCss: './src/css/custom.css',
        },
        gtag: {
          trackingID: 'G-4BDHB978P1',
          anonymizeIP: true,
        },
      },
    ],
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

  markdown: {
    mermaid: true,
  },
  themes: ['@docusaurus/theme-mermaid'],

  themeConfig: {
    image: '/img/og-image.png',

    navbar: {
      hideOnScroll: true,
      title: 'React Native Audio API',
      logo: {
        alt: 'react-native-audio-api logo',
        src: 'img/logo-hero.svg',
        srcDark: 'img/logo-hero.svg',
      },
      items: [
        {
          'href':
            'https://github.com/software-mansion/react-native-audio-api',
          'label': 'GitHub',
          'position': 'right',
          'aria-label': 'GitHub repository',
        },
      ],
    },
    footer: {
      links: [],
      copyright: `All trademarks and copyrights belong to their respective owners.`,
    },
    prism: {
      additionalLanguages: ['bash', 'cmake'],
      theme: lightCodeTheme,
      darkTheme: darkCodeTheme,
    },
    algolia: {
      appId: '7OKARNAQRP',
      apiKey: 'f06db3d3f64e619012f52f9fb3edf349',
      indexName: 'swmansion',
      askAi: {
        assistantId: 'Gy3lkaKLUCko',
        indexName: 'audio-api-ai',
        apiKey: 'f06db3d3f64e619012f52f9fb3edf349',
        appId: '7OKARNAQRP',
      },
    },
  },
    clientModules: [
    require.resolve('./src/wasm-loader.js'),
  ],

  plugins: [
    ...[
      process.env.NODE_ENV === 'production' && '@docusaurus/plugin-debug',
    ].filter(Boolean),
    async function docusaurusPlugin() {
      return {
        name: 'react-native-audio-api/docusaurus-plugin',
        // @ts-ignore
        configureWebpack(_config, isServer, _utils) {
          const processMock = !isServer ? { process: { env: {} } } : {};

          const raf = require('raf');
          raf.polyfill();

          return {
            mergeStrategy: {
              'resolve.extensions': 'prepend',
            },
            plugins: [
              new webpack.DefinePlugin({
                ...processMock,
                __DEV__: 'false',
              }),
              // Provide React automatically where library code expects global React
              new webpack.ProvidePlugin({
                React: 'react',
              }),
            ],
            module: {
              rules: [
                {
                  test: /\.txt$/,
                  type: 'asset/source',
                },
                {
                  test: /\.tsx?$/,
                  use: 'babel-loader',
                },
                {
                  test: /\.(js|jsx)$/,
                  use: {
                    loader: 'babel-loader',
                    options: {
                      presets: [
                        '@babel/preset-react',
                        { plugins: ['@babel/plugin-proposal-class-properties'] },
                      ],
                    },
                  },
                },
              ],
            },
            resolve: {
              alias: { 'react-native$': 'react-native-web' },
              extensions: ['.web.js', '.js', '...'],
            },
          };
        },
      };
    },
  ],
};

module.exports = config;
