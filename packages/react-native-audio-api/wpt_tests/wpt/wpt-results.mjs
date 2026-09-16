import { createRequire } from 'node:module';
import fs from 'node:fs';
import path from 'node:path';

import {
  getSkippedByPolicy,
  normalizeTestPath,
  testsPath,
  walkHtmlFiles,
} from './wpt-shared.mjs';

const require = createRequire(import.meta.url);

const DOCS_MARKERS = {
  start: '{/* wpt-summary:start */}',
  end: '{/* wpt-summary:end */}',
};

// WPT test categories that map to interfaces actually available (fully or
// partially) in react-native-audio-api. Keep this in sync with the coverage
// table in packages/audiodocs/docs/other/web-audio-api-coverage.mdx.
// Categories that are not yet implemented (e.g. PannerNode,
// DynamicsCompressorNode, MediaStream* nodes, AudioWorklet) are intentionally
// excluded from the docs summary.
const AVAILABLE_INTERFACES = {
  'the-analysernode-interface': 'AnalyserNode',
  'the-audiobuffer-interface': 'AudioBuffer',
  'the-audiobuffersourcenode-interface': 'AudioBufferSourceNode',
  'the-audiocontext-interface': 'AudioContext',
  'the-audionode-interface': 'AudioNode',
  'the-audioparam-interface': 'AudioParam',
  'the-biquadfilternode-interface': 'BiquadFilterNode',
  'the-channelmergernode-interface': 'ChannelMergerNode',
  'the-channelsplitternode-interface': 'ChannelSplitterNode',
  'the-constantsourcenode-interface': 'ConstantSourceNode',
  'the-convolvernode-interface': 'ConvolverNode',
  'the-delaynode-interface': 'DelayNode',
  'the-destinationnode-interface': 'AudioDestinationNode',
  'the-gainnode-interface': 'GainNode',
  'the-iirfilternode-interface': 'IIRFilterNode',
  'the-mediaelementaudiosourcenode-interface': 'MediaElementAudioSourceNode',
  'the-offlineaudiocontext-interface': 'OfflineAudioContext',
  'the-oscillatornode-interface': 'OscillatorNode',
  'the-periodicwave-interface': 'PeriodicWave',
  'the-stereopanner-interface': 'StereoPannerNode',
  'the-waveshapernode-interface': 'WaveShaperNode',
};

// Optional per-interface explanations for the remaining WPT failures, keyed by
// the same spec-section key as AVAILABLE_INTERFACES. Rendered in the "Notes"
// column of the coverage table.
const COVERAGE_NOTES = {
  'the-analysernode-interface':
    'One failure: minDecibels/maxDecibels are stored as float, so reading back a ' +
    'high-precision double value loses a few digits (cosmetic round-trip mismatch).',
  'the-audiobuffer-interface':
    'Remaining failures: acquire-the-content expects AudioBufferSourceNode.start() to ' +
    'snapshot the buffer contents (we still share the underlying memory).',
  'the-biquadfilternode-interface':
    'The filter runs in double precision, so the static frequency response matches the ' +
    'reference. The remaining failures are the parameter-automation tests, which expect ' +
    'per-sample (a-rate) coefficient updates; we sample the params once per 128-frame ' +
    'render quantum (k-rate) and recompute the coefficients once per block. The audible ' +
    'difference is negligible, while computing the coefficient/trig math once per block ' +
    'instead of once per sample makes processing dramatically faster on the audio thread.',
  'the-audionode-interface':
    'Channel mixing, indexed connect()/disconnect(), and ChannelMerger/Splitter ' +
    'graphs pass. Remaining failures are connect() chaining across unimplemented ' +
    'node types (DynamicsCompressor, Panner, ScriptProcessor, MediaStream*) and ' +
    'legacy 3-arg AudioContext construction (should throw TypeError).',
};

