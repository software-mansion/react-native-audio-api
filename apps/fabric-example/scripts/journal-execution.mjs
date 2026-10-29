#!/usr/bin/env node

import { execFileSync, spawn } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import readline from 'node:readline';
import { fileURLToPath } from 'node:url';

const SCRIPT_DIR = path.dirname(fileURLToPath(import.meta.url));
const APP_DIR = path.resolve(SCRIPT_DIR, '..');
const LOGS_ROOT_DIR = path.join(APP_DIR, 'logs');
const LOG_STREAM_IOS_SCRIPT = path.join(SCRIPT_DIR, 'log-stream-ios.sh');

const ENTER_ALTERNATE_SCREEN = '\x1b[?1049h\x1b[2J\x1b[H';
const EXIT_ALTERNATE_SCREEN = '\x1b[?1049l';
const CLEAR_SCREEN = '\x1b[2J\x1b[H';
const VALID_NAME_PATTERN = /^[A-Za-z0-9_-]+$/;

function printHelp() {
  console.log(`Usage: node scripts/journal-execution.mjs [options]

Streams logs and records timestamped journal entries into apps/fabric-example/logs/.
Press any key while streaming to open the interactive journal prompt.
Press Ctrl+D to exit and save logs and journal.

Options:
  --platform PLATFORM   ios (default) or android (not yet supported).
  --name NAME           Optional test label included in the run directory name.
  --device UDID         Stream from this device.
  --simulator           Stream from the booted simulator, even when a device is connected.
  --subsystem LIST      Subsystems, separated by '|' (audioapi, js, all, or full name).
                        Default: audioapi.
  --module LIST         Log categories, e.g. 'AudioEngine|AudioSessionManager'. Default: all.
  --level LEVEL         error, warning, debug or trace. Default: all.
  --help, -h            Show this help.

Examples:
  yarn journal-execution --name siri-interruption --subsystem 'audioapi|js' --level trace
  yarn journal-execution --simulator --level debug`);
}

function fail(message) {
  console.error(message);
  process.exit(1);
}

function parseArguments(argv) {
  let platform = 'ios';
  let testName = '';
  const forwardedArgs = [];

  for (let i = 0; i < argv.length; i += 1) {
    const arg = argv[i];
    switch (arg) {
      case '--platform': {
        const value = argv[i + 1];
        if (!value) {
          fail('--platform needs a value');
        }
        platform = value;
        i += 1;
        break;
      }
      case '--name': {
        const value = argv[i + 1];
        if (!value) {
          fail('--name needs a value');
        }
        if (!VALID_NAME_PATTERN.test(value)) {
          fail(
            `Invalid --name '${value}'. Use only letters, digits, '_' or '-'.`
          );
        }
        testName = value;
        i += 1;
        break;
      }
      case '--device':
      case '--subsystem':
      case '--module':
      case '--level': {
        const value = argv[i + 1];
        if (!value) {
          fail(`${arg} needs a value`);
        }
        forwardedArgs.push(arg, value);
        i += 1;
        break;
      }
      case '--simulator':
        forwardedArgs.push(arg);
        break;
      case '--help':
      case '-h':
        printHelp();
        process.exit(0);
        break;
      default:
        console.error(`Unknown option: ${arg}`);
        printHelp();
        process.exit(1);
    }
  }

  if (platform !== 'ios') {
    if (platform === 'android') {
      fail('Android is not supported in this version of journal-execution.');
    }
    fail(`Invalid --platform '${platform}'. Expected ios.`);
  }

  return { testName, forwardedArgs };
}

function padTwo(value) {
  return String(value).padStart(2, '0');
}

function padThree(value) {
  return String(value).padStart(3, '0');
}

function formatLogTimestamp(date) {
  const year = date.getFullYear();
  const month = padTwo(date.getMonth() + 1);
  const day = padTwo(date.getDate());
  const hours = padTwo(date.getHours());
  const minutes = padTwo(date.getMinutes());
  const seconds = padTwo(date.getSeconds());
  const millis = padThree(date.getMilliseconds());
  return `${year}-${month}-${day} ${hours}:${minutes}:${seconds}.${millis}`;
}

function formatDirectoryTimestamp(date) {
  const year = date.getFullYear();
  const month = padTwo(date.getMonth() + 1);
  const day = padTwo(date.getDate());
  const hours = padTwo(date.getHours());
  const minutes = padTwo(date.getMinutes());
  const seconds = padTwo(date.getSeconds());
  return `${year}-${month}-${day}_${hours}-${minutes}-${seconds}`;
}

function currentCommitShortHash() {
  try {
    return execFileSync('git', ['rev-parse', '--short', 'HEAD'], {
      cwd: APP_DIR,
      encoding: 'utf8',
    }).trim();
  } catch {
    return 'unknown';
  }
}

