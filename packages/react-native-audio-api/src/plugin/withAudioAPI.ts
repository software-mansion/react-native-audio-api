import {
  AndroidConfig,
  createRunOncePlugin,
  ConfigPlugin,
  withInfoPlist,
  withAndroidManifest,
} from '@expo/config-plugins';
const pkg = require('react-native-audio-api/package.json');

interface Options {
  iosMicrophonePermission?: string;
  iosBackgroundMode: boolean;
  androidPermissions: string[];
  androidForegroundService: boolean;
  androidFSTypes: string[];
}

const withDefaultOptions = (options: Partial<Options>): Options => {
  return {
    iosBackgroundMode: true,
    androidPermissions: [
      'android.permission.FOREGROUND_SERVICE',
      'android.permission.FOREGROUND_SERVICE_MEDIA_PLAYBACK',
    ],
    androidForegroundService: true,
    androidFSTypes: ['mediaPlayback'],
    ...options,
  };
};

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

const withIosMicrophonePermission: ConfigPlugin<Options> = (
  config,
  { iosMicrophonePermission }
) => {
  return withInfoPlist(config, (iosConfig) => {
    iosConfig.modResults.NSMicrophoneUsageDescription = iosMicrophonePermission;
    return iosConfig;
  });
};

const withAndroidPermissions: ConfigPlugin<Options> = (
  config,
  { androidPermissions }: Options
) => {
  return AndroidConfig.Permissions.withPermissions(config, androidPermissions);
};

const withForegroundService: ConfigPlugin<Options> = (
  config,
  { androidFSTypes }: Options
) => {
  return withAndroidManifest(config, (mod) => {
    const manifest = mod.modResults;
    const mainApplication =
      AndroidConfig.Manifest.getMainApplicationOrThrow(manifest);

    const SFTypes = androidFSTypes.join('|');

    const serviceElement = {
      $: {
        'android:name':
          'com.swmansion.audioapi.system.MediaNotificationManager$AudioForegroundService',
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

const withAudioAPI: ConfigPlugin<Options> = (config, optionsIn) => {
  const options = withDefaultOptions(optionsIn ?? {});

  if (options.iosBackgroundMode) {
    config = withBackgroundAudio(config);
  }

  config = withAndroidPermissions(config, options);

  if (options.androidForegroundService) {
    config = withForegroundService(config, options);
  }

  if (options.iosMicrophonePermission) {
    config = withIosMicrophonePermission(config, options);
  }

  return config;
};

export default createRunOncePlugin(withAudioAPI, pkg.name, pkg.version);
