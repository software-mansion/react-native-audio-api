import fs from 'node:fs';
import path from 'node:path';

function printUsageAndExit() {
  console.error(
    'Usage: node wpt-compare.mjs --baseline <base.json> --candidate <head.json> ' +
      '[--markdown-out <report.md>] [--baseline-label <text>] [--candidate-label <text>]'
  );
  process.exit(1);
}

function parseArgs(argv) {
  const args = {
    baseline: null,
    candidate: null,
    markdownOut: null,
    baselineLabel: null,
    candidateLabel: null,
  };
  for (let i = 0; i < argv.length; i += 1) {
    const arg = argv[i];
    if (arg === '--baseline') {
      args.baseline = argv[++i];
    } else if (arg === '--candidate') {
      args.candidate = argv[++i];
    } else if (arg === '--markdown-out') {
      args.markdownOut = argv[++i];
    } else if (arg === '--baseline-label') {
      args.baselineLabel = argv[++i];
    } else if (arg === '--candidate-label') {
      args.candidateLabel = argv[++i];
    } else if (arg === '--help' || arg === '-h') {
      printUsageAndExit();
    } else {
      console.error(`Unknown argument: ${arg}`);
      printUsageAndExit();
    }
  }
  if (!args.baseline || !args.candidate) {
    printUsageAndExit();
  }
  args.baselineLabel ??= args.baseline;
  args.candidateLabel ??= args.candidate;
  return args;
}

function loadReport(reportPath) {
  const absolute = path.resolve(reportPath);
  if (!fs.existsSync(absolute)) {
    console.error(`Report not found: ${absolute}`);
    process.exit(1);
  }

  const report = JSON.parse(fs.readFileSync(absolute, 'utf8'));
  if (!report?.summary || !Array.isArray(report.categories)) {
    console.error(`Invalid WPT report (missing summary/categories): ${absolute}`);
    process.exit(1);
  }
  return report;
}

function categoryPassMap(report) {
  const map = new Map();
  for (const category of report.categories) {
    if (category.skipped) {
      continue;
    }
    map.set(category.key, {
      label: category.label ?? category.key,
      pass: category.pass ?? 0,
      total: category.total ?? 0,
    });
  }
  return map;
}

function appendStepSummary(markdown) {
  const summaryPath = process.env.GITHUB_STEP_SUMMARY;
  if (!summaryPath) {
    return;
  }
  fs.appendFileSync(summaryPath, `${markdown}\n`);
}

function writeMarkdownReport(markdown, outputPath) {
  if (!outputPath) {
    return;
  }
  const absolute = path.resolve(outputPath);
  fs.mkdirSync(path.dirname(absolute), { recursive: true });
  fs.writeFileSync(absolute, `${markdown}\n`);
}

const options = parseArgs(process.argv.slice(2));
const baseline = loadReport(options.baseline);
const candidate = loadReport(options.candidate);

const baselineCategories = categoryPassMap(baseline);
const candidateCategories = categoryPassMap(candidate);

/**
 * Exact per-file comparison. Category pass counts cannot see an equal-count
 * swap (test X regresses while test Y starts passing), so when both reports
 * carry per-file data — reports written before it exists fall back to the
 * category-level gate — the failing-subtest sets are compared directly.
 */
function compareFiles(baselineReport, candidateReport) {
  if (!Array.isArray(baselineReport.files) || !Array.isArray(candidateReport.files)) {
    return null;
  }

  const toMap = (report) =>
    new Map(report.files.map((file) => [file.path, file]));
  const baseFiles = toMap(baselineReport);
  const headFiles = toMap(candidateReport);

  const newFailures = []; // { path, subtests: string[] }
  const brokenFiles = []; // { path, status } — crashed/hung/missing in candidate
  const newFiles = []; // informational: files only the candidate ran

  for (const [filePath, base] of baseFiles) {
    const head = headFiles.get(filePath);
    if (head == null) {
      brokenFiles.push({ path: filePath, status: 'missing' });
      continue;
    }
    if (head.status !== 'ok' && base.status === 'ok') {
      brokenFiles.push({ path: filePath, status: head.status });
      continue;
    }

    const baseFailures = new Set(base.failures ?? []);
    const subtests = (head.failures ?? []).filter(
      (message) => !baseFailures.has(message)
    );
    if (subtests.length > 0) {
      newFailures.push({ path: filePath, subtests });
    }
  }

  for (const filePath of headFiles.keys()) {
    if (!baseFiles.has(filePath)) {
      newFiles.push(filePath);
    }
  }

  return { newFailures, brokenFiles, newFiles };
}

const fileComparison = compareFiles(baseline, candidate);

const regressions = [];
const improvements = [];
const unchanged = [];

for (const [key, base] of baselineCategories) {
  const head = candidateCategories.get(key) ?? {
    label: base.label,
    pass: 0,
    total: 0,
  };
  const delta = head.pass - base.pass;

  if (delta < 0) {
    regressions.push({ key, label: base.label, base: base.pass, head: head.pass, delta });
  } else if (delta > 0) {
    improvements.push({ key, label: base.label, base: base.pass, head: head.pass, delta });
  } else {
    unchanged.push({ key, label: base.label, base: base.pass, head: head.pass, delta });
  }
}

for (const [key, head] of candidateCategories) {
  if (baselineCategories.has(key)) {
    continue;
  }
  if (head.pass > 0) {
    improvements.push({
      key,
      label: head.label,
      base: 0,
      head: head.pass,
      delta: head.pass,
    });
  }
}

