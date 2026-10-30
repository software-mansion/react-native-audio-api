import { concatAudioFiles } from 'react-native-audio-api';

import { describeCall, Log, logAction } from './log';

const JOINED_TAKE_FILE_NAME = 'take-joined.m4a';

/**
 * The one playable file of a take: its only path, or the paths joined by
 * `concatAudioFiles`. Undefined when there is no file or joining failed.
 */
export async function joinTake(
  log: Log,
  paths: string[]
): Promise<string | undefined> {
  if (paths.length === 0) {
    log('take has no files');
    return undefined;
  }
  if (paths.length === 1) {
    log(`take ${paths[0]}`);
    return paths[0];
  }

  const outputPath = paths[0].replace(/[^/]+$/, JOINED_TAKE_FILE_NAME);
  const joinedPath = await logAction(
    log,
    describeCall('concatAudioFiles', paths, outputPath),
    () => concatAudioFiles(paths, outputPath)
  );
  if (joinedPath) {
    log(`take ${joinedPath}`);
  }
  return joinedPath;
}
