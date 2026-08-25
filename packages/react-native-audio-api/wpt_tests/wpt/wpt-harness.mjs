import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { fork } from 'node:child_process';

import chalk from 'chalk';
import { program } from 'commander';

import {
  collectSelectedTestPaths,
  createConsoleReporter,
  createSequentialFilter,
  createWptEnvironment,
  getProfileAllowlist,
  normalizeTestPath,
  printSummary,
  runSequentialWpt,
} from './wpt-shared.mjs';
import {
  buildReport,
  formatCoverageMarkdown,
  updateDocsSection,
  WptResultsCollector,
  wrapReporter,
  writeReportFiles,
} from './wpt-results.mjs';

const harnessDir = path.dirname(fileURLToPath(import.meta.url));
const workerPath = path.join(harnessDir, 'wpt-worker.mjs');
const defaultJsonReportPath = path.join(harnessDir, '..', 'results', 'latest.json');
const defaultMarkdownReportPath = path.join(harnessDir, '..', 'results', 'latest.md');
const defaultDocsPath = path.join(
  harnessDir,
  '..',
  '..',
  '..',
  'audiodocs',
  'docs',
  'other',
  'web-audio-api-coverage.mdx'
);

// How long a worker may stay silent (no reporter event) before the parent
// assumes a native hang, kills it, and resumes past the stuck file. Generous
// against the 10s per-test testharness timeout — a file holds many tests.
const DEFAULT_INACTIVITY_TIMEOUT_S = 120;

// After a worker reports its batch done, its process.exit() still has to tear
// down native state. Give it this long to die on its own before SIGKILL — the
// parent already holds every result, so a hung teardown costs nothing.
const WORKER_EXIT_GRACE_MS = 10_000;

program
  .option('--list', 'List test files only')
  .option('--filter <regexp>', 'Additional regex filter for tests', '.*')
  .option('--include-crashtests', 'Include crashtests', false)
  .option(
    '--profile <name>',
    'Test selection profile: smoke (the-audio-api) or full (entire webaudio tree)',
    'smoke'
  )
  .option(
    '--batch-size <n>',
    'Files per worker process; 0 runs everything in this process without isolation',
    (value) => Number.parseInt(value, 10),
    25
  )
  .option(
    '--inactivity-timeout <seconds>',
    'Kill a worker that produces no events for this long and resume past the stuck file',
    (value) => Number.parseInt(value, 10),
    DEFAULT_INACTIVITY_TIMEOUT_S
  )
  .option(
    '--report-json <path>',
    'Write structured JSON results for markdown generation',
    null
  )
  .option(
    '--write-markdown <path>',
    'Write a wpt.fyi-style markdown report',
    null
  )
  .option(
    '--update-docs',
    'Replace the WPT summary block in the audiodocs Web Audio API coverage page',
    false
  )
  .option(
    '--allow-failures',
    'Exit 0 after a completed run even when assertions fail (still writes reports)',
    false
  );

program.parse(process.argv);
const options = program.opts();

const exitForFailures = failures => {
  if (failures > 0 && !options.allowFailures) {
    return 1;
  }
  return 0;
};

if (!['smoke', 'full'].includes(options.profile)) {
  console.error(chalk.red(`Invalid --profile value: ${options.profile}`));
  process.exit(1);
}

let numPass = 0;
let numFail = 0;
let timerStarted = false;
let summaryPrinted = false;
let activeWorker = null;
const resultsCollector = new WptResultsCollector();
const startedAt = Date.now();

const printHarnessSummary = () => {
  if (summaryPrinted) {
    return;
  }
  summaryPrinted = true;
  printSummary({ numPass, numFail, timerStarted });
};