const baselineSummaryPass = baseline.summary.pass ?? 0;
const candidateSummaryPass = candidate.summary.pass ?? 0;
const summaryDelta = candidateSummaryPass - baselineSummaryPass;

// A category present in the baseline but skipped in the candidate run drops out of both
// pass maps, so the per-category diff stays clean while the overall pass count falls.
// The summary therefore needs a regression check of its own.
const summaryRegressed = summaryDelta < 0;
const fileRegressed =
  fileComparison != null &&
  (fileComparison.newFailures.length > 0 || fileComparison.brokenFiles.length > 0);
const hasRegression = regressions.length > 0 || summaryRegressed || fileRegressed;

const signed = (delta) => `${delta > 0 ? '+' : ''}${delta}`;

const formatRow = ({ label, base, head, delta }) =>
  `| ${label} | ${base} | ${head} | ${signed(delta)} |`;

const tableHeader = [
  '| Spec section | Base pass | Head pass | Delta |',
  '| --- | ---: | ---: | ---: |',
];

const regressionParts = [];
if (regressions.length > 0) {
  regressionParts.push(`${regressions.length} regressed section(s)`);
}
if (fileComparison != null && fileComparison.newFailures.length > 0) {
  regressionParts.push(
    `${fileComparison.newFailures.length} file(s) with new failing subtests`
  );
}
if (fileComparison != null && fileComparison.brokenFiles.length > 0) {
  regressionParts.push(`${fileComparison.brokenFiles.length} broken file(s)`);
}
if (regressionParts.length === 0 && summaryRegressed) {
  regressionParts.push('overall pass count dropped');
}

const verdict = hasRegression
  ? `**FAIL** — ${regressionParts.join(', ')}`
  : '**PASS** — no regressions';

const lines = [
  '## WPT non-regression comparison',
  '',
  `${verdict} · ${improvements.length} improved section(s) · overall ${baselineSummaryPass} → ${candidateSummaryPass} (${signed(summaryDelta)})`,
  '',
];

const changed = [...regressions, ...improvements];
if (changed.length > 0) {
  lines.push(...tableHeader, ...changed.map(formatRow), '');
}

if (fileComparison == null) {
  lines.push(
    '_Per-file data unavailable in one of the reports — category-level comparison only._',
    ''
  );
} else {
  const MAX_LISTED_SUBTESTS = 30;
  const { newFailures, brokenFiles, newFiles } = fileComparison;

  if (brokenFiles.length > 0) {
    lines.push('**Broken test files:**', '');
    for (const { path: filePath, status } of brokenFiles) {
      lines.push(`- \`${filePath}\` — ${status}`);
    }
    lines.push('');
  }

  if (newFailures.length > 0) {
    lines.push('**New failing subtests:**', '');
    let listed = 0;
    for (const { path: filePath, subtests } of newFailures) {
      lines.push(`- \`${filePath}\``);
      for (const subtest of subtests) {
        if (listed >= MAX_LISTED_SUBTESTS) {
          break;
        }
        lines.push(`  - ${subtest}`);
        listed += 1;
      }
    }
    const totalSubtests = newFailures.reduce(
      (acc, { subtests }) => acc + subtests.length,
      0
    );
    if (totalSubtests > MAX_LISTED_SUBTESTS) {
      lines.push(`  - _…and ${totalSubtests - MAX_LISTED_SUBTESTS} more_`);
    }
    lines.push('');
  }

  if (newFiles.length > 0) {
    lines.push(
      `<sub>${newFiles.length} test file(s) ran only in the candidate (informational).</sub>`,
      ''
    );
  }
}

// Unchanged sections outnumber changed ones on almost every run, so they are collapsed
// to keep the PR comment readable without dropping the full picture.
if (unchanged.length > 0) {
  lines.push(
    `<details><summary>Unchanged sections (${unchanged.length})</summary>`,
    '',
    ...tableHeader,
    ...unchanged.map(formatRow),
    '',
    '</details>',
    ''
  );
}

lines.push(
  `<sub>Baseline: \`${options.baselineLabel}\` · Candidate: \`${options.candidateLabel}\`</sub>`
);

if (hasRegression) {
  console.error(`WPT regression: ${regressions.length} worse result(s) vs baseline.`);
  for (const row of regressions) {
    console.error(`  - ${row.label}: ${row.head} pass (was ${row.base}, delta ${row.delta})`);
  }
  if (fileComparison != null) {
    for (const { path: filePath, status } of fileComparison.brokenFiles) {
      console.error(`  - ${filePath}: ${status}`);
    }
    for (const { path: filePath, subtests } of fileComparison.newFailures) {
      console.error(`  - ${filePath}: ${subtests.length} new failing subtest(s)`);
      for (const subtest of subtests.slice(0, 5)) {
        console.error(`      ${subtest}`);
      }
    }
  }
  if (summaryRegressed) {
    console.error(
      `  - Overall summary: ${candidateSummaryPass} pass (was ${baselineSummaryPass}, delta ${summaryDelta})`
    );
  }
} else {
  console.log('WPT non-regression check passed.');
}

if (improvements.length > 0) {
  console.log(`Improvements (${improvements.length}):`);
  for (const row of improvements) {
    console.log(`  - ${row.label}: ${row.head} pass (was ${row.base}, +${row.delta})`);
  }
}

const markdown = lines.join('\n');
console.log(`\n${markdown}\n`);
appendStepSummary(markdown);
writeMarkdownReport(markdown, options.markdownOut);

process.exit(hasRegression ? 1 : 0);