const CATEGORY_LABELS = {
  'processing-model': 'Processing model',
  'the-analysernode-interface': 'AnalyserNode',
  'the-audiobuffer-interface': 'AudioBuffer',
  'the-audiobuffersourcenode-interface': 'AudioBufferSourceNode',
  'the-audiocontext-interface': 'AudioContext',
  'the-audionode-interface': 'AudioNode',
  'the-audioparam-interface': 'AudioParam',
  'the-audioworklet-interface': 'AudioWorklet',
  'the-biquadfilternode-interface': 'BiquadFilterNode',
  'the-channelmergernode-interface': 'ChannelMergerNode',
  'the-channelsplitternode-interface': 'ChannelSplitterNode',
  'the-constantsourcenode-interface': 'ConstantSourceNode',
  'the-convolvernode-interface': 'ConvolverNode',
  'the-delaynode-interface': 'DelayNode',
  'the-destinationnode-interface': 'DestinationNode',
  'the-dynamicscompressornode-interface': 'DynamicsCompressorNode',
  'the-gainnode-interface': 'GainNode',
  'the-iirfilternode-interface': 'IIRFilterNode',
  'the-mediaelementaudiosourcenode-interface': 'MediaElementAudioSourceNode',
  'the-mediastreamaudiodestinationnode-interface':
    'MediaStreamAudioDestinationNode',
  'the-mediastreamaudiosourcenode-interface': 'MediaStreamAudioSourceNode',
  'the-offlineaudiocontext-interface': 'OfflineAudioContext',
  'the-oscillatornode-interface': 'OscillatorNode',
  'the-pannernode-interface': 'PannerNode',
  'the-periodicwave-interface': 'PeriodicWave',
  'the-scriptprocessornode-interface': 'ScriptProcessorNode',
  'the-stereopanner-interface': 'StereoPannerNode',
  'the-waveshapernode-interface': 'WaveShaperNode',
  'media-playback-while-not-visible-permission-policy':
    'Media playback visibility policy',
  'root': 'Other',
};

export function getCategoryKey(testPath) {
  const normalized = normalizeTestPath(testPath);
  const parts = normalized.split('/');

  if (parts[0] === 'the-audio-api' && parts.length >= 3) {
    return parts[1];
  }

  if (parts[0] === 'the-audio-api' && parts.length === 2) {
    return 'root';
  }

  if (parts.length >= 2) {
    return parts.slice(0, -1).join('/');
  }

  return 'root';
}

export function getCategoryLabel(categoryKey) {
  return CATEGORY_LABELS[categoryKey] ?? categoryKey;
}

export function getCategoryAbbrev(categoryKey) {
  if (categoryKey.startsWith('the-') && categoryKey.endsWith('-interface')) {
    return categoryKey.slice(4, -'-interface'.length);
  }
  return categoryKey.replace(/^the-/, '').replace(/-interface$/, '');
}

function createEmptyCategoryStats() {
  return {
    pass: 0,
    fail: 0,
    files: 0,
    filesPassed: 0,
    filesFailed: 0,
    filesCrashed: 0,
    runnableFiles: 0,
    skippedFiles: 0,
    skipReason: null,
  };
}

export function discoverAudioApiCategories({
  profileAllowlist,
  includeCrashtests,
}) {
  const categories = new Map();

  for (const file of walkHtmlFiles(testsPath)) {
    const fullName = normalizeTestPath(file);
    if (!profileAllowlist.some((prefix) => fullName.includes(prefix))) {
      continue;
    }

    const categoryKey = getCategoryKey(fullName);
    const stats = categories.get(categoryKey) ?? createEmptyCategoryStats();

    const skip = getSkippedByPolicy(fullName);
    if (skip != null) {
      stats.skippedFiles += 1;
      stats.skipReason ??= skip.reason;
    } else if (!includeCrashtests && fullName.includes('/crashtests/')) {
      stats.skippedFiles += 1;
      stats.skipReason ??= 'excluded in smoke profile';
    } else {
      stats.runnableFiles += 1;
    }

    categories.set(categoryKey, stats);
  }

  return [...categories.entries()]
    .sort(([a], [b]) => a.localeCompare(b))
    .map(([key, meta]) => ({ key, ...meta }));
}

// Bounds for the per-file failure lists stored in the JSON report, so a
// catastrophic run cannot balloon the CI artifact.
const MAX_FAILURES_PER_FILE = 100;
const MAX_FAILURE_MESSAGE_LENGTH = 200;

export class WptResultsCollector {
  #categories = new Map();
  #files = [];
  #currentFile = null;

