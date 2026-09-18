import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import { program } from 'commander';

import {
  formatCoverageComparisonMarkdown,
  formatCoverageMarkdown,
  updateDocsSection,
  writeReportFiles,
} from './wpt-results.mjs';

const harnessDir = path.dirname(fileURLToPath(import.meta.url));
const defaultMarkdownReportPath = path.join(
  harnessDir,
  '..',
  'results',
  'latest.md'
);
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
  .requiredOption(
    '--from-json <path>',
    'Structured JSON report produced by wpt-harness'
  )
  .option(
    '--baseline-json <path>',
    'Baseline JSON report (latest stable release run); renders the docs summary as a side-by-side comparison',
    null
  )
  .option(
    '--write-markdown <path>',
    'Output markdown report path',
    defaultMarkdownReportPath
  )
  .option(
    '--update-docs',
    'Replace the WPT summary block in the audiodocs Web Audio API coverage page',
    false
  );

program.parse(process.argv);
const options = program.opts();

const report = JSON.parse(fs.readFileSync(options.fromJson, 'utf8'));

writeReportFiles({
  report,
  jsonPath: null,
  markdownPath: options.writeMarkdown,
});

const NIGHTLY_DIST_TAG = 'audio-api-nightly';

/**
 * Label for the current-checkout column: the most recent published nightly,
 * which is what readers can actually install. Falls back to the report's own
 * version when npm is unreachable.
 */
function resolveCurrentColumnLabel(report) {
  try {
    const distTags = JSON.parse(
      execFileSync(
        'npm',
        ['view', 'react-native-audio-api', 'dist-tags', '--json'],
        { encoding: 'utf8', stdio: ['ignore', 'pipe', 'ignore'] }
      )
    );
    if (distTags[NIGHTLY_DIST_TAG]) {
      return `v${distTags[NIGHTLY_DIST_TAG]}`;
    }
  } catch {
    console.warn(
      `[wpt] Could not read the ${NIGHTLY_DIST_TAG} dist-tag from npm; labelling the current column from the report.`
    );
  }
  return report.libraryVersion ? `v${report.libraryVersion}` : 'main';
}

if (options.updateDocs) {
  const summary = options.baselineJson
    ? formatCoverageComparisonMarkdown(
        report,
        JSON.parse(fs.readFileSync(options.baselineJson, 'utf8')),
        resolveCurrentColumnLabel(report)
      )
    : formatCoverageMarkdown(report);
  updateDocsSection(defaultDocsPath, summary);
}

console.log(`Wrote markdown report: ${options.writeMarkdown}`);
if (options.updateDocs) {
  console.log(`Updated docs summary: ${defaultDocsPath}`);
}
