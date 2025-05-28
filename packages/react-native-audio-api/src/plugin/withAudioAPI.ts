import {
  AndroidConfig,
  createRunOncePlugin,
  ConfigPlugin,
  withInfoPlist,
  withAndroidManifest,
} from '@expo/config-plugins';

const pkg = require('react-native-audio-api/package.json');

const withBackgroundAudio: ConfigPlugin = (config) => {
  return withInfoPlist(config, (iosConfig) => {
    iosConfig.modResults.UIBackgroundModes = [
      ...Array.from(
        new Set([...(iosConfig.modResults.UIBackgroundModes ?? []), 'audio'])
      ),
    ];

    return iosConfig;
  });
};

const withAndroidPermissions: ConfigPlugin<{
  androidPermissions: string[];
}> = (config, { androidPermissions }) => {
  return AndroidConfig.Permissions.withPermissions(config, androidPermissions);
};

const withForegroundService: ConfigPlugin<{
  androidFSTypes: string[];
}> = (config, { androidFSTypes }) => {
  return withAndroidManifest(config, (mod) => {
    const manifest = mod.modResults;
    const mainApplication =
      AndroidConfig.Manifest.getMainApplicationOrThrow(manifest);

    const SFTypes = androidFSTypes.join('|');

    const serviceElement = {
      $: {
        'android:name':
          'com.swmansion.audioapi.system.MediaNotificationManager$NotificationService',
        'android:stopWithTask': 'true',
        'android:foregroundServiceType': SFTypes,
      },
      intentFilter: [],
    };

    if (!mainApplication.service) {
      mainApplication.service = [];
    }

    mainApplication.service.push(serviceElement);

    return mod;
  });
};

const withAudioAPI: ConfigPlugin<{
  iosBackgroundMode?: boolean;
  androidPermissions?: string[];
  androidForegroundService?: boolean;
  androidFSTypes?: string[];
}> = (
  config,
  {
    iosBackgroundMode = true,
    androidPermissions = [],
    androidForegroundService = true,
    androidFSTypes = [],
  } = {}
) => {
  if (iosBackgroundMode) {
    config = withBackgroundAudio(config);
  }

  config = withAndroidPermissions(config, {
    androidPermissions,
  });

  if (androidForegroundService) {
    config = withForegroundService(config, {
      androidFSTypes,
    });
  }

  return config;
};

export default createRunOncePlugin(withAudioAPI, pkg.name, pkg.version);
