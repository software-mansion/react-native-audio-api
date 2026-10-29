#!/usr/bin/env node

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const SCRIPT_DIR = path.dirname(fileURLToPath(import.meta.url));
const APP_DIR = path.resolve(SCRIPT_DIR, '..');
const LOGS_ROOT_DIR = path.join(APP_DIR, 'logs');

const OWN_SUBSYSTEM = 'com.swmansion.audioapi';
const JS_SUBSYSTEM = 'com.facebook.react.log';
const LOG_LEVELS = ['error', 'warning', 'debug', 'trace'];
const VALID_SOURCES = new Set(['all', 'logs', 'journal']);
const LEADING_TIMESTAMP_PATTERN = /^(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+)/;

// Simulator compact format: "... <Process>[<pid>:<tid>] [<subsystem>:<category>] <message>"
const SIMULATOR_LABEL_PATTERN =
  /\] \[([A-Za-z0-9_.-]+):([A-Za-z0-9_.-]*)\] ([\s\S]*)$/;
// pymobiledevice3 format: "... <LEVEL>: <message> [<subsystem>][<category>]"
const DEVICE_LABEL_PATTERN =
  /<([A-Z]+)>: ([\s\S]*) \[([A-Za-z0-9_.-]+)\]\[([A-Za-z0-9_.-]*)\]$/;
// Simulator compact type column: "YYYY-MM-DD HH:MM:SS.mmm <Ty> <process>..."
const SIMULATOR_TYPE_PATTERN =
  /^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d+\s+([A-Za-z]+)/;
const OWN_LEVEL_TAG_PATTERN = /^\[(error|warning|debug|trace)\]/;

function printHelp() {
  console.log(`Usage: node scripts/journal-view.mjs [RUN_DIR] [options]

Combines logs.log and journal.log from a run folder into a single chronological timeline.
If RUN_DIR is omitted, the most recent directory in apps/fabric-example/logs/ is used.

Options:
  --source SOURCE     all (default), logs, or journal.
  --subsystem LIST    Subsystems, separated by '|' (audioapi, js, all, or full name).
                      Default: all.
  --module LIST       Log categories, e.g. 'AudioEngine|AudioSessionManager'. Default: all.
  --level LEVEL       error, warning, debug or trace. Keeps that level and more severe ones.
                      Default: all.
  --grep PATTERN      Regular expression matched against the entry text.
  --help, -h          Show this help.

Examples:
  yarn journal-view
  yarn journal-view --source journal
  yarn journal-view --subsystem audioapi --module AudioEngine --level debug
  yarn journal-view --grep 'interruption|\\\\[journal\\\\]'`);
}

function fail(message) {
  console.error(message);
  process.exit(1);
}

function parseArguments(argv) {
  let runDirArg = '';
  let source = 'all';
  let subsystemsArg = 'all';
  let modulesArg = '';
  let levelArg = '';
  let grepArg = '';

  for (let i = 0; i < argv.length; i += 1) {
    const arg = argv[i];
    switch (arg) {
      case '--source':
        source = argv[++i] ?? fail('--source needs a value');
        break;
      case '--subsystem':
        subsystemsArg = argv[++i] ?? fail('--subsystem needs a value');
        break;
      case '--module':
        modulesArg = argv[++i] ?? fail('--module needs a value');
        break;
      case '--level':
        levelArg = argv[++i] ?? fail('--level needs a value');
        break;
      case '--grep':
        grepArg = argv[++i] ?? fail('--grep needs a value');
        break;
      case '--help':
      case '-h':
        printHelp();
        process.exit(0);
        break;
      default:
        if (arg.startsWith('-')) {
          fail(`Unknown option: ${arg}`);
        }
        if (runDirArg) {
          fail(`Unexpected extra argument: ${arg}`);
        }
        runDirArg = arg;
    }
  }

  if (!VALID_SOURCES.has(source)) {
    fail(`Invalid --source '${source}'. Expected one of: all, logs, journal.`);
  }

  let allowedSubsystems = null;
  if (subsystemsArg !== 'all') {
    const items = subsystemsArg.split('|').filter(Boolean);
    if (items.includes('all')) {
      fail("'all' cannot be combined with other subsystems.");
    }
    allowedSubsystems = new Set(
      items.map((item) => {
        if (item === 'audioapi') {
          return OWN_SUBSYSTEM;
        }
        if (item === 'js') {
          return JS_SUBSYSTEM;
        }
        return item;
      })
    );
  }

  const allowedModules = modulesArg
    ? new Set(modulesArg.split('|').filter(Boolean))
    : null;

  let allowedLevels = null;
  if (levelArg) {
    const maxIndex = LOG_LEVELS.indexOf(levelArg);
    if (maxIndex === -1) {
      fail(
        `Invalid level: '${levelArg}'. Expected one of: ${LOG_LEVELS.join(' ')}`
      );
    }
    allowedLevels = new Set(LOG_LEVELS.slice(0, maxIndex + 1));
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
    runDirArg,
    source,
    allowedSubsystems,
    allowedModules,
    allowedLevels,
    levelArg,
    grepRegex,
  };
}

