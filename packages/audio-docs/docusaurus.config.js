// @ts-check
// Note: type annotations allow type checking and IDEs autocompletion

const lightCodeTheme = require('./src/theme/CodeBlock/highlighting-light.js');
const darkCodeTheme = require('./src/theme/CodeBlock/highlighting-dark.js');

const webpack = require('webpack');

/** @type {import('@docusaurus/types').Config} */
const config = {
  title: 'React Native Audio Api',
  favicon: 'img/favicon.ico',

  // Set the production url of your site here
  url: 'https://docs.swmansion.com',

  // Change this to /react-native-audio-api/ when deploying to GitHub pages
  baseUrl: '/react-native-audio-api/',

  // GitHub pages deployment config.
  // If you aren't using GitHub pages, you don't need these.
  organizationName: 'software-mansion-labs', // Usually your GitHub org/user name.
  projectName: 'react-native-audio-api', // Usually your repo name.

  onBrokenLinks: 'throw',
  onBrokenMarkdownLinks: 'throw',

  // Even if you don't use internalization, you can use this field to set useful
  // metadata like html lang. For example, if your site is Chinese, you may want
  // to replace "en" with "zh-Hans".
  i18n: {
    defaultLocale: 'en',
    locales: ['en'],
  },

  presets: [
    [
      '@docusaurus/preset-classic',
      /** @type {import('@docusaurus/preset-classic').Options} */
      ({
        docs: {
          breadcrumbs: false,
          sidebarPath: require.resolve('./sidebars.js'),
          sidebarCollapsible: false,
          editUrl:
            'https://github.com/software-mansion-labs/react-native-audio-api/edit/main/packages/audio-docs',
          lastVersion: 'current', // <- this makes 3.x docs as default
          versions: {
            current: {
              label: '0.2',
            },
          },
        },
        theme: {
          customCss: require.resolve('./src/css/index.css'),
        },
      }),
    ],
  ],
  themeConfig:
    /** @type {import('@docusaurus/preset-classic').ThemeConfig} */
    ({
      metadata: [
        { name: 'og:image:width', content: '1200' },
        { name: 'og:image:height', content: '630' },
      ],
      navbar: {
        title: 'React Native Audio Api',
        hideOnScroll: true,
        logo: {
          alt: 'React Native Audio Api',
          src: 'img/logo.svg',
          srcDark: 'img/logo-dark.svg',
        },
        items: [
          {
            to: 'docs/GettingStarted/getting-started',
            activeBasePath: 'docs',
            label: 'Docs',
            position: 'left',
          },
          {
            type: 'docsVersionDropdown',
            position: 'right',
            dropdownActiveClassDisabled: true,
          },
          {
            href: 'https://github.com/software-mansion-labs/react-native-audio-api/',
            position: 'right',
            className: 'header-github',
            'aria-label': 'GitHub repository',
          },
        ],
      },
      announcementBar: {
        content: ' ',
        backgroundColor: '#03c',
        textColor: '#fff',
      },
      footer: {
        style: 'light',
        links: [],
        copyright:
          'All trademarks and copyrights belong to their respective owners.',
      },
      prism: {
        theme: lightCodeTheme,
        darkTheme: darkCodeTheme,
      },
    }),
  plugins: [
    ...[
      process.env.NODE_ENV === 'production' && '@docusaurus/plugin-debug',
    ].filter(Boolean),
    async function swmDocusaurusPlugin(_context, _options) {
      return {
        name: 'react-native-swm/docusaurus-plugin',
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
