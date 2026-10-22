import {
  AndroidConfig,
  ConfigPlugin,
  createRunOncePlugin,
  withAndroidManifest,
  withDangerousMod,
  withInfoPlist,
} from '@expo/config-plugins';
const fs = require('fs');
const path = require('path');
const pkg = require('react-native-audio-api/package.json');

interface Options {
  iosMicrophonePermission?: string;
  iosBackgroundMode: boolean;
  androidPermissions: string[];
  androidForegroundService: boolean;
  androidFSTypes: string[];
  androidFSStopWithTask: boolean;
  enableFFmpeg?: boolean;
  disableStaticExternalLibs?: boolean;
}

const BUILD_OPTION_KEYS = [
  'enableFFmpeg',
  'disableStaticExternalLibs',
] as const;

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

/**
 * Writes the given build options into the `react-native-audio-api` section of
 * the app's package.json, which is where CocoaPods, Swift Package Manager and
 * Gradle read them from.
 */
const writeBuildOptionsToPackageJson = (
  projectRoot: string,
  options: Options
) => {
  const packageJsonPath = path.join(projectRoot, 'package.json');
  const source: string = fs.readFileSync(packageJsonPath, 'utf8');
  const packageJson = JSON.parse(source);

  const section = { ...(packageJson[pkg.name] ?? {}) };
  for (const key of BUILD_OPTION_KEYS) {
    const value = options[key];
    if (value === undefined) {
      continue;
    }
    if (typeof value !== 'boolean') {
      throw new Error(
        `[react-native-audio-api] plugin option "${key}" must be true or false, got ${JSON.stringify(value)}.`
      );
    }
    section[key] = value;
  }
  packageJson[pkg.name] = section;

  const indent = source.match(/^[ \t]+/m)?.[0] ?? '  ';
  const trailingNewline = source.endsWith('\n') ? '\n' : '';
  const updated = JSON.stringify(packageJson, null, indent) + trailingNewline;
  if (updated !== source) {
    fs.writeFileSync(packageJsonPath, updated);
  }
};

// Registered on both platforms so a single-platform prebuild writes the file
// too; the second run finds the file already up to date.
const withBuildOptions: ConfigPlugin<Options> = (config, options) => {
  const isAnyBuildOptionGiven = BUILD_OPTION_KEYS.some(
    (key) => options[key] !== undefined
  );
  if (!isAnyBuildOptionGiven) {
    return config;
  }

  for (const platform of ['ios', 'android'] as const) {
    config = withDangerousMod(config, [
      platform,
      (mod) => {
        writeBuildOptionsToPackageJson(mod.modRequest.projectRoot, options);
        return mod;
      },
    ]);
  }
  return config;
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

  config = withBuildOptions(config, options);

  return config;
};

export default createRunOncePlugin(withAudioAPI, pkg.name, pkg.version);