  #ensureCategory(categoryKey) {
    if (!this.#categories.has(categoryKey)) {
      this.#categories.set(categoryKey, createEmptyCategoryStats());
    }
    return this.#categories.get(categoryKey);
  }

  startSuite(name) {
    this.#finishFile('ok');

    const suitePath = normalizeTestPath(name);
    this.#currentFile = {
      path: suitePath,
      category: getCategoryKey(suitePath),
      pass: 0,
      fail: 0,
      failures: [],
      startedAt: Date.now(),
    };

    const category = this.#ensureCategory(this.#currentFile.category);
    category.files += 1;
  }

  pass() {
    if (this.#currentFile == null) {
      return;
    }
    this.#currentFile.pass += 1;
    this.#ensureCategory(this.#currentFile.category).pass += 1;
  }

  fail(message) {
    if (this.#currentFile == null) {
      return;
    }
    this.#currentFile.fail += 1;
    if (
      typeof message === 'string' &&
      this.#currentFile.failures.length < MAX_FAILURES_PER_FILE
    ) {
      this.#currentFile.failures.push(
        message.slice(0, MAX_FAILURE_MESSAGE_LENGTH)
      );
    }
    this.#ensureCategory(this.#currentFile.category).fail += 1;
  }

  /**
   * Record that the file currently running (or the named file, if none is
   * in flight) died without finishing — the worker process crashed or the
   * inactivity watchdog killed it. Counted as a failed file in its category.
   *
   * @param {string} path
   * @param {'crashed' | 'timeout'} status
   */
  markFileCrashed(path, status) {
    const suitePath = normalizeTestPath(path);
    if (this.#currentFile?.path === suitePath) {
      this.#finishFile(status);
      return;
    }

    const category = this.#ensureCategory(getCategoryKey(suitePath));
    category.files += 1;
    category.filesFailed += 1;
    category.filesCrashed += 1;
    this.#files.push({
      path: suitePath,
      pass: 0,
      fail: 0,
      failures: [],
      status,
      durationMs: null,
    });
  }

  #finishFile(status) {
    if (this.#currentFile == null) {
      return;
    }

    const file = this.#currentFile;
    this.#currentFile = null;

    const category = this.#ensureCategory(file.category);
    if (status !== 'ok') {
      category.filesFailed += 1;
      category.filesCrashed += 1;
    } else if (file.fail === 0 && file.pass > 0) {
      category.filesPassed += 1;
    } else if (file.fail > 0) {
      category.filesFailed += 1;
    }

    this.#files.push({
      path: file.path,
      pass: file.pass,
      fail: file.fail,
      failures: file.failures,
      status,
      durationMs: Date.now() - file.startedAt,
    });
  }

  finalize() {
    this.#finishFile('ok');
  }

  getCategoryStats() {
    this.finalize();
    return this.#categories;
  }

  /** Per-file outcomes in run order, for the exact non-regression compare. */
  getFileResults() {
    this.finalize();
    return this.#files;
  }
}

function formatRate(pass, total) {
  if (total === 0) {
    return '—';
  }
  return `${((pass / total) * 100).toFixed(1)}%`;
}

function formatCell(pass, total, skipped, skipReason) {
  if (skipped && total === 0) {
    return '—';
  }
  if (total === 0) {
    return '—';
  }
  return `${pass}/${total}`;
}