const writeReports = () => {
  const reportJsonPath =
    options.reportJson === true ? defaultJsonReportPath : options.reportJson;
  const markdownPath =
    options.writeMarkdown === true ? defaultMarkdownReportPath : options.writeMarkdown;

  if (!reportJsonPath && !markdownPath && !options.updateDocs) {
    return;
  }

  const report = buildReport({
    collector: resultsCollector,
    profile: options.profile,
    filterRegexp: options.filter,
    includeCrashtests: options.includeCrashtests,
    profileAllowlist: getProfileAllowlist(options.profile),
    durationMs: Date.now() - startedAt,
  });

  writeReportFiles({
    report,
    jsonPath: reportJsonPath,
    markdownPath,
  });

  if (reportJsonPath) {
    console.log(chalk.dim(`Wrote JSON report: ${reportJsonPath}`));
  }
  if (markdownPath) {
    console.log(chalk.dim(`Wrote markdown report: ${markdownPath}`));
  }
  if (options.updateDocs) {
    updateDocsSection(defaultDocsPath, formatCoverageMarkdown(report));
    console.log(chalk.dim(`Updated docs summary: ${defaultDocsPath}`));
  }
};

const signalExitCode = {
  SIGHUP: 129,
  SIGINT: 130,
  SIGTERM: 143,
};

const handleSignal = signal => {
  console.error(chalk.yellow(`\nReceived ${signal}; printing partial summary.`));
  activeWorker?.kill('SIGKILL');
  printHarnessSummary();
  writeReports();
  // Interrupted runs always exit non-zero; --allow-failures only applies to completed runs.
  process.exit(signalExitCode[signal] ?? 1);
};

process.on('SIGHUP', () => handleSignal('SIGHUP'));
process.on('SIGINT', () => handleSignal('SIGINT'));
process.on('SIGTERM', () => handleSignal('SIGTERM'));

process.on('unhandledRejection', error => {
  const message = error instanceof Error ? error.stack || error.message : String(error);
  console.error(chalk.red(`Unhandled rejection during WPT run:\n${message}`));
});

process.on('uncaughtException', error => {
  const message = error instanceof Error ? error.stack || error.message : String(error);
  console.error(chalk.red(`Uncaught exception during WPT run:\n${message}`));
});

if (options.list) {
  for (const file of collectSelectedTestPaths({
    filterRegexp: options.filter,
    includeCrashtests: options.includeCrashtests,
    profile: options.profile,
  })) {
    console.log(file);
  }
  process.exit(0);
}

/**
 * Run one batch of files in a forked worker, forwarding its reporter events.
 *
 * @returns {Promise<{
 *   startedFiles: string[],
 *   outcome: 'done' | 'crashed' | 'timeout',
 *   fileFailures: number,
 * }>} `startedFiles` lists files the worker began, in order; on 'crashed' or
 * 'timeout' the last entry (if any) is the file that never finished.
 */
function runWorkerBatch(files, reporter) {
  return new Promise((resolve) => {
    const worker = fork(workerPath, [], {
      stdio: ['inherit', 'inherit', 'inherit', 'ipc'],
    });
    activeWorker = worker;

    const startedFiles = [];
    let fileFailures = 0;
    let doneReceived = false;
    let settled = false;
    let outcome = 'crashed';
    let inactivityTimer = null;
    let exitGraceTimer = null;

    const settle = () => {
      if (settled) {
        return;
      }
      settled = true;
      clearTimeout(inactivityTimer);
      clearTimeout(exitGraceTimer);
      activeWorker = null;
      resolve({ startedFiles, outcome, fileFailures });
    };

    const armInactivityTimer = () => {
      clearTimeout(inactivityTimer);
      if (options.inactivityTimeout <= 0) {
        return;
      }
      inactivityTimer = setTimeout(() => {
        outcome = 'timeout';
        worker.kill('SIGKILL');
      }, options.inactivityTimeout * 1000);
    };

    worker.on('message', (message) => {
      armInactivityTimer();
      switch (message.type) {
        case 'suite-start':
          startedFiles.push(normalizeTestPath(message.name));
          reporter.startSuite(message.name);
          break;
        case 'pass':
          reporter.pass(message.message);
          break;
        case 'fail':
          reporter.fail(message.message);
          break;
        case 'stack':
          reporter.reportStack(message.stack);
          break;
        case 'done':
          doneReceived = true;
          outcome = 'done';
          fileFailures = message.fileFailures ?? 0;
          clearTimeout(inactivityTimer);
          exitGraceTimer = setTimeout(() => {
            console.error(
              chalk.yellow(
                '[wpt] worker finished its batch but its native teardown hung; killed.'
              )
            );
            worker.kill('SIGKILL');
          }, WORKER_EXIT_GRACE_MS);
          break;
        default:
          break;
      }
    });

    worker.on('error', () => {
      worker.kill('SIGKILL');
    });

    worker.on('exit', () => {
      // 'done' already fixed the outcome; otherwise the child died mid-batch
      // (native crash) unless the watchdog set 'timeout' first.
      if (!doneReceived && outcome !== 'timeout') {
        outcome = 'crashed';
      }
      settle();
    });

    armInactivityTimer();
    worker.send({ files });
  });
}

