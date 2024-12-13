// @ts-check
// Note: type annotations allow type checking and IDEs autocompletion

const lightCodeTheme = require('./src/theme/CodeBlock/highlighting-light.js');
const darkCodeTheme = require('./src/theme/CodeBlock/highlighting-dark.js');
// This runs in Node.js - Don't use client-side code here (browser APIs, JSX...)

const config = {
  title: 'React Native Audio API',
  favicon: 'img/favicon.ico',

  // Set the production url of your site here
  url: 'https://dummy-docs.no.no',
  // Set the /<baseUrl>/ pathname under which your site is served
  // For GitHub pages deployment, it is often '/<projectName>/'
  baseUrl: '/',

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
        googleAnalytics: {
          trackingID: 'G-4BDHB978P1',
          anonymizeIP: true,
        },
      },
    ],
  ],

  markdown: {
    mermaid: true,
  },

  themeConfig: {
    // Replace with your project's social card
    // image: 'img/docusaurus-social-card.jpg',
    navbar: {
      hideOnScroll: true,
      // logo: {
      //   // alt: 'react-native-audio-api logo',
      //   src: 'img/logo.svg',
      //   srcDark: 'img/logo.svg',
      // },
      items: [
        {
          type: 'docSidebar',
          sidebarId: 'tutorialSidebar',
          position: 'left',
          label: 'Tutorial',
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
      style: 'dark',
      links: [],
      copyright: `All trademarks and copyrights belong to their respective owners.`,
    },
    prism: {
      theme: lightCodeTheme,
      darkTheme: darkCodeTheme,
    },
  },
};

module.exports = config;