export function buildReport({
  collector,
  profile,
  filterRegexp,
  includeCrashtests,
  profileAllowlist,
  durationMs = null,
}) {
  collector.finalize();

  const discovered = discoverAudioApiCategories({
    profileAllowlist,
    includeCrashtests,
  });
  const runStats = collector.getCategoryStats();

  const categories = discovered.map(
    ({ key, skipReason, runnableFiles, skippedFiles }) => {
      const run = runStats.get(key) ?? createEmptyCategoryStats();
      const pass = run.pass;
      const fail = run.fail;
      const total = pass + fail;
      const skipped = runnableFiles === 0 && skippedFiles > 0;

      return {
        key,
        label: getCategoryLabel(key),
        abbrev: getCategoryAbbrev(key),
        pass,
        fail,
        total,
        passRate: total > 0 ? Number(((pass / total) * 100).toFixed(1)) : null,
        files: run.files,
        filesPassed: run.filesPassed,
        filesFailed: run.filesFailed,
        filesCrashed: run.filesCrashed,
        runnableFiles,
        skippedFiles,
        skipped,
        skipReason: skipped ? skipReason : null,
      };
    }
  );

  const summary = categories.reduce(
    (acc, category) => {
      if (category.skipped) {
        acc.skippedCategories += 1;
        return acc;
      }
      acc.pass += category.pass;
      acc.fail += category.fail;
      acc.files += category.files;
      acc.filesPassed += category.filesPassed;
      acc.filesFailed += category.filesFailed;
      acc.filesCrashed += category.filesCrashed;
      acc.runnableFiles += category.runnableFiles;
      return acc;
    },
    {
      pass: 0,
      fail: 0,
      files: 0,
      filesPassed: 0,
      filesFailed: 0,
      filesCrashed: 0,
      runnableFiles: 0,
      skippedCategories: 0,
    }
  );

  summary.total = summary.pass + summary.fail;
  summary.passRate =
    summary.total > 0
      ? Number(((summary.pass / summary.total) * 100).toFixed(1))
      : null;

  // Resolved lazily so that reading an existing JSON report (wpt-markdown.mjs)
  // never triggers the RN_AUDIO_API_APP_ROOT package lookup.
  const packageRoot = require('../package-root.js');
  const libraryVersion = JSON.parse(
    fs.readFileSync(path.join(packageRoot, 'package.json'), 'utf8')
  ).version;

  return {
    generatedAt: new Date().toISOString(),
    engine: 'react-native-audio-api (Node WPT harness)',
    libraryVersion,
    reference:
      'https://wpt.fyi/results/webaudio/the-audio-api?label=experimental&label=master&aligned',
    profile,
    filterRegexp,
    includeCrashtests,
    durationMs,
    summary,
    categories,
    files: collector.getFileResults(),
  };
}

export function formatMarkdownReport(report) {
  const generated = new Date(report.generatedAt).toISOString().slice(0, 10);
  const alignedCategories = report.categories;

  const headerCells = alignedCategories.map((category) => category.abbrev);
  const valueCells = alignedCategories.map((category) =>
    formatCell(
      category.pass,
      category.total,
      category.skipped,
      category.skipReason
    )
  );

  const alignedHeader = ['', ...headerCells, '**Total**'].join(' | ');
  const alignedSeparator = [
    '---',
    ...headerCells.map(() => ':---:'),
    ':---:',
  ].join(' | ');
  const alignedValues = [
    'react-native-audio-api',
    ...valueCells,
    formatCell(report.summary.pass, report.summary.total, false),
  ].join(' | ');

  const detailRows = report.categories
    .map((category) => {
      if (category.skipped) {
        return `| ${category.label} | — | — | — | skipped |`;
      }
      if (category.total === 0) {
        return `| ${category.label} | — | — | — | — |`;
      }
      return `| ${category.label} | ${category.pass} | ${category.total} | ${formatRate(category.pass, category.total)} | ${category.filesPassed}/${category.files} files |`;
    })
    .join('\n');

  const skippedNotes = report.categories
    .filter((category) => category.skipped && category.skipReason)
    .map((category) => `- **${category.label}**: ${category.skipReason}`)
    .join('\n');

  return [
    `Updated: ${generated} · Profile: \`${report.profile}\` · Assertions: **${report.summary.pass}/${report.summary.total} (${formatRate(report.summary.pass, report.summary.total)})**`,
    '',
    'Aligned overview (similar to [wpt.fyi](https://wpt.fyi/results/webaudio/the-audio-api?label=experimental&label=master&aligned)):',
    '',
    `| ${alignedHeader} |`,
    `| ${alignedSeparator} |`,
    `| ${alignedValues} |`,
    '',
    'Detailed results:',
    '',
    '| Spec section | Pass | Total | Rate | Test files |',
    '| --- | ---: | ---: | ---: | ---: |',
    detailRows,
    '',
    skippedNotes ? 'Skipped sections:\n\n' + skippedNotes : null,
    '',
    `Runnable HTML tests in profile: ${report.summary.runnableFiles}. Skipped categories: ${report.summary.skippedCategories}.`,
  ]
    .filter((section) => section != null)
    .join('\n');
}

