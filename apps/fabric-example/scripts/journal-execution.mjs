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
const LEADING_TIMESTAMP_PATTERN = /^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+)/;

function printHelp() {
  console.log(`Usage: node scripts/journal-execution.mjs [options]

Streams logs and records timestamped journal entries into a temporary directory,
then merges them into a single <run-name>.log file in apps/fabric-example/logs/ on exit.
Press any key while streaming to open the interactive journal prompt.
Press Ctrl+D to exit and save the merged log and journal file.

Options:
  --platform PLATFORM   ios (default) or android (not yet supported).
  --name NAME           Optional test label included in the output file name.
  --output-dir DIR      Directory where the temporary run folder and merged .log file are written.
                        Default: apps/fabric-example/logs.
  --headless            Disable interactive keyboard input and stdout log echoing; stream only
                        to disk until SIGTERM/SIGINT merges logs.log and journal.log.
  --run-dir-file FILE   Write the path of the created temporary run directory to FILE once ready.
  --device UDID         Stream from this device.
  --simulator           Stream from the booted simulator, even when a device is connected.
  --subsystem LIST      Subsystems, separated by '|' (audioapi, js, all, or full name).
                        Default: audioapi.
  --module LIST         Log categories, e.g. 'AudioEngine|AudioSessionManager'. Default: all.
  --level LEVEL         error, warning, debug or trace. Default: all.
  --grep PATTERN        Regular expression matched against log lines (does not filter journal entries).
  --help, -h            Show this help.

Examples:
  yarn journal-execution --name siri-interruption --subsystem 'audioapi|js' --level trace
  yarn journal-execution --simulator --level debug --grep 'interruption|routeChange'`);
}

function fail(message) {
  console.error(message);
  process.exit(1);
}

