import {
  AndroidConfig,
  createRunOncePlugin,
  ConfigPlugin,
  withInfoPlist,
} from '@expo/config-plugins';

const pkg = require('react-native-audio-api/package.json');

const withBackgroundAudio: ConfigPlugin = (config) => {
  return withInfoPlist(config, (iosConfig) => {
    if (iosConfig.modResults.UIBackgroundModes) {
      iosConfig.modResults.UIBackgroundModes = [
        ...Array.from(
          new Set([...iosConfig.modResults.UIBackgroundModes, 'audio'])
        ),
      ];
    } else {
      iosConfig.modResults.UIBackgroundModes = ['audio'];
    }
    return iosConfig;
  });
};

const withAndroidPermissions: ConfigPlugin = (config) => {
  return AndroidConfig.Permissions.withPermissions(config, [
    'android.permission.FOREGROUND_SERVICE',
    'android.permission.FOREGROUND_SERVICE_MEDIA_PLAYBACK',
  ]);
};

const withAudioAPI: ConfigPlugin = (config) => {
  config = withBackgroundAudio(config);
  config = withAndroidPermissions(config);
  return config;
};

export default createRunOncePlugin(withAudioAPI, pkg.name, pkg.version);
