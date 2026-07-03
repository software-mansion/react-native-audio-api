import { fork } from 'node:child_process';
import path from 'node:path';

import chalk from 'chalk';
import wptRunner from 'wpt-runner';

import {
  createReporter,
  createWptEnvironment,
  groupTestsByClass,
  normalizeTestPath,
  printSummary,
  rootURL,
  testsPath,
} from './wpt-shared.mjs';

const workerScriptPath = path.join(
  path.dirname(new URL(import.meta.url).pathname),
  'wpt-worker.mjs'
);

/**
 * Runs independent WPT test classes in separate child processes.
 * Each worker loads its own JSI/native runtime and executes one test class
 * (interface directory) at a time from a shared queue.
 */
export class WptParallelRunner {
  #jobs;
  #handlers;
  #numPass = 0;
  #numFail = 0;
  #timerStarted = false;
  #summaryPrinted = false;
  #queue = [];
  #activeWorkers = 0;
  #nextWorkerId = 0;
  #resolveRun;
  #rejectRun;
  /** @type {Map<number, { child: import('node:child_process').ChildProcess, busy: boolean, assignment: object | null }>} */
  #workerState = new Map();

  constructor({ jobs, handlers = {} }) {
    if (!Number.isFinite(jobs) || jobs < 1) {
      throw new Error(`WptParallelRunner requires jobs >= 1, got ${jobs}`);
    }
    this.#jobs = jobs;
    this.#handlers = handlers;
  }

  async run(testPaths) {
    const normalizedPaths = testPaths.map(normalizeTestPath);
    this.#queue = groupTestsByClass(normalizedPaths);

    if (this.#queue.length === 0) {
      return { numPass: 0, numFail: 0 };
    }

    const workerCount = Math.min(this.#jobs, this.#queue.length);
    console.log(
      chalk.dim(
        `Running ${normalizedPaths.length} files in ${this.#queue.length} test classes ` +
          `using ${workerCount} worker process(es).`
      )
    );

    return new Promise((resolve, reject) => {
      this.#resolveRun = resolve;
      this.#rejectRun = reject;

      for (let i = 0; i < workerCount; i += 1) {
        this.#spawnWorker();
      }
    });
  }

  printSummary() {
    if (this.#summaryPrinted) {
      return;
    }
    this.#summaryPrinted = true;
    printSummary({
      numPass: this.#numPass,
      numFail: this.#numFail,
      timerStarted: this.#timerStarted,
    });
  }

  markTimerStarted() {
    this.#timerStarted = true;
  }

  getCounts() {
    return { numPass: this.#numPass, numFail: this.#numFail };
  }

  #spawnWorker() {
    const workerId = this.#nextWorkerId;
    this.#nextWorkerId += 1;

    const child = fork(workerScriptPath, [], {
      stdio: ['inherit', 'inherit', 'inherit', 'ipc'],
      env: {
        ...process.env,
        WPT_WORKER_ID: String(workerId),
      },
    });

    this.#activeWorkers += 1;
    this.#workerState.set(workerId, { child, busy: false, assignment: null });

    child.on('message', message => {
      this.#handleWorkerMessage(workerId, message);
    });

    child.on('error', error => {
      if (this.#rejectRun != null) {
        const reject = this.#rejectRun;
        this.#resolveRun = null;
        this.#rejectRun = null;
        reject(error);
      }
    });

    child.on('exit', (code, signal) => {
      const state = this.#workerState.get(workerId);
      const assignment = state?.assignment ?? null;
      const wasBusy = state?.busy === true;

      this.#activeWorkers -= 1;
      this.#workerState.delete(workerId);

      if (signal != null && this.#rejectRun != null) {
        const reject = this.#rejectRun;
        this.#resolveRun = null;
        this.#rejectRun = null;
        reject(new Error(`WPT worker ${workerId} exited due to signal ${signal}`));
        return;
      }

      if (code !== 0 && wasBusy) {
        this.#numFail += 1;
        console.error(
          chalk.red(
            `WPT worker ${workerId} crashed while running a test class (exit code ${code}).`
          )
        );
        if (assignment != null) {
          this.#queue.unshift(assignment);
        }
      }

      if (this.#queue.length > 0 && this.#workerState.size < this.#jobs) {
        this.#spawnWorker();
      }

      this.#maybeFinishRun();
    });

    this.#dispatchNext(workerId);
  }

  #dispatchNext(workerId) {
    const state = this.#workerState.get(workerId);
    if (state == null) {
      return;
    }

    const nextGroup = this.#queue.shift();
    if (nextGroup == null) {
      state.busy = false;
      state.assignment = null;
      state.child.send({ type: 'shutdown' });
      return;
    }

    state.busy = true;
    state.assignment = nextGroup;
    state.child.send({
      type: 'run',
      testClass: nextGroup.testClass,
      testPaths: nextGroup.testPaths,
    });
  }

  #handleWorkerMessage(workerId, message) {
    switch (message.type) {
      case 'suite':
        this.#handlers.startSuite?.(message.name, message.testClass);
        break;
      case 'pass':
        this.#numPass += 1;
        this.#handlers.pass?.(message.message, message.testClass);
        break;
      case 'fail':
        this.#numFail += 1;
        this.#handlers.fail?.(message.message, message.stack, message.testClass);
        break;
      case 'stack':
        this.#handlers.reportStack?.(message.stack, message.testClass);
        break;
      case 'done': {
        const state = this.#workerState.get(workerId);
        if (state != null) {
          state.busy = false;
          state.assignment = null;
        }
        this.#dispatchNext(workerId);
        break;
      }
      default:
        break;
    }
  }

  #maybeFinishRun() {
    const hasBusyWorkers = [...this.#workerState.values()].some(state => state.busy);

    if (
      this.#activeWorkers === 0 &&
      this.#queue.length === 0 &&
      !hasBusyWorkers &&
      this.#resolveRun != null
    ) {
      const resolve = this.#resolveRun;
      this.#resolveRun = null;
      this.#rejectRun = null;
      resolve({ numPass: this.#numPass, numFail: this.#numFail });
    }
  }
}

/**
 * Sequential fallback — same behavior as the original harness (single process).
 */
export async function runSequentialWpt({ filter, reporter }) {
  const { setup } = createWptEnvironment();
  const failures = await wptRunner(testsPath, { rootURL, setup, filter, reporter });
  return failures;
}

export function createParallelConsoleReporter() {
  return {
    startSuite: (name, testClass) => {
      const label = testClass != null ? chalk.dim(`[${testClass}] `) : '';
      console.log(`\n${label}${chalk.bold.underline(path.join(testsPath, name))}\n`);
    },
    pass: (message, testClass) => {
      const label = testClass != null ? chalk.dim(`[${testClass}] `) : '';
      console.log(`${label}${chalk.green(`  √ ${message}`)}`);
    },
    fail: (message, stack, testClass) => {
      const label = testClass != null ? chalk.dim(`[${testClass}] `) : '';
      console.log(`${label}${chalk.red(`  × ${message}`)}`);
      if (stack) {
        console.log(chalk.dim(`    ${stack}`));
      }
    },
    reportStack: (stack, testClass) => {
      const label = testClass != null ? chalk.dim(`[${testClass}] `) : '';
      console.log(`${label}${chalk.dim(`    ${stack}`)}`);
    },
  };
}