function parseArguments(argv) {
  let platform = 'ios';
  let testName = '';
  let outputDir = LOGS_ROOT_DIR;
  let headless = false;
  let runDirFile = '';
  let grepArg = '';
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
      case '--output-dir': {
        const value = argv[i + 1];
        if (!value) {
          fail('--output-dir needs a value');
        }
        outputDir = path.resolve(value);
        i += 1;
        break;
      }
      case '--headless':
        headless = true;
        break;
      case '--run-dir-file': {
        const value = argv[i + 1];
        if (!value) {
          fail('--run-dir-file needs a value');
        }
        runDirFile = path.resolve(value);
        i += 1;
        break;
      }
      case '--grep': {
        const value = argv[i + 1];
        if (!value) {
          fail('--grep needs a value');
        }
        grepArg = value;
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

  let grepRegex = null;
  if (grepArg) {
    try {
      grepRegex = new RegExp(grepArg);
    } catch (error) {
      fail(`Invalid --grep regular expression '${grepArg}': ${error.message}`);
    }
  }

  return {
    testName,
    outputDir,
    headless,
    runDirFile,
    forwardedArgs,
    grepRegex,
  };
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

function formatRunTimestamp(date) {
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

function createRunPaths(testName, outputDir) {
  const commitHash = currentCommitShortHash();
  const timestamp = formatRunTimestamp(new Date());
  const baseName = testName
    ? `${commitHash}_${testName}_${timestamp}`
    : `${commitHash}_${timestamp}`;
  const runDirectory = path.join(outputDir, baseName);
  const finalLogFilePath = path.join(outputDir, `${baseName}.log`);
  fs.mkdirSync(runDirectory, { recursive: true });
  return { runDirectory, finalLogFilePath };
}

function readTimestampedEntries(filePath) {
  if (!fs.existsSync(filePath)) {
    return [];
  }
  const content = fs.readFileSync(filePath, 'utf8');
  const lines = content.split(/\r?\n/);
  const entries = [];
  let currentEntry = null;

  for (const line of lines) {
    if (!line) {
      continue;
    }
    const match = line.match(LEADING_TIMESTAMP_PATTERN);
    if (match) {
      currentEntry = {
        timestamp: match[1].slice(0, 23),
        text: line,
      };
      entries.push(currentEntry);
    } else if (currentEntry) {
      currentEntry.text += `\n${line}`;
    } else {
      entries.push({
        timestamp: '',
        text: line,
      });
    }
  }

  return entries;
}

function mergeRunDirectoryIntoSingleFile(
  runDirectory,
  logsFilePath,
  journalFilePath,
  finalLogFilePath
) {
  const entries = [
    ...readTimestampedEntries(logsFilePath),
    ...readTimestampedEntries(journalFilePath),
  ];
  entries.sort((a, b) => a.timestamp.localeCompare(b.timestamp));

  const mergedContent =
    entries.length > 0
      ? `${entries.map((entry) => entry.text).join('\n')}\n`
      : '';
  fs.writeFileSync(finalLogFilePath, mergedContent, 'utf8');
  fs.rmSync(runDirectory, { recursive: true, force: true });
}

const { testName, outputDir, headless, runDirFile, forwardedArgs, grepRegex } =
  parseArguments(process.argv.slice(2));
const { runDirectory, finalLogFilePath } = createRunPaths(testName, outputDir);
const logsFilePath = path.join(runDirectory, 'logs.log');
const journalFilePath = path.join(runDirectory, 'journal.log');

fs.writeFileSync(logsFilePath, '');
fs.writeFileSync(journalFilePath, '');
if (runDirFile) {
  fs.mkdirSync(path.dirname(runDirFile), { recursive: true });
  fs.writeFileSync(runDirFile, `${runDirectory}\n`, 'utf8');
}

const logsFileStream = fs.createWriteStream(logsFilePath, { flags: 'a' });
const journalFileStream = fs.createWriteStream(journalFilePath, { flags: 'a' });

console.error(`Working directory: ${runDirectory}`);
if (!headless) {
  console.error(
    'Press any key to enter interactive journal mode. Press Ctrl+D to save and exit.'
  );
}

let isInteractive = false;
let isShuttingDown = false;
let promptTimestamp = '';
let promptBuffer = '';
const bufferedOutputChunks = [];

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

function flushBufferedOutput() {
  while (bufferedOutputChunks.length > 0) {
    const item = bufferedOutputChunks.shift();
    item.stream.write(item.data);
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
  flushBufferedOutput();
  if (savedEntryLine) {
    process.stdout.write(savedEntryLine);
  }
  isInteractive = false;
  promptBuffer = '';
}

const logProcess = spawn('bash', [LOG_STREAM_IOS_SCRIPT, ...forwardedArgs], {
  stdio: ['ignore', 'pipe', 'pipe'],
});

function emitLogLine(line) {
  if (grepRegex && !grepRegex.test(line)) {
    return;
  }
  const lineWithNewline = `${line}\n`;
  logsFileStream.write(lineWithNewline);
  if (headless) {
    return;
  }
  if (isInteractive) {
    bufferedOutputChunks.push({
      stream: process.stdout,
      data: lineWithNewline,
    });
  } else {
    process.stdout.write(lineWithNewline);
  }
}

const stdoutReadline = readline.createInterface({
  input: logProcess.stdout,
  crlfDelay: Infinity,
});

stdoutReadline.on('line', emitLogLine);

logProcess.stderr.on('data', (chunk) => {
  if (isInteractive) {
    bufferedOutputChunks.push({ stream: process.stderr, data: chunk });
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

  if (!headless) {
    if (process.stdin.isTTY) {
      process.stdin.setRawMode(false);
    }
    process.stdin.pause();
  }

  stdoutReadline.close();
  if (!logProcess.killed && logProcess.exitCode === null) {
    logProcess.kill('SIGTERM');
  }

  logsFileStream.end(() => {
    journalFileStream.end(() => {
      mergeRunDirectoryIntoSingleFile(
        runDirectory,
        logsFilePath,
        journalFilePath,
        finalLogFilePath
      );
      console.error(`\nSaved merged logs and journal to ${finalLogFilePath}`);
      process.exit(exitCode);
    });
  });
}

logProcess.on('close', (code) => {
  shutdown(code ?? 0);
});

if (!headless) {
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
}

process.on('SIGINT', () => shutdown(0));
process.on('SIGTERM', () => shutdown(0));
