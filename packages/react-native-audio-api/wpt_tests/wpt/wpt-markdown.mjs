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

if (options.updateDocs) {
  const summary = options.baselineJson
    ? formatCoverageComparisonMarkdown(
        report,
        JSON.parse(fs.readFileSync(options.baselineJson, 'utf8'))
      )
    : formatCoverageMarkdown(report);
  updateDocsSection(defaultDocsPath, summary);
}

console.log(`Wrote markdown report: ${options.writeMarkdown}`);
if (options.updateDocs) {
  console.log(`Updated docs summary: ${defaultDocsPath}`);
}