// Red -> yellow -> green gradient keyed on pass ratio, mirroring the wpt.fyi
// results dashboard. Higher saturation and mid lightness keep rows vivid but
// still readable with dark cell text in light and dark docs themes.
function colorForRate(pass, total) {
  if (total === 0) {
    return 'transparent';
  }
  const hue = Math.round((pass / total) * 120); // 0 = red, 120 = green
  return `hsl(${hue}, 85%, 72%)`;
}

function coverageRow({ label, pass, total, note = '', bold = false }) {
  const name = bold ? `<strong>${label}</strong>` : label;
  const rate = bold
    ? `<strong>${formatRate(pass, total)}</strong>`
    : formatRate(pass, total);
  const style = `{{ backgroundColor: '${colorForRate(pass, total)}', color: '#1c1e21' }}`;
  return (
    `    <tr style=${style}>` +
    `<td>${name}</td>` +
    `<td style={{ textAlign: 'right' }}>${pass}</td>` +
    `<td style={{ textAlign: 'right' }}>${total}</td>` +
    `<td style={{ textAlign: 'right' }}>${rate}</td>` +
    `<td>${note}</td>` +
    `</tr>`
  );
}

/**
 * Compact summary for the audiodocs coverage page: only interfaces that are
 * available in react-native-audio-api, with passing count, total, and pass rate.
 * Rendered as a JSX table so each row can be shaded by pass rate (wpt.fyi style).
 */
export function formatCoverageMarkdown(report) {
  const generated = new Date(report.generatedAt).toISOString().slice(0, 10);

  const rows = report.categories
    .filter(
      (category) =>
        AVAILABLE_INTERFACES[category.key] != null && category.total > 0
    )
    .map((category) => ({
      label: AVAILABLE_INTERFACES[category.key],
      pass: category.pass,
      total: category.total,
      note: COVERAGE_NOTES[category.key] ?? '',
    }))
    .sort((a, b) => a.label.localeCompare(b.label));

  const totals = rows.reduce(
    (acc, row) => {
      acc.pass += row.pass;
      acc.total += row.total;
      return acc;
    },
    { pass: 0, total: 0 }
  );

  const bodyRows = rows.map((row) => coverageRow(row)).join('\n');
  const totalRow = coverageRow({ ...totals, label: 'Total', bold: true });

  return [
    `_Last updated: ${generated}._`,
    '',
    '<table>',
    '  <thead>',
    '    <tr>',
    '      <th>Interface</th>',
    "      <th style={{ textAlign: 'right' }}>Passing</th>",
    "      <th style={{ textAlign: 'right' }}>Total</th>",
    "      <th style={{ textAlign: 'right' }}>Pass rate</th>",
    '      <th>Differences</th>',
    '    </tr>',
    '  </thead>',
    '  <tbody>',
    bodyRows,
    totalRow,
    '  </tbody>',
    '</table>',
  ].join('\n');
}

function comparisonCell(stats, bold = false) {
  if (stats == null || stats.total === 0) {
    return "<td style={{ textAlign: 'right' }}>—</td>";
  }
  const text = `${stats.pass}/${stats.total} (${formatRate(stats.pass, stats.total)})`;
  const content = bold ? `<strong>${text}</strong>` : text;
  const style = `{{ backgroundColor: '${colorForRate(stats.pass, stats.total)}', color: '#1c1e21', textAlign: 'right' }}`;
  return `<td style=${style}>${content}</td>`;
}

function comparisonRow({ label, baseline, current, note = '', bold = false }) {
  const name = bold ? `<strong>${label}</strong>` : label;
  return (
    `    <tr><td>${name}</td>` +
    comparisonCell(baseline, bold) +
    comparisonCell(current, bold) +
    `<td>${note}</td></tr>`
  );
}

function availableInterfaceStats(report) {
  return new Map(
    report.categories
      .filter(
        (category) =>
          AVAILABLE_INTERFACES[category.key] != null && category.total > 0
      )
      .map((category) => [
        category.key,
        { pass: category.pass, total: category.total },
      ])
  );
}

