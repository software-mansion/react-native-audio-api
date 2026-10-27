import { useCallback, useEffect, useState } from 'react';
import {
  AudioManager,
  AudioRecorder,
  concatAudioFiles,
  FileFormat,
  Result,
} from 'react-native-audio-api';

import { recorderOptions } from '../../singletons';
import { Log, logAction } from './log';
import {
  appendLevel,
  glyphForPeak,
  LEVEL_CALLBACK_OPTIONS,
  NO_BUFFERS,
  peakAmplitude,
} from './recordingLevel';
import { usePolled } from './usePolled';

const FILE_OPTIONS = {
  format: FileFormat.M4A,
  subDirectory: 'AudioIOPlayground',
  fileName: 'take',
};
const JOINED_TAKE_FILE_NAME = 'take-joined.m4a';

function unwrap<T>(result: Result<T>): T {
  if (result.status === 'error') {
    throw new Error(result.message);
  }
  return result;
}

function describeRecorder(recorder: AudioRecorder): string {
  const seconds = `${recorder.getCurrentDuration().toFixed(1)} s`;
  if (recorder.isPaused()) {
    return `paused ${seconds}`;
  }
  return recorder.isRecording() ? `recording ${seconds}` : 'idle';
}

async function joinTake(paths: string[]): Promise<string> {
  if (paths.length === 1) {
    return paths[0];
  }
  const outputPath = paths[0].replace(/[^/]+$/, JOINED_TAKE_FILE_NAME);
  return concatAudioFiles(paths, outputPath);
}

/** Owns the playground's own recorder; `onTake` receives each finished file. */
export function useRecorder(log: Log, onTake: (path: string) => void) {
  const [recorder] = useState(() => new AudioRecorder(recorderOptions));
  const [level, setLevel] = useState(NO_BUFFERS);
  const status = usePolled(
    useCallback(() => describeRecorder(recorder), [recorder])
  );

  useEffect(() => {
    if (AudioRecorder.isRecordingOngoing()) {
      log('recorder a recording from elsewhere is still ongoing');
    }
    logAction(log, 'recorder enable file output', () =>
      recorder.enableFileOutput(FILE_OPTIONS)
    );
    logAction(log, 'recorder listen', () => {
      unwrap(
        recorder.onAudioReady(LEVEL_CALLBACK_OPTIONS, ({ buffer }) => {
          const glyph = glyphForPeak(peakAmplitude(buffer.getChannelData(0)));
          setLevel((previous) => appendLevel(previous, glyph));
        })
      );
    });
    recorder.onError(({ message }) => log(`recorder error ${message}`));

    return () => {
      recorder.clearOnAudioReady();
      recorder.clearOnError();
      if (recorder.isRecording() || recorder.isPaused()) {
        logAction(log, 'recorder stop on leaving the screen', async () => {
          unwrap(await recorder.stop());
          AudioRecorder.consumeLastRecordingResult();
        });
      }
    };
  }, [log, recorder]);

  const start = useCallback(async () => {
    const permission = await AudioManager.requestRecordingPermissions();
    log(`recorder permission ${permission}`);
    setLevel(NO_BUFFERS);
    await logAction(log, 'recorder start', async () =>
      unwrap(await recorder.start())
    );
  }, [log, recorder]);

  const pause = useCallback(
    () => logAction(log, 'recorder pause', () => recorder.pause()),
    [log, recorder]
  );

  const resume = useCallback(
    () => logAction(log, 'recorder resume', () => recorder.resume()),
    [log, recorder]
  );

  const stop = useCallback(async () => {
    const info = await logAction(log, 'recorder stop', async () =>
      unwrap(await recorder.stop())
    );
    if (!info) {
      return;
    }
    // A successful stop also stashes its result for
    // consumeLastRecordingResult(). Drop it, or the Record demo loads this take
    // the next time it mounts. The cleanup above does the same.
    AudioRecorder.consumeLastRecordingResult();

    if (info.paths.length === 0) {
      log('recorder take has no files');
      return;
    }
    const path = await logAction(
      log,
      `recorder join ${info.paths.length} file(s)`,
      () => joinTake(info.paths)
    );
    if (path) {
      log(`recorder take ${path}`);
      onTake(path);
    }
  }, [log, recorder, onTake]);

  return { status, level, start, pause, resume, stop };
}
