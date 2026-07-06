import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import { program } from 'commander';

import {
  formatCoverageMarkdown,
  updateDocsSection,
  writeReportFiles,
} from './wpt-results.mjs';

const harnessDir = path.dirname(fileURLToPath(import.meta.url));
const defaultMarkdownReportPath = path.join(harnessDir, '..', 'results', 'latest.md');
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
  .requiredOption('--from-json <path>', 'Structured JSON report produced by wpt-harness')
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
  updateDocsSection(defaultDocsPath, formatCoverageMarkdown(report));
}

console.log(`Wrote markdown report: ${options.writeMarkdown}`);
if (options.updateDocs) {
  console.log(`Updated docs summary: ${defaultDocsPath}`);
}
