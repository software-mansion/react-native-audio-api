import {
  AudioManager,
  AudioRecorder,
  type FileInfo,
} from 'react-native-audio-api';

import { activateRecordingSession } from './audioSessions';
import {
  RECORDING_CALLBACK_TIMEOUT_MS,
  SHORT_RECORDING_MS,
  TORTURE_CHURN_COUNT,
  TORTURE_CYCLE_COUNT,
  TORTURE_DURATION_WINDOW_MS,
  TORTURE_STOP_RACE_COUNT,
  TORTURE_TOGGLE_COUNT,
} from './constants';
import { sleep, waitForCondition } from './helpers';
import type StressResourceOwner from './StressResourceOwner';

/**
 * What the torture steps need from the stress screen, bound to the scenario's
 * step list.
 */
export interface RecorderTortureEnv {
  resources: StressResourceOwner;
  runStep: (
    id: string,
    message: string,
    action: () => Promise<void>
  ) => Promise<void>;
  addInfoStep: (id: string, message: string, details?: string) => void;
}

/**
 * How far the duration may move while paused: one callback that was already on
 * its way.
 */
const PAUSED_DURATION_TOLERANCE_SECONDS = 0.1;

function check(condition: boolean, message: string): asserts condition {
  if (!condition) {
    throw new Error(message);
  }
}

/** Resolves with the rejection message; throws when @p promise resolves instead. */
async function rejectionOf(
  promise: Promise<unknown>,
  call: string
): Promise<string> {
  try {
    await promise;
  } catch (error) {
    return error instanceof Error ? error.message : String(error);
  }
  throw new Error(`${call} resolved, but it should have been refused`);
}

/**
 * Drives one recorder through misuse, unawaited bursts, races and mid-session
 * output changes, checking after each that it still records and reports a
 * consistent state. Expects an idle recorder with file output enabled, and
 * leaves it that way.
 */