function resolveRunDirectory(runDirArg) {
  if (runDirArg) {
    const directCandidate = path.resolve(runDirArg);
    if (fs.existsSync(directCandidate)) {
      return directCandidate;
    }
    const underLogsCandidate = path.join(LOGS_ROOT_DIR, runDirArg);
    if (fs.existsSync(underLogsCandidate)) {
      return underLogsCandidate;
    }
    fail(`Run directory not found: ${runDirArg}`);
  }

  if (!fs.existsSync(LOGS_ROOT_DIR)) {
    fail(`No logs directory found at ${LOGS_ROOT_DIR}`);
  }

  const entries = fs
    .readdirSync(LOGS_ROOT_DIR, { withFileTypes: true })
    .filter((entry) => entry.isDirectory())
    .map((entry) => {
      const fullPath = path.join(LOGS_ROOT_DIR, entry.name);
      return {
        fullPath,
        mtimeMs: fs.statSync(fullPath).mtimeMs,
      };
    })
    .sort((a, b) => b.mtimeMs - a.mtimeMs);

  if (entries.length === 0) {
    fail(`No run directories found in ${LOGS_ROOT_DIR}`);
  }

  return entries[0].fullPath;
}

function readTimestampedEntries(filePath, entrySource) {
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
        source: entrySource,
        firstLine: line,
        text: line,
      };
      entries.push(currentEntry);
    } else if (currentEntry) {
      currentEntry.text += `\n${line}`;
    }
  }

  return entries;
}

function inferOtherSubsystemLevelFromLine(firstLine, deviceType) {
  if (deviceType) {
    if (deviceType === 'ERROR' || deviceType === 'FAULT') {
      return 'error';
    }
    if (deviceType === 'INFO') {
      return 'debug';
    }
    if (deviceType === 'DEBUG') {
      return 'trace';
    }
    return 'warning';
  }

  const simTypeMatch = firstLine.match(SIMULATOR_TYPE_PATTERN);
  const simType = simTypeMatch ? simTypeMatch[1] : '';
  if (simType === 'E' || simType === 'F') {
    return 'error';
  }
  if (simType === 'I') {
    return 'debug';
  }
  if (simType === 'D') {
    return 'trace';
  }
  return 'warning';
}

function parseLogMetadata(firstLine) {
  const simMatch = firstLine.match(SIMULATOR_LABEL_PATTERN);
  if (simMatch) {
    const [, subsystem, module, message] = simMatch;
    const tagMatch = message.trimStart().match(OWN_LEVEL_TAG_PATTERN);
    const level =
      subsystem === OWN_SUBSYSTEM
        ? (tagMatch?.[1] ?? null)
        : inferOtherSubsystemLevelFromLine(firstLine, null);
    return { subsystem, module, level };
  }

  const deviceMatch = firstLine.match(DEVICE_LABEL_PATTERN);
  if (deviceMatch) {
    const [, deviceType, message, subsystem, module] = deviceMatch;
    const tagMatch = message.trimStart().match(OWN_LEVEL_TAG_PATTERN);
    const level =
      subsystem === OWN_SUBSYSTEM
        ? (tagMatch?.[1] ?? null)
        : inferOtherSubsystemLevelFromLine(firstLine, deviceType);
    return { subsystem, module, level };
  }

  return {
    subsystem: null,
    module: null,
    level: inferOtherSubsystemLevelFromLine(firstLine, null),
  };
}

function matchesLogFilters(entry, filters) {
  if (filters.grepRegex && !filters.grepRegex.test(entry.text)) {
    return false;
  }

  if (entry.source === 'journal') {
    return true;
  }

  const { subsystem, module, level } = parseLogMetadata(entry.firstLine);

  if (
    filters.allowedSubsystems &&
    (!subsystem || !filters.allowedSubsystems.has(subsystem))
  ) {
    return false;
  }

  if (
    filters.allowedModules &&
    (!module || !filters.allowedModules.has(module))
  ) {
    return false;
  }

  if (filters.allowedLevels && (!level || !filters.allowedLevels.has(level))) {
    return false;
  }

  return true;
}

const filters = parseArguments(process.argv.slice(2));
const runDirectory = resolveRunDirectory(filters.runDirArg);

const entries = [];
if (filters.source === 'all' || filters.source === 'logs') {
  entries.push(
    ...readTimestampedEntries(path.join(runDirectory, 'logs.log'), 'logs')
  );
}
if (filters.source === 'all' || filters.source === 'journal') {
  entries.push(
    ...readTimestampedEntries(path.join(runDirectory, 'journal.log'), 'journal')
  );
}

entries.sort((a, b) => a.timestamp.localeCompare(b.timestamp));

for (const entry of entries) {
  if (matchesLogFilters(entry, filters)) {
    process.stdout.write(`${entry.text}\n`);
  }
}