/**
 * Run all selected files through short-lived worker processes.
 *
 * A worker that crashes or hangs costs exactly one file: it is recorded as
 * crashed and the remaining files of its batch are re-queued for a fresh
 * worker. The parent never loads the native module, so its own exit is
 * instant no matter what the audio engine's teardown does.
 */
async function runBatched(reporter) {
  const queue = collectSelectedTestPaths({
    filterRegexp: options.filter,
    includeCrashtests: options.includeCrashtests,
    profile: options.profile,
  });

  let totalFileFailures = 0;
  let crashedFiles = 0;

  while (queue.length > 0) {
    const batch = queue.splice(0, options.batchSize);
    const { startedFiles, outcome, fileFailures } = await runWorkerBatch(
      batch,
      reporter
    );
    totalFileFailures += fileFailures;

    if (outcome === 'done') {
      continue;
    }

    // The file that never finished: the last one started, or — when the worker
    // died before starting anything (e.g. the addon failed to load) — the first
    // of the batch, so the queue always shrinks and the run always terminates.
    const crashed =
      startedFiles.length > 0 ? startedFiles[startedFiles.length - 1] : batch[0];
    const label = outcome === 'timeout' ? 'hung (no events)' : 'crashed the worker';
    console.error(chalk.red(`\n  × ${crashed} ${label}; resuming after it.\n`));
    resultsCollector.markFileCrashed(crashed, outcome);
    crashedFiles += 1;

    const crashedIndex = batch.indexOf(crashed);
    queue.unshift(...batch.slice(crashedIndex + 1));
  }

  return { totalFileFailures, crashedFiles };
}

try {
  console.time('wpt-duration');
  timerStarted = true;

  const numPassRef = { value: 0 };
  const numFailRef = { value: 0 };
  const reporter = wrapReporter(
    createConsoleReporter({ numPassRef, numFailRef }),
    resultsCollector
  );

  let fileFailures = 0;
  let crashedFiles = 0;

  if (options.batchSize > 0) {
    ({ totalFileFailures: fileFailures, crashedFiles } =
      await runBatched(reporter));
  } else {
    // Legacy single-process mode: everything shares this process, including
    // whatever native teardown process.exit() runs into.
    createWptEnvironment();
    const filter = createSequentialFilter({
      filterRegexp: options.filter,
      includeCrashtests: options.includeCrashtests,
      listOnly: false,
      profile: options.profile,
    });
    fileFailures = await runSequentialWpt({ filter, reporter });
  }

  numPass = numPassRef.value;
  numFail = numFailRef.value;
  // A crashed file lost at least one test, and a file-level failure with no
  // recorded subtest failures (a file that errored before producing subtests)
  // must still fail the run.
  numFail += crashedFiles;
  if (fileFailures > 0 && numFail === 0) {
    numFail = fileFailures;
  }

  printHarnessSummary();
  writeReports();
  process.exit(exitForFailures(numFail));
} catch (error) {
  printHarnessSummary();
  writeReports();
  console.error(error.stack);
  process.exit(1);
}
