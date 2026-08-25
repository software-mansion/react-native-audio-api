import EventEmitter from 'node:events';
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';

import chalk from 'chalk';
import wptRunner from 'wpt-runner';

import { wrapAudioNodeConstructors } from './wrap-audio-node-constructors.mjs';
import { applyChannelMergerSplitterAttributeLocks } from './wpt-only/channel-merger-splitter-attribute-locks.mjs';
import {
  setCurrentTestWindow,
  wrapAudioBufferCopyMethods,
  wrapWebAudioRealmErrors,
} from './wpt-utils.mjs';

const require = createRequire(import.meta.url);
const packageRoot = require('../package-root.js');
const errorsModule = require(
  path.join(packageRoot, 'lib/commonjs/errors/index.js')
);
const createRequestAnimationFrame = require('./mocks/requestAnimationFrame.js');

// Older package versions (loaded via RN_AUDIO_API_APP_ROOT) may predate this hook.
const setFloat32ArrayViewFactory =
  errorsModule.setFloat32ArrayViewFactory ?? (() => {});

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
  return getProfileAllowlist(profile).some((prefix) => name.includes(prefix));
}

export function walkHtmlFiles(rootDir, prefix = '') {
  const entries = fs.readdirSync(path.join(rootDir, prefix), {
    withFileTypes: true,
  });
  let files = [];
  for (const entry of entries) {
    const rel = path.join(prefix, entry.name);
    if (entry.isDirectory()) {
      files = files.concat(walkHtmlFiles(rootDir, rel));
    } else if (entry.name.endsWith('.html')) {
      files.push(rel);
    } else if (entry.name.endsWith('.window.js')) {
      // wpt-runner serves each .window.js as a synthesized <name>.window.html
      // test and reports it under that name; enumerate it the same way so
      // batch scheduling and --list see what actually runs.
      files.push(rel.replace(/\.window\.js$/, '.window.html'));
    }
  }
  return files;
}

export function smokeFilter(name, profile = 'smoke') {
  return profileMatches(name, profile);
}

export function getSkippedByPolicy(name) {
  return skipList.find((item) => name.includes(item.pattern));
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
  const {
    TypeError: _TypeError,
    RangeError: _RangeError,
    ...audioApiForWindow
  } = nodeAudioApi;
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

/**
 * Record every realtime AudioContext a test constructs, so the harness can close
 * the ones the test abandoned. Each context owns native worker threads that only
 * a close() (or GC, eventually) releases; without this, leaked contexts pile up
 * across files and the process ends the run holding hundreds of threads.
 * OfflineAudioContext has no close() and winds down when its render finishes.
 */
function trackRealtimeAudioContexts(window, liveContexts) {
  const Previous = window.AudioContext;
  if (typeof Previous !== 'function') {
    return;
  }

  function Tracked(...args) {
    const instance = Reflect.construct(Previous, args, new.target ?? Tracked);
    liveContexts.add(instance);
    return instance;
  }
  Tracked.prototype = Previous.prototype;
  Object.defineProperty(Tracked, 'name', { value: Previous.name });
  window.AudioContext = Tracked;
}

export function createWptEnvironment() {
  const cleanupEmitter = new EventEmitter();
  const { nodeAudioApi, audioApiForWindow } = loadNodeAudioApi();
  let cancelPendingAnimationFrames = () => {};
  const liveAudioContexts = new Set();

  cleanupEmitter.on('cleanup', () => {
    cancelPendingAnimationFrames();

    // Close whatever realtime contexts the finished test left running. Tests
    // that closed their own context make close() reject — swallowed on purpose.
    for (const context of liveAudioContexts) {
      try {
        const result = context.close();
        if (typeof result?.catch === 'function') {
          result.catch(() => {});
        }
      } catch {
        // Already closed or torn down.
      }
    }
    liveAudioContexts.clear();
  });

  const setup = (window) => {
    cleanupEmitter.emit('cleanup');

    // Shared-prototype patches resolve the window through this rather than closing
    // over it, so a finished test's window stays collectable.
    setCurrentTestWindow(window);

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

    // Last, so it wraps the outermost constructor and records real instances.
    trackRealtimeAudioContexts(window, liveAudioContexts);
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

  return (name) => {
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
  const { setup, cleanupEmitter } = createWptEnvironment();
  const failures = await wptRunner(testsPath, {
    rootURL,
    setup,
    filter,
    reporter,
  });
  // One final sweep for the last file — 'cleanup' otherwise only fires when the
  // NEXT file's setup() runs.
  cleanupEmitter.emit('cleanup');
  return failures;
}

export function createReporter(handlers) {
  return {
    startSuite: (name) => handlers.startSuite?.(name),
    pass: (message) => handlers.pass?.(message),
    fail: (message) => handlers.fail?.(message),
    reportStack: (stack) => handlers.reportStack?.(stack),
  };
}

export function createConsoleReporter({ numPassRef, numFailRef }) {
  return createReporter({
    startSuite: (name) => {
      console.log(`\n${chalk.bold.underline(path.join(testsPath, name))}\n`);
    },
    pass: (message) => {
      numPassRef.value += 1;
      console.log(chalk.green(`  √ ${message}`));
    },
    fail: (message) => {
      numFailRef.value += 1;
      console.log(chalk.red(`  × ${message}`));
    },
    reportStack: (stack) => {
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