function sumStats(statsByKey) {
  let pass = 0;
  let total = 0;
  for (const stats of statsByKey.values()) {
    pass += stats.pass;
    total += stats.total;
  }
  return { pass, total };
}

/**
 * Side-by-side docs summary: the latest stable release (baseline report,
 * produced with RN_AUDIO_API_APP_ROOT pointing at an app that installs the
 * published npm package) next to the current main checkout. An interface
 * missing from one run renders as "—" in that column.
 */
export function formatCoverageComparisonMarkdown(
  currentReport,
  baselineReport
) {
  const generated = new Date(currentReport.generatedAt)
    .toISOString()
    .slice(0, 10);
  const baselineLabel = baselineReport.libraryVersion
    ? `v${baselineReport.libraryVersion} (latest)`
    : 'latest release';

  const baselineStats = availableInterfaceStats(baselineReport);
  const currentStats = availableInterfaceStats(currentReport);
  const keys = [
    ...new Set([...baselineStats.keys(), ...currentStats.keys()]),
  ].sort((a, b) =>
    AVAILABLE_INTERFACES[a].localeCompare(AVAILABLE_INTERFACES[b])
  );

  const bodyRows = keys
    .map((key) =>
      comparisonRow({
        label: AVAILABLE_INTERFACES[key],
        baseline: baselineStats.get(key),
        current: currentStats.get(key),
        note: COVERAGE_NOTES[key] ?? '',
      })
    )
    .join('\n');
  const totalRow = comparisonRow({
    label: 'Total',
    baseline: sumStats(baselineStats),
    current: sumStats(currentStats),
    bold: true,
  });

  return [
    `_Last updated: ${generated}._`,
    '',
    '<table>',
    '  <thead>',
    '    <tr>',
    '      <th>Interface</th>',
    `      <th style={{ textAlign: 'right' }}>${baselineLabel}</th>`,
    "      <th style={{ textAlign: 'right' }}>main</th>",
    '      <th>Differences</th>',
    '    </tr>',
    '  </thead>',
    '  <tbody>',
    bodyRows,
    totalRow,
    '  </tbody>',
    '</table>',
  ].join('\n');
}

export function writeReportFiles({ report, jsonPath, markdownPath }) {
  if (jsonPath) {
    fs.mkdirSync(path.dirname(jsonPath), { recursive: true });
    fs.writeFileSync(jsonPath, `${JSON.stringify(report, null, 2)}\n`);
  }

  const markdown = formatMarkdownReport(report);
  if (markdownPath) {
    fs.mkdirSync(path.dirname(markdownPath), { recursive: true });
    fs.writeFileSync(markdownPath, `${markdown}\n`);
  }

  return markdown;
}

/**
 * Replaces the marked WPT summary block in the audiodocs coverage page. The
 * surrounding explanatory prose lives in the .mdx file and is left untouched;
 * only the content between the markers is regenerated.
 */
export function updateDocsSection(docsPath, markdown) {
  const contents = fs.readFileSync(docsPath, 'utf8');
  const block = `${DOCS_MARKERS.start}\n\n${markdown}\n\n${DOCS_MARKERS.end}`;

  if (
    contents.includes(DOCS_MARKERS.start) &&
    contents.includes(DOCS_MARKERS.end)
  ) {
    const pattern = new RegExp(
      `${escapeRegExp(DOCS_MARKERS.start)}[\\s\\S]*?${escapeRegExp(DOCS_MARKERS.end)}`,
      'm'
    );
    fs.writeFileSync(docsPath, contents.replace(pattern, block));
    return;
  }

  throw new Error(
    `Could not find WPT summary markers in ${docsPath}. ` +
      `Expected "${DOCS_MARKERS.start}" and "${DOCS_MARKERS.end}".`
  );
}

function escapeRegExp(value) {
  return value.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

export function wrapReporter(reporter, collector) {
  return {
    startSuite: (name) => {
      collector.startSuite(name);
      reporter.startSuite?.(name);
    },
    pass: (message) => {
      collector.pass();
      reporter.pass?.(message);
    },
    fail: (message) => {
      collector.fail(message);
      reporter.fail?.(message);
    },
    reportStack: (stack) => {
      reporter.reportStack?.(stack);
    },
  };
}
