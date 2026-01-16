module.exports = function (api) {
  api.cache(false);
  return {
    presets: ['module:@react-native/babel-preset'],
    plugins: [
      'react-native-worklets/plugin',
      [
        'module-resolver',
        {
          alias: {
            'common-app': '../common-app',
          },
          extensions: [
            '.js',
            '.jsx',
            '.ts',
            '.tsx',
            '.ios.js',
            '.android.js',
            '.json',
            '.ios.ts',
            '.android.ts',
            '.ios.tsx',
            '.android.tsx',
          ],
        },
      ],
    ],
  };
};