export async function runRecorderTorture({
  resources,
  runStep,
  addInfoStep,
}: RecorderTortureEnv): Promise<void> {
  const recorder = () => resources.getReadyResources().recorder;

  const waitForMoreCallbacks = async (count: number) => {
    const target = resources.callbackCount + count;
    await waitForCondition(
      () => resources.callbackCount >= target,
      RECORDING_CALLBACK_TIMEOUT_MS,
      `Recorder did not deliver ${count} more callback buffer(s)`
    );
  };

  const checkNoRecorderError = () => {
    check(
      resources.lastRecorderError === null,
      `Recorder reported an error: ${resources.lastRecorderError}`
    );
  };

  await runStep(
    'idle-calls',
    'Calls on an idle recorder are refused or ignored',
    async () => {
      const idle = recorder();
      const stopError = await rejectionOf(idle.stop(), 'stop() while idle');
      const resumeError = await rejectionOf(
        idle.resume(),
        'resume() while idle'
      );
      await idle.pause();

      check(
        !idle.isRecording() && !idle.isPaused(),
        'An idle recorder reports recording or paused'
      );
      check(
        idle.getCurrentDuration() === 0,
        `An idle recorder reports a duration of ${idle.getCurrentDuration()}s`
      );
      addInfoStep(
        'idle-call-refusals',
        'Refusals',
        `stop: ${stopError}; resume: ${resumeError}`
      );
    }
  );

  await runStep(
    'unawaited-burst',
    'Fire start, pause, resume, pause, resume and stop without awaiting',
    async () => {
      await activateRecordingSession();
      resources.configureRecorderTap();
      const burst = recorder();

      const [, , , , , info] = await Promise.all([
        burst.start(),
        burst.pause(),
        burst.resume(),
        burst.pause(),
        burst.resume(),
        burst.stop(),
      ]);

      check(
        info.paths.length === 1,
        `stop() returned ${info.paths.length} file(s)`
      );
      check(
        !burst.isRecording() && !burst.isPaused(),
        'The recorder is not idle after the burst'
      );
      check(
        burst.getCurrentDuration() === 0,
        `getCurrentDuration() is ${burst.getCurrentDuration()}s after stop()`
      );
      check(
        !AudioRecorder.isRecordingOngoing(),
        'isRecordingOngoing() is still true after stop()'
      );
    }
  );

  await runStep(
    'misuse-mid-session',
    'Start again, enable file output, and start a second recorder mid-session',
    async () => {
      const active = recorder();
      await active.start();
      await waitForMoreCallbacks(1);

      const restartError = await rejectionOf(
        active.start(),
        'start() while recording'
      );

      let enableError: string | null = null;
      try {
        resources.enableFileOutput();
      } catch (error) {
        enableError = error instanceof Error ? error.message : String(error);
      }
      check(
        enableError !== null,
        'enableFileOutput() was accepted mid-session'
      );

      const intruder = new AudioRecorder();
      let intruderError: string;
      try {
        intruderError = await rejectionOf(
          intruder.start(),
          'start() of a second recorder'
        );
      } finally {
        if (intruder.isRecording() || intruder.isPaused()) {
          await intruder.stop().catch(() => undefined);
        }
      }

      check(
        active.isRecording(),
        'The recording stopped after the refused calls'
      );
      await waitForMoreCallbacks(2);
      addInfoStep(
        'mid-session-refusals',
        'Refusals',
        `start: ${restartError}; enableFileOutput: ${enableError}; second recorder: ${intruderError}`
      );
    }
  );

  await runStep(
    'pause-resume-storm',
    `Toggle pause and resume ${TORTURE_TOGGLE_COUNT} times without awaiting`,
    async () => {
      const toggled = recorder();
      const toggles: Promise<void>[] = [];
      for (let index = 0; index < TORTURE_TOGGLE_COUNT; index += 1) {
        toggles.push(toggled.pause(), toggled.resume());
      }
      await Promise.all(toggles);

      check(
        toggled.isRecording() && !toggled.isPaused(),
        'The recorder is not recording after the last resume()'
      );
      await waitForMoreCallbacks(2);
    }
  );

  await runStep(
    'duration',
    'getCurrentDuration() grows while recording and holds while paused',
    async () => {
      const timed = recorder();
      const atStart = timed.getCurrentDuration();
      await sleep(TORTURE_DURATION_WINDOW_MS);
      const afterRecording = timed.getCurrentDuration();
      check(
        afterRecording > atStart,
        `Duration did not grow while recording (${atStart}s -> ${afterRecording}s)`
      );

      await timed.pause();
      const atPause = timed.getCurrentDuration();
      await sleep(TORTURE_DURATION_WINDOW_MS);
      const whilePaused = timed.getCurrentDuration();
      check(
        whilePaused - atPause <= PAUSED_DURATION_TOLERANCE_SECONDS,
        `Duration kept growing while paused (${atPause}s -> ${whilePaused}s)`
      );

      await timed.resume();
      await sleep(TORTURE_DURATION_WINDOW_MS);
      const afterResume = timed.getCurrentDuration();
      check(
        afterResume > whilePaused,
        `Duration did not grow after resume() (${whilePaused}s -> ${afterResume}s)`
      );
      addInfoStep(
        'durations',
        'Durations',
        [atStart, afterRecording, atPause, whilePaused, afterResume]
          .map((seconds) => `${seconds.toFixed(3)}s`)
          .join(' -> ')
      );
    }
  );

  await runStep(
    'callback-churn',
    `Replace the audio callback ${TORTURE_CHURN_COUNT} times mid-session`,
    async () => {
      for (let index = 0; index < TORTURE_CHURN_COUNT; index += 1) {
        resources.configureRecorderTap();
        if (index % 2 === 1) {
          recorder().clearOnAudioReady();
        }
      }
      resources.configureRecorderTap();

      await waitForMoreCallbacks(2);
      check(
        recorder().isRecording(),
        'The recording stopped during callback churn'
      );
      checkNoRecorderError();
    }
  );

  await runStep(
    'adapter-churn',
    `Connect and disconnect the recorder ${TORTURE_CHURN_COUNT} times mid-session`,
    async () => {
      const { context } = resources.getReadyResources();
      if (context.state === 'suspended') {
        await context.resume();
      }

      const silentSink = context.createGain();
      silentSink.gain.value = 0;
      silentSink.connect(context.destination);

      const connected = recorder();
      try {
        for (let index = 0; index < TORTURE_CHURN_COUNT; index += 1) {
          connected.connect(context, silentSink);
          connected.disconnect();
        }
        connected.connect(context, silentSink);
        await waitForMoreCallbacks(2);
      } finally {
        connected.disconnect();
        silentSink.disconnect();
      }

      check(
        connected.isRecording(),
        'The recording stopped during adapter churn'
      );
      checkNoRecorderError();
    }
  );

  await runStep(
    'disable-file-mid-session',
    'Disable file output mid-session; recording goes on without a file',
    async () => {
      const fileless = recorder();
      await fileless.disableFileOutput();

      const atDisable = fileless.getCurrentDuration();
      await waitForMoreCallbacks(2);
      check(
        fileless.getCurrentDuration() > atDisable,
        'Duration stopped growing once the file was closed'
      );

      const info = await fileless.stop();
      check(
        info.paths.length === 0,
        `stop() returned ${info.paths.length} file(s) after file output was disabled`
      );

      resources.enableFileOutput();
      checkNoRecorderError();
    }
  );

  await runStep(
    'stop-race',
    `Race ${TORTURE_STOP_RACE_COUNT} stop() calls; one wins and its file is consumable once`,
    async () => {
      const raced = recorder();
      AudioRecorder.consumeLastRecordingResult();
      await raced.start();
      await waitForMoreCallbacks(1);

      const outcomes = await Promise.allSettled(
        Array.from({ length: TORTURE_STOP_RACE_COUNT }, () => raced.stop())
      );
      const winners = outcomes.filter(
        (outcome): outcome is PromiseFulfilledResult<FileInfo> =>
          outcome.status === 'fulfilled'
      );
      check(
        winners.length === 1,
        `${winners.length} of ${TORTURE_STOP_RACE_COUNT} stop() calls resolved`
      );

      const [winner] = winners;
      check(
        winner.value.paths.length === 1,
        `The winning stop() returned ${winner.value.paths.length} file(s)`
      );

      const kept = AudioRecorder.consumeLastRecordingResult();
      check(
        kept !== null && kept.paths[0] === winner.value.paths[0],
        `consumeLastRecordingResult() returned ${JSON.stringify(kept?.paths ?? null)}, expected ${winner.value.paths[0]}`
      );
      check(
        AudioRecorder.consumeLastRecordingResult() === null,
        'consumeLastRecordingResult() returned the recording a second time'
      );
    }
  );

  await runStep(
    'start-stop-cycles',
    `Start and stop ${TORTURE_CYCLE_COUNT} times back to back, then decode the last file`,
    async () => {
      const cycled = recorder();
      for (let cycle = 1; cycle <= TORTURE_CYCLE_COUNT; cycle += 1) {
        await cycled.start();
        await waitForMoreCallbacks(1);
        const info = await cycled.stop();
        check(
          info.paths.length === 1,
          `Cycle ${cycle}: stop() returned ${info.paths.length} file(s)`
        );
      }

      await cycled.start();
      await waitForMoreCallbacks(1);
      await sleep(SHORT_RECORDING_MS);
      const capture = await resources.stopRecordingAndDecode();

      checkNoRecorderError();
      addInfoStep(
        'last-cycle-file',
        'Last cycle file',
        `${capture.path} (${capture.fileDurationSeconds.toFixed(2)}s)`
      );
      await AudioManager.setAudioSessionActivity(false);
    }
  );
}
