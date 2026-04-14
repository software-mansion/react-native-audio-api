module.exports = {
  presets: [require.resolve('@docusaurus/core/lib/babel/preset')],
  plugins: [
          '@babel/plugin-proposal-export-namespace-from',
          'react-native-worklets/plugin',
      ],
};
