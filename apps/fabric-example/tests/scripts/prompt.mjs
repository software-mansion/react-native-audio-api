#!/usr/bin/env node

import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import readline from 'node:readline';

function printHelp() {
  console.log(`Usage: node tests/scripts/prompt.mjs [options]

Prompts a human tester for an external action or observation between Argent E2E
workflows and appends timestamped entries to the run's journal.log file.

Options:
  --mode MODE             action, choice, or text (required).
  --message TEXT          Instruction or question displayed to the tester (required).
  --journal-file PATH     Path to journal.log where timestamped entries are appended.
  --option LABEL          Add a choice option (repeatable, used with --mode choice).
  --pass-options LIST     Comma-separated 1-based option indices that count as a pass
                          (e.g. "1" or "1,2"). Selecting any other option exits with 1.
  --gui                   Use native macOS dialogs (osascript) instead of terminal stdin.
  --help, -h              Show this help.`);
}

function fail(message) {
  console.error(message);
  process.exit(1);
}

function parseArguments(argv) {
  let mode = '';
  let message = '';
  let journalFile = '';
  let useGui = false;
  const options = [];
  const passOptions = new Set();

  for (let i = 0; i < argv.length; i += 1) {
    const arg = argv[i];
    switch (arg) {
      case '--mode': {
        const value = argv[i + 1];
        if (!value) {
          fail('--mode needs a value (action, choice, or text)');
        }
        mode = value;
        i += 1;
        break;
      }
      case '--message': {
        const value = argv[i + 1];
        if (!value) {
          fail('--message needs a value');
        }
        message = value;
        i += 1;
        break;
      }
      case '--journal-file': {
        const value = argv[i + 1];
        if (!value) {
          fail('--journal-file needs a value');
        }
        journalFile = value;
        i += 1;
        break;
      }
      case '--option': {
        const value = argv[i + 1];
        if (!value) {
          fail('--option needs a value');
        }
        options.push(value);
        i += 1;
        break;
      }
      case '--pass-options': {
        const value = argv[i + 1];
        if (!value) {
          fail('--pass-options needs a value');
        }
        for (const item of value.split(',')) {
          const parsed = Number.parseInt(item.trim(), 10);
          if (!Number.isFinite(parsed) || parsed < 1) {
            fail(`Invalid option number in --pass-options: '${item}'`);
          }
          passOptions.add(parsed);
        }
        i += 1;
        break;
      }
      case '--gui':
        useGui = true;
        break;
      case '--help':
      case '-h':
        printHelp();
        process.exit(0);
        break;
      default:
        fail(`Unknown option: ${arg}`);
    }
  }

  if (!['action', 'choice', 'text'].includes(mode)) {
    fail(`Invalid --mode '${mode}'. Expected action, choice, or text.`);
  }
  if (!message) {
    fail('--message is required.');
  }
  if (mode === 'choice' && options.length < 2) {
    fail('--mode choice requires at least two --option values.');
  }

  return { mode, message, journalFile, options, passOptions, useGui };
}

function padTwo(value) {
  return String(value).padStart(2, '0');
}

function padThree(value) {
  return String(value).padStart(3, '0');
}

function formatLogTimestamp(date = new Date()) {
  const year = date.getFullYear();
  const month = padTwo(date.getMonth() + 1);
  const day = padTwo(date.getDate());
  const hours = padTwo(date.getHours());
  const minutes = padTwo(date.getMinutes());
  const seconds = padTwo(date.getSeconds());
  const millis = padThree(date.getMilliseconds());
  return `${year}-${month}-${day} ${hours}:${minutes}:${seconds}.${millis}`;
}

function appendJournal(journalFile, text) {
  const line = `${formatLogTimestamp()} [journal] ${text}\n`;
  if (journalFile) {
    fs.appendFileSync(journalFile, line, 'utf8');
  }
  return line;
}

function openTtyStreams() {
  if (process.stdin.isTTY && process.stdout.isTTY) {
    return { input: process.stdin, output: process.stdout, close: () => {} };
  }
  try {
    const fd = fs.openSync('/dev/tty', 'r+');
    const input = fs.createReadStream(null, { fd, autoClose: false });
    const output = fs.createWriteStream(null, { fd, autoClose: false });
    return {
      input,
      output,
      close: () => {
        input.destroy();
        output.destroy();
        try {
          fs.closeSync(fd);
        } catch {}
      },
    };
  } catch {
    return null;
  }
}

function askLine(rl, output, promptText) {
  return new Promise((resolve) => {
    output.write(promptText);
    rl.once('line', (line) => {
      resolve(line.trim());
    });
  });
}

function runAppleScript(script, args = []) {
  return execFileSync('osascript', ['-e', script, ...args], {
    encoding: 'utf8',
  }).trim();
}

async function runActionPrompt({ message, journalFile, tty }) {
  appendJournal(journalFile, `[prompt:action:start] ${message}`);

  if (!tty) {
    const script = `
      on run argv
        set promptMessage to item 1 of argv
        set dialogResult to display dialog promptMessage with title "Action Required" default answer "" buttons {"Abort Test", "Done"} default button "Done" cancel button "Abort Test"
        return text returned of dialogResult
      end run
    `;
    try {
      const note = runAppleScript(script, [message]);
      const suffix = note ? ` (note: ${note})` : '';
      appendJournal(journalFile, `[prompt:action:done] ${message}${suffix}`);
      return;
    } catch {
      appendJournal(journalFile, `[prompt:action:aborted] ${message}`);
      process.exit(1);
    }
  }

  const { input, output, close } = tty;
  const rl = readline.createInterface({ input, output });
  try {
    output.write(
      `\n── Action Required ───────────────────────────────────────────────────────\n` +
        `${message}\n` +
        `─────────────────────────────────────────────────────────────────────────\n` +
        `Perform the action above, optionally type a note, and press Enter\n` +
        `(or type 'abort' to fail the test):\n`
    );
    const note = await askLine(rl, output, '> ');
    if (note.toLowerCase() === 'abort') {
      appendJournal(journalFile, `[prompt:action:aborted] ${message}`);
      process.exit(1);
    }
    const suffix = note ? ` (note: ${note})` : '';
    appendJournal(journalFile, `[prompt:action:done] ${message}${suffix}`);
  } finally {
    rl.close();
    close();
  }
}

