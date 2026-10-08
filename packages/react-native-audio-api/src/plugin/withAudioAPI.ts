import {
  AndroidConfig,
  ConfigPlugin,
  createRunOncePlugin,
  WarningAggregator,
  withAndroidManifest,
  withGradleProperties,
  withInfoPlist,
  withPodfile,
} from '@expo/config-plugins';
const pkg = require('react-native-audio-api/package.json');

interface Options {
  iosMicrophonePermission?: string;
  iosBackgroundMode: boolean;
  androidPermissions: string[];
  androidForegroundService: boolean;
  androidFSTypes: string[];
  androidFSStopWithTask: boolean;
  /** Links FFmpeg for remote URL streaming / HLS. Off by default. */
  enableFFmpeg: boolean;
  disableStaticExternalLibs: boolean;
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
    androidFSStopWithTask: true,
    enableFFmpeg: false,
    disableStaticExternalLibs: false,
    ...options,
  };
};

// TODO: remove this warning in some later versions after 1.0.0 is released
const REMOVED_DISABLE_FFMPEG_MESSAGE =
  '`disableFFmpeg` is ignored since 1.0.0: FFmpeg is off by default. Set `enableFFmpeg: true` to link it.';

const warnAboutRemovedOptions = (options: object) => {
  if ('disableFFmpeg' in options) {
    WarningAggregator.addWarningIOS(
      'react-native-audio-api',
      REMOVED_DISABLE_FFMPEG_MESSAGE
    );
    WarningAggregator.addWarningAndroid(
      'react-native-audio-api',
      REMOVED_DISABLE_FFMPEG_MESSAGE
    );
  }
};

/**
 * Keeps exactly one `key = value` line in the Podfile when `value` is set, and
 * removes any existing line for that key when it is not.
 */
const upsertPodfileEnvLine = (
  contents: string,
  key: string,
  value: string | null
): string => {
  const lineRegex = new RegExp(`^.*ENV\\['${key}'\\].*$`, 'gm');
  const line = value === null ? '' : `ENV['${key}'] = '${value}'`;

  if (contents.search(lineRegex) !== -1) {
    return contents.replace(lineRegex, line);
  }

  if (line === '') {
    return contents;
  }

  return contents.endsWith('\n')
    ? `${contents}${line}`
    : `${contents}\n${line}`;
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
  { androidFSTypes, androidFSStopWithTask }: Options
) => {
  return withAndroidManifest(config, (mod) => {
    const manifest = mod.modResults;
    const mainApplication =
      AndroidConfig.Manifest.getMainApplicationOrThrow(manifest);

    const SFTypes = androidFSTypes.join('|');

    const serviceElement = {
      $: {
        'android:name':
          'com.swmansion.audioapi.system.CentralizedForegroundService',
        'android:stopWithTask': String(androidFSStopWithTask),
        'android:foregroundServiceType': SFTypes,
      },
      intentFilter: [],
    };

    if (!mainApplication.service) {
      mainApplication.service = [];
    }

    const existingServiceIndex = mainApplication.service.findIndex((service) =>
      service.$['android:name'].includes(serviceElement.$['android:name'])
    );

    if (existingServiceIndex !== -1) {
      mainApplication.service[existingServiceIndex] = serviceElement;
      return mod;
    }

    mainApplication.service.push(serviceElement);
    return mod;
  });
};

const withFFmpegConfig: ConfigPlugin<Options> = (config, options) => {
  const iosConf = withPodfile(config, (mod) => {
    mod.modResults.contents = upsertPodfileEnvLine(
      mod.modResults.contents,
      'ENABLE_AUDIOAPI_FFMPEG',
      options.enableFFmpeg ? '1' : null
    );
    return mod;
  });

  const finalConf = withGradleProperties(iosConf, (mod) => {
    const gradleProperties = mod.modResults;

    const existingIndex = gradleProperties.findIndex(
      (prop) => prop.type === 'property' && prop.key === 'enableAudioapiFFmpeg'
    );
    if (existingIndex !== -1) {
      gradleProperties.splice(existingIndex, 1);
    }

    if (options.enableFFmpeg) {
      gradleProperties.push({
        type: 'property',
        key: 'enableAudioapiFFmpeg',
        value: 'true',
      });
    }

    return mod;
  });

  return finalConf;
};

const withStaticExternalLibsConfig: ConfigPlugin<Options> = (
  config,
  options
) => {
  const iosConf = withPodfile(config, (mod) => {
    let contents = mod.modResults.contents;
    const staticLibsRegex =
      /^.*ENV\['DISABLE_AUDIOAPI_STATIC_EXTERNAL_LIBS'\].*$/gm;
    const podfileString = options.disableStaticExternalLibs
      ? `ENV['DISABLE_AUDIOAPI_STATIC_EXTERNAL_LIBS'] = '1'`
      : '';
    // No existing setting
    if (contents.search(staticLibsRegex) === -1) {
      if (options.disableStaticExternalLibs) {
        if (contents.endsWith('\n')) {
          contents = `${contents}${podfileString}`;
        } else {
          contents = `${contents}\n${podfileString}`;
        }
        mod.modResults.contents = contents;
      }
    } else {
      // Existing setting found, will replace
      contents = contents.replace(staticLibsRegex, podfileString);
    }

    mod.modResults.contents = contents;
    return mod;
  });

  const finalConf = withGradleProperties(iosConf, (mod) => {
    const gradleProperties = mod.modResults;

    const existingIndex = gradleProperties.findIndex(
      (prop) =>
        prop.type === 'property' &&
        prop.key === 'disableAudioapiStaticExternalLibs'
    );
    if (existingIndex !== -1) {
      gradleProperties.splice(existingIndex, 1);
    } else if (!options.disableStaticExternalLibs) {
      // No existing setting and static external libs are enabled, do nothing.
      return mod;
    }

    if (options.disableStaticExternalLibs) {
      gradleProperties.push({
        type: 'property',
        key: 'disableAudioapiStaticExternalLibs',
        value: options.disableStaticExternalLibs ? 'true' : 'false',
      });
    }

    return mod;
  });

  return finalConf;
};

const withAudioAPI: ConfigPlugin<Options> = (config, optionsIn) => {
  warnAboutRemovedOptions(optionsIn ?? {});
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

  config = withFFmpegConfig(config, options);

  if (options.disableStaticExternalLibs !== undefined) {
    config = withStaticExternalLibsConfig(config, options);
  }

  return config;
};

export default createRunOncePlugin(withAudioAPI, pkg.name, pkg.version);
