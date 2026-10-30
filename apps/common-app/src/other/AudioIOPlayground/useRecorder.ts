import { useCallback, useEffect, useState } from 'react';
import {
  AudioManager,
  AudioRecorder,
  AudioRecorderOptions,
  FileFormat,
  Result,
} from 'react-native-audio-api';

import { recorderOptions } from '../../singletons';
import { joinTake } from './joinTake';
import { describeCall, Log, logAction } from './log';
import {
  appendLevel,
  glyphForPeak,
  LEVEL_CALLBACK_OPTIONS,
  NO_BUFFERS,
  peakAmplitude,
} from './recordingLevel';
import { usePolled } from './usePolled';
import { useRecreatable } from './useRecreatable';

const FILE_OPTIONS = {
  format: FileFormat.M4A,
  subDirectory: 'AudioIOPlayground',
  fileName: 'take',
};
const CALLS = {
  permission: 'AudioManager.requestRecordingPermissions()',
  start: 'recorder.start()',
  pause: 'recorder.pause()',
  resume: 'recorder.resume()',
  stop: 'recorder.stop()',
  consume: 'AudioRecorder.consumeLastRecordingResult()',
};

export const RECORDER_CALLS = {
  status:
    'recorder.isPaused(), recorder.isRecording(), recorder.getCurrentDuration()',
  level: 'peak of each recorder.onAudioReady buffer',
  start: `${CALLS.permission}, then ${CALLS.start}`,
  pause: CALLS.pause,
  resume: CALLS.resume,
  stop: `${CALLS.stop}, then ${CALLS.consume}; concatAudioFiles for several files`,
  options: 'the options the current recorder was created with',
  recreate:
    'recorder = new AudioRecorder(<options above>); ' +
    `the previous recorder first runs ${CALLS.stop} if it holds a take`,
};

function createRecorder(options: AudioRecorderOptions): AudioRecorder {
  return new AudioRecorder(options);
}

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

/**
 * Owns the playground's own recorder; `onTake` receives each finished file. Its
 * options are fixed at construction, so `recreate` replaces it.
 */
export function useRecorder(log: Log, onTake: (path: string) => void) {
  const {
    instance: recorder,
    options,
    recreate: recreateRecorder,
  } = useRecreatable(createRecorder, recorderOptions);
  const [level, setLevel] = useState(NO_BUFFERS);
  const status = usePolled(
    useCallback(() => describeRecorder(recorder), [recorder])
  );

  useEffect(() => {
    log(describeCall('new AudioRecorder', options));
    log(
      `AudioRecorder.isRecordingOngoing() → ${AudioRecorder.isRecordingOngoing()}`
    );
    const fileOptionsAsWritten = {
      ...FILE_OPTIONS,
      format: `FileFormat.${FileFormat[FILE_OPTIONS.format]}`,
    };
    logAction(
      log,
      describeCall('recorder.enableFileOutput', fileOptionsAsWritten),
      () => recorder.enableFileOutput(FILE_OPTIONS)
    );
    logAction(
      log,
      `recorder.onAudioReady(${JSON.stringify(LEVEL_CALLBACK_OPTIONS)}, callback)`,
      () => {
        unwrap(
          recorder.onAudioReady(LEVEL_CALLBACK_OPTIONS, ({ buffer }) => {
            const peak = peakAmplitude(buffer.getChannelData(0));
            setLevel((previous) => appendLevel(previous, glyphForPeak(peak)));
          })
        );
      }
    );
    logAction(log, 'recorder.onError(callback)', () =>
      recorder.onError(({ message }) => log(`recorder.onError: ${message}`))
    );

    return () => {
      recorder.clearOnAudioReady();
      recorder.clearOnError();
      if (recorder.isRecording() || recorder.isPaused()) {
        logAction(log, `${CALLS.stop} on releasing the recorder`, async () => {
          unwrap(await recorder.stop());
          AudioRecorder.consumeLastRecordingResult();
        });
      }
    };
  }, [log, recorder, options]);

  const recreate = useCallback(
    (nextOptions: AudioRecorderOptions) => {
      setLevel(NO_BUFFERS);
      recreateRecorder(nextOptions);
    },
    [recreateRecorder]
  );

  const start = useCallback(async () => {
    const permission = await AudioManager.requestRecordingPermissions();
    log(`${CALLS.permission} → ${permission}`);
    setLevel(NO_BUFFERS);
    await logAction(log, CALLS.start, async () =>
      unwrap(await recorder.start())
    );
  }, [log, recorder]);

  const pause = useCallback(
    () => logAction(log, CALLS.pause, () => recorder.pause()),
    [log, recorder]
  );

  const resume = useCallback(
    () => logAction(log, CALLS.resume, () => recorder.resume()),
    [log, recorder]
  );

  const stop = useCallback(async () => {
    const info = await logAction(log, CALLS.stop, async () =>
      unwrap(await recorder.stop())
    );
    if (!info) {
      return;
    }
    // A successful stop also stashes its result here. Drop it, or the Record
    // demo loads this take the next time it mounts. The cleanup does the same.
    await logAction(log, CALLS.consume, () =>
      AudioRecorder.consumeLastRecordingResult()
    );

    const takePath = await joinTake(log, info.paths);
    if (takePath) {
      onTake(takePath);
    }
  }, [log, recorder, onTake]);

  return { status, level, options, start, pause, resume, stop, recreate };
}
