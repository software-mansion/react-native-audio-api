import path from 'node:path';
import { fileURLToPath } from 'node:url';

import chalk from 'chalk';
import { program } from 'commander';

import {
  collectSelectedTestPaths,
  createConsoleReporter,
  createSequentialFilter,
  createWptEnvironment,
  getProfileAllowlist,
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

// Warm up the native module in the parent before running.
createWptEnvironment();

try {
  console.time('wpt-duration');
  timerStarted = true;

  const numPassRef = { value: 0 };
  const numFailRef = { value: 0 };
  const filter = createSequentialFilter({
    filterRegexp: options.filter,
    includeCrashtests: options.includeCrashtests,
    listOnly: false,
    profile: options.profile,
  });
  const reporter = wrapReporter(
    createConsoleReporter({ numPassRef, numFailRef }),
    resultsCollector
  );
  const fileFailures = await runSequentialWpt({ filter, reporter });
  numPass = numPassRef.value;
  numFail = numFailRef.value;
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