async function runTextPrompt({ message, journalFile, tty }) {
  appendJournal(journalFile, `[prompt:question] ${message}`);

  if (!tty) {
    const script = `
      on run argv
        set promptMessage to item 1 of argv
        set dialogResult to display dialog promptMessage with title "Human Observation" default answer "" buttons {"Cancel", "Submit"} default button "Submit" cancel button "Cancel"
        return text returned of dialogResult
      end run
    `;
    try {
      const answer = runAppleScript(script, [message]);
      appendJournal(
        journalFile,
        `[prompt:answer] ${message} -> ${answer || '(empty)'}`
      );
      return;
    } catch {
      appendJournal(journalFile, `[prompt:aborted] ${message}`);
      process.exit(1);
    }
  }

  const { input, output, close } = tty;
  const rl = readline.createInterface({ input, output });
  try {
    output.write(
      `\n── Human Observation ─────────────────────────────────────────────────────\n` +
        `${message}\n` +
        `─────────────────────────────────────────────────────────────────────────\n`
    );
    const answer = await askLine(rl, output, '> ');
    appendJournal(
      journalFile,
      `[prompt:answer] ${message} -> ${answer || '(empty)'}`
    );
  } finally {
    rl.close();
    close();
  }
}

async function runChoicePrompt({
  message,
  options,
  passOptions,
  journalFile,
  tty,
}) {
  appendJournal(journalFile, `[prompt:question] ${message}`);
  const customIndex = options.length + 1;

  let selectedIndex = 0;
  let selectedLabel = '';
  let extraNote = '';

  if (!tty) {
    const numberedChoices = [
      ...options.map((option, index) => `${index + 1}) ${option}`),
      `${customIndex}) Other (custom note)`,
    ];
    const chooseScript = `
      on run argv
        set promptMessage to item 1 of argv
        set choiceItems to rest of argv
        set chosen to choose from list choiceItems with title "Human Verification" with prompt promptMessage default items {item 1 of choiceItems}
        if chosen is false then error number -128
        return item 1 of chosen
      end run
    `;
    try {
      const chosenLine = runAppleScript(chooseScript, [
        message,
        ...numberedChoices,
      ]);
      selectedIndex = Number.parseInt(chosenLine.split(')')[0], 10);
      if (selectedIndex === customIndex) {
        const customScript = `
          on run argv
            set dialogResult to display dialog (item 1 of argv) with title "Custom Observation" default answer "" buttons {"Cancel", "Submit"} default button "Submit" cancel button "Cancel"
            return text returned of dialogResult
          end run
        `;
        selectedLabel = runAppleScript(customScript, [message]);
      } else {
        selectedLabel = options[selectedIndex - 1];
      }
    } catch {
      appendJournal(journalFile, `[prompt:aborted] ${message}`);
      process.exit(1);
    }
  } else {
    const { input, output, close } = tty;
    const rl = readline.createInterface({ input, output });
    try {
      output.write(
        `\n── Human Verification ────────────────────────────────────────────────────\n` +
          `${message}\n` +
          `─────────────────────────────────────────────────────────────────────────\n`
      );
      for (let i = 0; i < options.length; i += 1) {
        output.write(`  ${i + 1}) ${options[i]}\n`);
      }
      output.write(`  ${customIndex}) Other (provide custom answer)\n\n`);

      while (selectedIndex < 1 || selectedIndex > customIndex) {
        const rawChoice = await askLine(
          rl,
          output,
          `Select [1-${customIndex}]: `
        );
        const parsed = Number.parseInt(rawChoice, 10);
        if (Number.isFinite(parsed) && parsed >= 1 && parsed <= customIndex) {
          selectedIndex = parsed;
        }
      }

      if (selectedIndex === customIndex) {
        while (!selectedLabel) {
          selectedLabel = await askLine(rl, output, 'Enter custom answer: ');
        }
      } else {
        selectedLabel = options[selectedIndex - 1];
        extraNote = await askLine(
          rl,
          output,
          'Optional note (Enter to skip): '
        );
      }
    } finally {
      rl.close();
      close();
    }
  }

  const noteSuffix = extraNote ? ` (note: ${extraNote})` : '';
  appendJournal(
    journalFile,
    `[prompt:answer] ${message} -> [${selectedIndex}] ${selectedLabel}${noteSuffix}`
  );

  if (passOptions.size > 0 && !passOptions.has(selectedIndex)) {
    appendJournal(
      journalFile,
      `[test:failed] Human reported non-passing outcome [${selectedIndex}] ${selectedLabel}`
    );
    console.error(
      `Test failed based on human verification: [${selectedIndex}] ${selectedLabel}`
    );
    process.exit(1);
  }
}

const config = parseArguments(process.argv.slice(2));
const tty = config.useGui ? null : openTtyStreams();

if (config.mode === 'action') {
  await runActionPrompt({ ...config, tty });
} else if (config.mode === 'text') {
  await runTextPrompt({ ...config, tty });
} else {
  await runChoicePrompt({ ...config, tty });
}
