import { Blob } from 'node:buffer';
import EventEmitter from 'node:events';
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';

import chalk from 'chalk';
import { program } from 'commander';
import wptRunner from 'wpt-runner';
import { setFloat32ArrayViewFactory } from '../../lib/commonjs/errors/index.js';
import { wrapAudioNodeConstructors } from './wrap-audio-node-constructors.mjs';

const require = createRequire(import.meta.url);
const createRequestAnimationFrame = require('./mocks/requestAnimationFrame.js');

const harnessDir = path.dirname(new URL(import.meta.url).pathname);
const allowListPath = path.join(harnessDir, 'smoke-allowlist.json');
const skipListPath = path.join(harnessDir, 'skip-list.json');
const smokeAllowlist = JSON.parse(fs.readFileSync(allowListPath, 'utf8'));
const skipList = JSON.parse(fs.readFileSync(skipListPath, 'utf8'));

program
  .option('--list', 'List test files only')
  .option('--filter <regexp>', 'Additional regex filter for tests', '.*')
  .option('--include-crashtests', 'Include crashtests', false);

program.parse(process.argv);
const options = program.opts();

const wptRootPath = path.join(harnessDir, '..', 'wpt-src');
const testsPath = path.join(wptRootPath, 'webaudio');
const rootURL = 'webaudio';

if (!fs.existsSync(testsPath)) {
  console.error(`Missing WPT checkout at '${testsPath}'.`);
  console.error(
    'Initialize WPT source first: yarn wpt:init'
  );
  process.exit(1);
}

process.WPT_TEST_RUNNER = new EventEmitter();

process.on('unhandledRejection', error => {
  const message = error instanceof Error ? error.stack || error.message : String(error);
  console.error(chalk.red(`Unhandled rejection during WPT run:\n${message}`));
});

process.on('uncaughtException', error => {
  const message = error instanceof Error ? error.stack || error.message : String(error);
  console.error(chalk.red(`Uncaught exception during WPT run:\n${message}`));
});

let numPass = 0;
let numFail = 0;
let timerStarted = false;
let summaryPrinted = false;

const printSummary = () => {
  if (summaryPrinted) {
    return;
  }
  summaryPrinted = true;
  const total = numPass + numFail;
  const passRate = total > 0 ? ((numPass / total) * 100).toFixed(1) : '0.0';
  console.log(chalk.bold(`\nPASS: ${numPass}`));
  console.log(chalk.bold(`FAIL: ${numFail}`));
  console.log(chalk.bold(`PASS RATE: ${passRate}% (${numPass}/${total})`));
  if (timerStarted) {
    console.timeEnd('wpt-duration');
  }
};

const signalExitCode = {
  SIGHUP: 129,
  SIGINT: 130,
  SIGTERM: 143,
};

const handleSignal = signal => {
  console.error(chalk.yellow(`\nReceived ${signal}; printing partial summary.`));
  printSummary();
  process.exit(signalExitCode[signal] ?? 1);
};

process.on('SIGHUP', () => handleSignal('SIGHUP'));
process.on('SIGINT', () => handleSignal('SIGINT'));
process.on('SIGTERM', () => handleSignal('SIGTERM'));

const smokeFilter = name => smokeAllowlist.some(prefix => name.includes(prefix));
const extraFilterRe = new RegExp(options.filter);

function walkHtmlFiles(rootDir, prefix = '') {
  const entries = fs.readdirSync(path.join(rootDir, prefix), { withFileTypes: true });
  let files = [];
  for (const entry of entries) {
    const rel = path.join(prefix, entry.name);
    if (entry.isDirectory()) {
      files = files.concat(walkHtmlFiles(rootDir, rel));
    } else if (entry.name.endsWith('.html')) {
      files.push(rel);
    }
  }
  return files;
}

if (options.list) {
  const allFiles = walkHtmlFiles(testsPath);
  for (const file of allFiles) {
    const fullName = file.split(path.sep).join('/');
    const skippedByPolicy = skipList.find(item => fullName.includes(item.pattern));
    if (skippedByPolicy != null) {
      continue;
    }
    if (smokeFilter(fullName) && extraFilterRe.test(fullName)) {
      console.log(fullName);
    }
  }
  process.exit(0);
}

const nodeAudioApi = require('../index.js');
const { TypeError: _TypeError, RangeError: _RangeError, ...audioApiForWindow } =
  nodeAudioApi;

let cancelPendingAnimationFrames = () => {};

process.WPT_TEST_RUNNER.on('cleanup', () => {
  cancelPendingAnimationFrames();
});

const setup = window => {
  process.WPT_TEST_RUNNER.emit('cleanup');

  setFloat32ArrayViewFactory(
    (buffer, byteOffset, length) => new window.Float32Array(buffer, byteOffset, length)
  );

  Object.assign(window, audioApiForWindow);
  wrapAudioNodeConstructors(window);
  if (window.navigator == null) {
    window.navigator = {};
  }
  window.navigator.mediaDevices = nodeAudioApi.mediaDevices;

  const animationFrame = createRequestAnimationFrame(window);
  window.requestAnimationFrame = animationFrame.requestAnimationFrame;
  window.cancelAnimationFrame = animationFrame.cancelAnimationFrame;
  cancelPendingAnimationFrames = animationFrame.cancelAll;
};

const filter = name => {
  const skippedByPolicy = skipList.find(item => name.includes(item.pattern));
  if (skippedByPolicy != null) {
    return false;
  }

  if (!options.includeCrashtests && name.includes('/crashtests/')) {
    return false;
  }

  if (!smokeFilter(name)) {
    return false;
  }

  if (!extraFilterRe.test(name)) {
    return false;
  }

  if (options.list) {
    console.log(name);
    return false;
  }

  return true;
};

const reporter = {
  startSuite: name => {
    console.log(`\n${chalk.bold.underline(path.join(testsPath, name))}\n`);
  },
  pass: message => {
    numPass += 1;
    console.log(chalk.green(`  √ ${message}`));
  },
  fail: message => {
    numFail += 1;
    console.log(chalk.red(`  × ${message}`));
  },
  reportStack: stack => {
    console.log(chalk.dim(`    ${stack}`));
  },
};

try {
  console.time('wpt-duration');
  timerStarted = true;
  await wptRunner(testsPath, { rootURL, setup, filter, reporter });
  printSummary();
  process.exit(numFail > 0 ? 1 : 0);
} catch (error) {
  printSummary();
  console.error(error.stack);
  process.exit(1);
}
