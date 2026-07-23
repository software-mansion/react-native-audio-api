import fs from 'node:fs';
import path from 'node:path';

function printUsageAndExit() {
  console.error(
    'Usage: node wpt-compare.mjs --baseline <base.json> --candidate <head.json>'
  );
  process.exit(1);
}

function parseArgs(argv) {
  const args = { baseline: null, candidate: null };
  for (let i = 0; i < argv.length; i += 1) {
    const arg = argv[i];
    if (arg === '--baseline') {
      args.baseline = argv[++i];
    } else if (arg === '--candidate') {
      args.candidate = argv[++i];
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

const options = parseArgs(process.argv.slice(2));
const baseline = loadReport(options.baseline);
const candidate = loadReport(options.candidate);

const baselineCategories = categoryPassMap(baseline);
const candidateCategories = categoryPassMap(candidate);

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

if (summaryDelta < 0) {
  regressions.push({
    key: '__summary__',
    label: 'Overall summary',
    base: baselineSummaryPass,
    head: candidateSummaryPass,
    delta: summaryDelta,
  });
} else if (summaryDelta > 0) {
  improvements.push({
    key: '__summary__',
    label: 'Overall summary',
    base: baselineSummaryPass,
    head: candidateSummaryPass,
    delta: summaryDelta,
  });
}

const formatRow = ({ label, base, head, delta }) => {
  const sign = delta > 0 ? '+' : '';
  return `| ${label} | ${base} | ${head} | ${sign}${delta} |`;
};

const lines = [
  '## WPT non-regression comparison',
  '',
  `Baseline: \`${options.baseline}\` · Candidate: \`${options.candidate}\``,
  '',
  `| Spec section | Base pass | Head pass | Delta |`,
  `| --- | ---: | ---: | ---: |`,
];

for (const row of [...regressions, ...improvements, ...unchanged]) {
  if (row.key === '__summary__') {
    continue;
  }
  lines.push(formatRow(row));
}

lines.push(
  `| **Overall summary** | ${baselineSummaryPass} | ${candidateSummaryPass} | ${
    summaryDelta > 0 ? '+' : ''
  }${summaryDelta} |`,
  ''
);

if (regressions.length > 0) {
  lines.push(`**Result: FAIL** — ${regressions.length} regression(s).`);
  console.error(`WPT regression: ${regressions.length} worse result(s) vs baseline.`);
  for (const row of regressions) {
    console.error(`  - ${row.label}: ${row.head} pass (was ${row.base}, delta ${row.delta})`);
  }
} else {
  lines.push('**Result: PASS** — all pass counts are equal or greater than baseline.');
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

process.exit(regressions.length > 0 ? 1 : 0);
