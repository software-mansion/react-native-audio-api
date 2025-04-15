import {
  AndroidConfig,
  createRunOncePlugin,
  ConfigPlugin,
  withInfoPlist,
  // withAndroidManifest,
  // withMainApplication,
} from '@expo/config-plugins';
// import { ExpoConfig } from '@expo/config-types';

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

// const addMediaNotificationService = (manifest: ExpoConfig) => {
//   const service = {
//     $: {
//       'android:name': 'system.MediaNotificationManager$NotificationService',
//       'android:stopWithTask': 'true',
//       'android:foregroundServiceType': 'mediaPlayback|dataSync',
//     },
//   };

//   const mainApplication =
//     AndroidConfig.Manifest.getMainApplicationOrThrow(manifest);

//   if (!mainApplication.service) {
//     mainApplication.service = [];
//   }

//   mainApplication.service.push({ $: service.$ });

//   return manifest;
// };

// const withMediaNotificationService: ConfigPlugin = (config) => {
//   return withAndroidManifest(config, async (config) => {
//     config.modResults = addMediaNotificationService(config.modResults);
//     return config;
//   });
// };

const withAudioAPI: ConfigPlugin = (config) => {
  config = withBackgroundAudio(config);
  config = withAndroidPermissions(config);
  // config = withMediaNotificationService(config);
  return config;
};

export default createRunOncePlugin(withAudioAPI, pkg.name, pkg.version);
