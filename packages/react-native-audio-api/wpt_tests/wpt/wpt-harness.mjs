import chalk from 'chalk';
import { program } from 'commander';

import {
  collectSelectedTestPaths,
  createConsoleReporter,
  createSequentialFilter,
  createWptEnvironment,
  printSummary,
  resolveJobsOption,
} from './wpt-shared.mjs';
import {
  createParallelConsoleReporter,
  runSequentialWpt,
  WptParallelRunner,
} from './WptParallelRunner.mjs';

program
  .option('--list', 'List test files only')
  .option('--filter <regexp>', 'Additional regex filter for tests', '.*')
  .option('--include-crashtests', 'Include crashtests', false)
  .option(
    '--jobs <n|auto>',
    'Run test classes in parallel worker processes (1 = sequential)',
    '1'
  );

program.parse(process.argv);
const options = program.opts();

let numPass = 0;
let numFail = 0;
let timerStarted = false;
let summaryPrinted = false;
/** @type {import('./WptParallelRunner.mjs').WptParallelRunner | null} */
let parallelRunner = null;

const printHarnessSummary = () => {
  if (summaryPrinted) {
    return;
  }
  summaryPrinted = true;

  if (parallelRunner != null) {
    parallelRunner.printSummary();
    return;
  }

  printSummary({ numPass, numFail, timerStarted });
};

const signalExitCode = {
  SIGHUP: 129,
  SIGINT: 130,
  SIGTERM: 143,
};

const handleSignal = signal => {
  console.error(chalk.yellow(`\nReceived ${signal}; printing partial summary.`));
  printHarnessSummary();

  const failCount =
    parallelRunner != null ? parallelRunner.getCounts().numFail : numFail;
  const exitCode = signalExitCode[signal] ?? 1;
  process.exit(failCount > 0 ? 1 : exitCode);
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
  })) {
    console.log(file);
  }
  process.exit(0);
}

const jobs = resolveJobsOption(options.jobs);
const selectedTestPaths = collectSelectedTestPaths({
  filterRegexp: options.filter,
  includeCrashtests: options.includeCrashtests,
});

// Warm up native module in the parent for sequential mode; workers load their own copy.
if (jobs === 1) {
  createWptEnvironment();
}

try {
  console.time('wpt-duration');
  timerStarted = true;

  if (jobs === 1) {
    const numPassRef = { value: 0 };
    const numFailRef = { value: 0 };
    const filter = createSequentialFilter({
      filterRegexp: options.filter,
      includeCrashtests: options.includeCrashtests,
      listOnly: false,
    });
    const reporter = createConsoleReporter({ numPassRef, numFailRef });
    const fileFailures = await runSequentialWpt({ filter, reporter });
    numPass = numPassRef.value;
    numFail = numFailRef.value;
    if (fileFailures > 0 && numFail === 0) {
      numFail = fileFailures;
    }
  } else {
    parallelRunner = new WptParallelRunner({
      jobs,
      handlers: createParallelConsoleReporter(),
    });
    parallelRunner.markTimerStarted();
    const result = await parallelRunner.run(selectedTestPaths);
    numPass = result.numPass;
    numFail = result.numFail;
    parallelRunner.printSummary();
    summaryPrinted = true;
    parallelRunner = null;
  }

  if (!summaryPrinted) {
    printHarnessSummary();
  }
  process.exit(numFail > 0 ? 1 : 0);
} catch (error) {
  printHarnessSummary();
  console.error(error.stack);
  process.exit(1);
}
