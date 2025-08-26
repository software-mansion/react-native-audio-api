module.exports = function (api) {
  api.cache(false);
  return {
    presets: ['module:@react-native/babel-preset'],
    plugins: [
      'module:react-native-dotenv',
      'react-native-worklets/plugin',
    ],
  };
};