function createRunDirectory(testName) {
  const commitHash = currentCommitShortHash();
  const timestamp = formatDirectoryTimestamp(new Date());
  const directoryName = testName
    ? `${commitHash}_${testName}_${timestamp}`
    : `${commitHash}_${timestamp}`;
  const runDirectory = path.join(LOGS_ROOT_DIR, directoryName);
  fs.mkdirSync(runDirectory, { recursive: true });
  return runDirectory;
}

const { testName, forwardedArgs } = parseArguments(process.argv.slice(2));
const runDirectory = createRunDirectory(testName);
const logsFilePath = path.join(runDirectory, 'logs.log');
const journalFilePath = path.join(runDirectory, 'journal.log');

fs.writeFileSync(logsFilePath, '');
fs.writeFileSync(journalFilePath, '');

const logsFileStream = fs.createWriteStream(logsFilePath, { flags: 'a' });
const journalFileStream = fs.createWriteStream(journalFilePath, { flags: 'a' });

console.error(`Run directory: ${runDirectory}`);
console.error(
  'Press any key to enter interactive journal mode. Press Ctrl+D to save and exit.'
);

let isInteractive = false;
let isShuttingDown = false;
let promptTimestamp = '';
let promptBuffer = '';
const bufferedLogChunks = [];

function renderInteractivePrompt() {
  process.stdout.write(
    `${CLEAR_SCREEN}` +
      `── Journal Entry (${promptTimestamp}) ──────────────────────────────────\n` +
      `Type your journal note.\n` +
      `Enter: accept and save  |  Esc: reject and discard  |  Ctrl+D: save and exit\n` +
      `─────────────────────────────────────────────────────────────────────────\n\n` +
      `> ${promptBuffer}`
  );
}

function enterInteractiveMode() {
  isInteractive = true;
  promptTimestamp = formatLogTimestamp(new Date());
  promptBuffer = '';
  process.stdout.write(ENTER_ALTERNATE_SCREEN);
  renderInteractivePrompt();
}

function flushBufferedLogsToStdout() {
  while (bufferedLogChunks.length > 0) {
    const chunk = bufferedLogChunks.shift();
    process.stdout.write(chunk);
  }
}

function saveJournalEntry(timestamp, text) {
  const trimmed = text.trim();
  if (!trimmed) {
    return null;
  }
  const entryLine = `${timestamp} [journal] ${trimmed}\n`;
  journalFileStream.write(entryLine);
  return entryLine;
}

function exitInteractiveMode(acceptedText) {
  const savedEntryLine =
    acceptedText !== null
      ? saveJournalEntry(promptTimestamp, acceptedText)
      : null;

  process.stdout.write(EXIT_ALTERNATE_SCREEN);
  flushBufferedLogsToStdout();
  if (savedEntryLine) {
    process.stdout.write(savedEntryLine);
  }
  isInteractive = false;
  promptBuffer = '';
}

const logProcess = spawn('bash', [LOG_STREAM_IOS_SCRIPT, ...forwardedArgs], {
  stdio: ['ignore', 'pipe', 'pipe'],
});

function handleLogChunk(chunk) {
  logsFileStream.write(chunk);
  if (isInteractive) {
    bufferedLogChunks.push(chunk);
  } else {
    process.stdout.write(chunk);
  }
}

logProcess.stdout.on('data', handleLogChunk);
logProcess.stderr.on('data', (chunk) => {
  if (isInteractive) {
    bufferedLogChunks.push(chunk);
  } else {
    process.stderr.write(chunk);
  }
});

function shutdown(exitCode = 0) {
  if (isShuttingDown) {
    return;
  }
  isShuttingDown = true;

  if (isInteractive) {
    exitInteractiveMode(promptBuffer);
  }

  if (process.stdin.isTTY) {
    process.stdin.setRawMode(false);
  }
  process.stdin.pause();

  if (!logProcess.killed && logProcess.exitCode === null) {
    logProcess.kill('SIGTERM');
  }

  logsFileStream.end(() => {
    journalFileStream.end(() => {
      console.error(`\nSaved logs and journal to ${runDirectory}`);
      process.exit(exitCode);
    });
  });
}

logProcess.on('close', (code) => {
  shutdown(code ?? 0);
});

readline.emitKeypressEvents(process.stdin);
if (process.stdin.isTTY) {
  process.stdin.setRawMode(true);
}
process.stdin.resume();

process.stdin.on('keypress', (character, key = {}) => {
  if (key.ctrl && (key.name === 'd' || key.name === 'c')) {
    shutdown(0);
    return;
  }

  if (!isInteractive) {
    enterInteractiveMode();
    return;
  }

  if (key.name === 'escape') {
    exitInteractiveMode(null);
    return;
  }

  if (key.name === 'return' || key.name === 'enter') {
    exitInteractiveMode(promptBuffer);
    return;
  }

  if (key.name === 'backspace') {
    promptBuffer = promptBuffer.slice(0, -1);
    renderInteractivePrompt();
    return;
  }

  if (character && !key.ctrl && !key.meta && character >= ' ') {
    promptBuffer += character;
    renderInteractivePrompt();
  }
});

process.on('SIGINT', () => shutdown(0));
process.on('SIGTERM', () => shutdown(0));
