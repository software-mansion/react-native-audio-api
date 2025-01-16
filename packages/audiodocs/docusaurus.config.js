// @ts-check
// Note: type annotations allow type checking and IDEs autocompletion

const lightCodeTheme = require('./src/theme/CodeBlock/highlighting-light.js');
const darkCodeTheme = require('./src/theme/CodeBlock/highlighting-dark.js');
// This runs in Node.js - Don't use client-side code here (browser APIs, JSX...)

const webpack = require('webpack');

const config = {
  title: 'React Native Audio API',
  favicon: 'img/favicon.ico',

  // Set the production url of your site here
  url: 'https://software-mansion-labs.github.io/',
  // Set the /<baseUrl>/ pathname under which your site is served
  // For GitHub pages deployment, it is often '/<projectName>/'
  baseUrl: '/react-native-audio-api/',

  // GitHub pages deployment config.
  // If you aren't using GitHub pages, you don't need these.
  organizationName: 'software-mansion-labs', // Usually your GitHub org/user name.
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
          routeBasePath: '/',
          breadcrumbs: false,
          sidebarCollapsible: false,
          sidebarPath: require.resolve('./sidebars.js'),
          editUrl:
            'https://github.com/software-mansion-labs/react-native-audio-api/edit/main/packages/audiodocs/docs',
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

  markdown: {
    mermaid: true,
  },
  themes: ['@docusaurus/theme-mermaid'],

  themeConfig: {
    // Replace with your project's social card
    // image: 'img/docusaurus-social-card.jpg',
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
          type: 'docSidebar',
          sidebarId: 'tutorialSidebar',
          position: 'left',
          label: '',
        },
        {
          'href':
            'https://github.com/software-mansion-labs/react-native-audio-api',
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
      theme: lightCodeTheme,
      darkTheme: darkCodeTheme,
    },
  },

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
                setImmediate: () => {},
              }),
            ],
            module: {
              rules: [
                {
                  test: /\.txt/,
                  type: 'asset/source',
                },
                {
                  test: /\.tsx?$/,
                },
              ],
            },
            resolve: {
              alias: { 'react-native$': 'react-native-web' },
              extensions: ['.web.js', '...'],
            },
          };
        },
      };
    },
  ],
};

module.exports = config;
