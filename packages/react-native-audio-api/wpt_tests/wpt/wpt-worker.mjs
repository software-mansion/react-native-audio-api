/**
 * Child process entry for batched WPT runs (see wpt-harness.mjs).
 *
 * Runs one batch of test files in-process and streams reporter events to the
 * parent over IPC. Keeping batches in short-lived children means native state
 * (audio threads, event-registry entries) accumulates only across a batch, a
 * native crash loses one file instead of the whole run, and the final
 * process.exit() of each child tears down a small heap — the parent survives
 * even if that teardown hangs.
 *
 * Protocol (child -> parent):
 *   { type: 'suite-start', name }   a test file began
 *   { type: 'pass', message }       one subtest passed
 *   { type: 'fail', message }       one subtest failed
 *   { type: 'stack', stack }        stack trace for the preceding failure
 *   { type: 'done', fileFailures }  batch finished; parent may kill us
 *
 * Parent -> child: a single { files: string[] } message starts the batch.
 */

import {
  createReporter,
  normalizeTestPath,
  runSequentialWpt,
} from './wpt-shared.mjs';

function send(message) {
  // The parent may already have killed us (e.g. its inactivity watchdog fired
  // while an event was in flight); losing that race is fine.
  try {
    process.send(message);
  } catch {
    // Channel closed — nothing left to report to.
  }
}

process.on('message', async ({ files }) => {
  const batch = new Set(files.map(normalizeTestPath));

  const reporter = createReporter({
    startSuite: (name) => send({ type: 'suite-start', name }),
    pass: (message) => send({ type: 'pass', message }),
    fail: (message) => send({ type: 'fail', message }),
    reportStack: (stack) => send({ type: 'stack', stack }),
  });

  let fileFailures = 0;
  try {
    fileFailures = await runSequentialWpt({
      filter: (name) => batch.has(normalizeTestPath(name)),
      reporter,
    });
  } catch (error) {
    send({ type: 'fail', message: `worker error: ${error.message}` });
    send({ type: 'stack', stack: error.stack ?? String(error) });
    fileFailures = Math.max(fileFailures, 1);
  }

  send({ type: 'done', fileFailures });
  // Native teardown (audio thread joins, registry destruction) happens inside
  // this exit. The parent holds every result already, waits briefly, and
  // SIGKILLs us if teardown hangs — the historical CI "exit code 129" mode.
  process.exit(0);
});
