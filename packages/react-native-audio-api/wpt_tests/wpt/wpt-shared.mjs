import EventEmitter from 'node:events';
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';

import chalk from 'chalk';
import wptRunner from 'wpt-runner';

import { wrapAudioNodeConstructors } from './wrap-audio-node-constructors.mjs';
import { applyChannelMergerSplitterAttributeLocks } from './wpt-only/channel-merger-splitter-attribute-locks.mjs';
import {
  wrapAudioBufferCopyMethods,
  wrapWebAudioRealmErrors,
} from './wpt-utils.mjs';

const require = createRequire(import.meta.url);
const { setFloat32ArrayViewFactory } = require('../../lib/commonjs/errors/index.js');
const createRequestAnimationFrame = require('./mocks/requestAnimationFrame.js');

export const harnessDir = path.dirname(new URL(import.meta.url).pathname);
export const testsPath = path.join(harnessDir, '..', 'webaudio');
export const rootURL = 'webaudio';

export const smokeAllowlist = JSON.parse(
  fs.readFileSync(path.join(harnessDir, 'smoke-allowlist.json'), 'utf8')
);
export const fullAllowlist = ['webaudio'];
export const skipList = JSON.parse(
  fs.readFileSync(path.join(harnessDir, 'skip-list.json'), 'utf8')
);

export function getProfileAllowlist(profile = 'smoke') {
  if (profile === 'full') {
    return fullAllowlist;
  }
  return smokeAllowlist;
}

export function profileMatches(name, profile = 'smoke') {
  return getProfileAllowlist(profile).some(prefix => name.includes(prefix));
}

export function walkHtmlFiles(rootDir, prefix = '') {
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

export function smokeFilter(name, profile = 'smoke') {
  return profileMatches(name, profile);
}

export function getSkippedByPolicy(name) {
  return skipList.find(item => name.includes(item.pattern));
}

export function normalizeTestPath(filePath) {
  return filePath.split(path.sep).join('/');
}

/**
 * WPT "test class" — typically one interface directory under the-audio-api/.
 * Used to group results by interface for the coverage report.
 */
export function getTestClass(testPath) {
  const normalized = normalizeTestPath(testPath);
  const parts = normalized.split('/');

  if (parts[0] === 'the-audio-api' && parts.length >= 3) {
    return `${parts[0]}/${parts[1]}`;
  }

  if (parts.length >= 2) {
    return parts.slice(0, -1).join('/');
  }

  return 'root';
}

export function collectSelectedTestPaths({
  filterRegexp = '.*',
  includeCrashtests = false,
  profile = 'smoke',
}) {
  const extraFilterRe = new RegExp(filterRegexp);
  const selected = [];

  for (const file of walkHtmlFiles(testsPath)) {
    const fullName = normalizeTestPath(file);
    if (getSkippedByPolicy(fullName) != null) {
      continue;
    }
    if (!includeCrashtests && fullName.includes('/crashtests/')) {
      continue;
    }
    if (!smokeFilter(fullName, profile)) {
      continue;
    }
    if (!extraFilterRe.test(fullName)) {
      continue;
    }
    selected.push(fullName);
  }

  return selected.sort();
}

export function loadNodeAudioApi() {
  const nodeAudioApi = require('../index.js');
  const { TypeError: _TypeError, RangeError: _RangeError, ...audioApiForWindow } =
    nodeAudioApi;
  return { nodeAudioApi, audioApiForWindow };
}

export function alignGlobalRealmConstructors(window) {
  // Library throws via globalThis.*; align with the jsdom window realm so
  // assert_throws_js / assert_throws_dom and audit.js constructor checks pass.
  globalThis.TypeError = window.TypeError;
  globalThis.RangeError = window.RangeError;
  if (window.DOMException != null) {
    globalThis.DOMException = window.DOMException;
  }
}

export function createWptEnvironment() {
  const cleanupEmitter = new EventEmitter();
  const { nodeAudioApi, audioApiForWindow } = loadNodeAudioApi();
  let cancelPendingAnimationFrames = () => {};

  cleanupEmitter.on('cleanup', () => {
    cancelPendingAnimationFrames();
  });

  const setup = window => {
    cleanupEmitter.emit('cleanup');

    setFloat32ArrayViewFactory(
      (buffer, byteOffset, length) =>
        new window.Float32Array(buffer, byteOffset, length)
    );

    Object.assign(window, audioApiForWindow);
    alignGlobalRealmConstructors(window);
    wrapAudioNodeConstructors(window);
    applyChannelMergerSplitterAttributeLocks(window);
    wrapWebAudioRealmErrors(window);
    wrapAudioBufferCopyMethods(window);
    if (window.navigator == null) {
      window.navigator = {};
    }
    window.navigator.mediaDevices = nodeAudioApi.mediaDevices;

    const animationFrame = createRequestAnimationFrame(window);
    window.requestAnimationFrame = animationFrame.requestAnimationFrame;
    window.cancelAnimationFrame = animationFrame.cancelAnimationFrame;
    cancelPendingAnimationFrames = animationFrame.cancelAll;
  };

  return { setup, cleanupEmitter };
}

export function createSequentialFilter({
  filterRegexp,
  includeCrashtests,
  listOnly,
  profile = 'smoke',
}) {
  const extraFilterRe = new RegExp(filterRegexp);

  return name => {
    if (getSkippedByPolicy(name) != null) {
      return false;
    }
    if (!includeCrashtests && name.includes('/crashtests/')) {
      return false;
    }
    if (!smokeFilter(name, profile)) {
      return false;
    }
    if (!extraFilterRe.test(name)) {
      return false;
    }
    if (listOnly) {
      console.log(name);
      return false;
    }
    return true;
  };
}

export async function runSequentialWpt({ filter, reporter }) {
  const { setup } = createWptEnvironment();
  return wptRunner(testsPath, { rootURL, setup, filter, reporter });
}

export function createReporter(handlers) {
  return {
    startSuite: name => handlers.startSuite?.(name),
    pass: message => handlers.pass?.(message),
    fail: message => handlers.fail?.(message),
    reportStack: stack => handlers.reportStack?.(stack),
  };
}

export function createConsoleReporter({ numPassRef, numFailRef }) {
  return createReporter({
    startSuite: name => {
      console.log(`\n${chalk.bold.underline(path.join(testsPath, name))}\n`);
    },
    pass: message => {
      numPassRef.value += 1;
      console.log(chalk.green(`  √ ${message}`));
    },
    fail: message => {
      numFailRef.value += 1;
      console.log(chalk.red(`  × ${message}`));
    },
    reportStack: stack => {
      console.log(chalk.dim(`    ${stack}`));
    },
  });
}

export function printSummary({ numPass, numFail, timerStarted }) {
  const total = numPass + numFail;
  const passRate = total > 0 ? ((numPass / total) * 100).toFixed(1) : '0.0';
  console.log(chalk.bold(`\nPASS: ${numPass}`));
  console.log(chalk.bold(`FAIL: ${numFail}`));
  console.log(chalk.bold(`PASS RATE: ${passRate}% (${numPass}/${total})`));
  if (timerStarted) {
    console.timeEnd('wpt-duration');
  }
}
