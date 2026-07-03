import wptRunner from 'wpt-runner';

import {
  createReporter,
  createWptEnvironment,
  normalizeTestPath,
  rootURL,
  testsPath,
} from './wpt-shared.mjs';

const workerId = process.env.WPT_WORKER_ID ?? '0';
const { setup } = createWptEnvironment();

function safeSend(message) {
  if (!process.connected) {
    return false;
  }

  try {
    process.send(message);
    return true;
  } catch {
    return false;
  }
}

process.on('message', async message => {
  if (message?.type === 'shutdown') {
    process.exit(0);
    return;
  }

  if (message?.type !== 'run') {
    return;
  }

  const allowedPaths = new Set(message.testPaths.map(normalizeTestPath));
  const testClass = message.testClass;

  let numPass = 0;
  let numFail = 0;

  const reporter = createReporter({
    startSuite: name => {
      safeSend({ type: 'suite', name, testClass });
    },
    pass: msg => {
      numPass += 1;
      safeSend({ type: 'pass', message: msg, testClass });
    },
    fail: msg => {
      numFail += 1;
      safeSend({ type: 'fail', message: msg, testClass });
    },
    reportStack: stack => {
      safeSend({ type: 'stack', stack, testClass });
    },
  });

  const filter = (name, _url) => allowedPaths.has(normalizeTestPath(name));

  try {
    await wptRunner(testsPath, { rootURL, setup, filter, reporter });
    safeSend({
      type: 'done',
      testClass,
      numPass,
      numFail,
      workerId,
    });
  } catch (error) {
    const stack = error instanceof Error ? error.stack || error.message : String(error);
    safeSend({
      type: 'fail',
      message: `Worker ${workerId} failed while running ${testClass}`,
      stack,
      testClass,
    });
    safeSend({
      type: 'done',
      testClass,
      numPass,
      numFail: numFail + 1,
      workerId,
    });
  }
});
