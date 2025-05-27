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
  androidFSPermissions: string[];
}> = (config, { androidFSPermissions }) => {
  return AndroidConfig.Permissions.withPermissions(
    config,
    androidFSPermissions
  );
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
  androidForegroundService?: boolean;
  androidFSPermissions?: string[];
  androidFSTypes?: string[];
}> = (
  config,
  {
    iosBackgroundMode = true,
    androidForegroundService = true,
    androidFSPermissions = [],
    androidFSTypes = [],
  } = {}
) => {
  if (iosBackgroundMode) {
    config = withBackgroundAudio(config);
  }
  if (androidForegroundService) {
    config = withAndroidPermissions(config, {
      androidFSPermissions,
    });
    config = withForegroundService(config, {
      androidFSTypes,
    });
  }

  return config;
};

export default createRunOncePlugin(withAudioAPI, pkg.name, pkg.version);
