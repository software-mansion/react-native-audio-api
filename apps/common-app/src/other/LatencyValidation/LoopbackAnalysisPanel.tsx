import React, { FC } from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { Card } from '../../components';
import * as TestUI from '../../testComponents';
import { colors, layout, typography } from '../../styles';
import CorrelationPlot from './CorrelationPlot';
import { formatMs } from './helpers';
import type { LoopbackAnalysis } from './types';
import WaveformPlot from './WaveformPlot';

interface LoopbackAnalysisPanelProps {
  analysis: LoopbackAnalysis | null;
}

const InfoRow: FC<{ label: string; value: string }> = ({ label, value }) => (
  <View style={styles.infoRow}>
    <Text style={styles.infoLabel}>{label}</Text>
    <Text style={styles.infoValue}>{value}</Text>
  </View>
);

const LoopbackAnalysisPanel: FC<LoopbackAnalysisPanelProps> = ({
  analysis,
}) => {
  if (!analysis) {
    return (
      <TestUI.EmptyState message="Run the test to generate the reference tone, record the microphone signal, and compare both waveforms." />
    );
  }

  // The screen and the plot card each pad both sides.
  const plotWidth = layout.screenWidth - 4 * layout.screenPadding;

  return (
    <>
      <TestUI.SectionTitle title="How the comparison works" />
      <Card>
        {analysis.methodology.map((step, index) => (
          <Text key={step} style={typography.hint}>
            {index + 1}. {step}
          </Text>
        ))}
      </Card>

      <TestUI.SectionTitle title="Reported latencies" />
      <Card>
        <InfoRow label="baseLatency" value={formatMs(analysis.snapshot.base)} />
        <InfoRow
          label="outputLatency"
          value={formatMs(analysis.snapshot.output)}
        />
        <InfoRow
          label="inputLatency"
          value={formatMs(analysis.snapshot.input)}
        />
        <InfoRow
          label="Round-trip estimate"
          value={formatMs(analysis.reportedRoundTripLatency)}
        />
        <InfoRow
          label="Scheduled playback offset"
          value={formatMs(analysis.scheduleOffsetSeconds)}
        />
        <InfoRow
          label="Expected detection time"
          value={formatMs(analysis.expectedLatencySeconds)}
        />
        <InfoRow
          label="Measured detection time"
          value={
            analysis.measuredLatencySeconds !== null
              ? formatMs(analysis.measuredLatencySeconds)
              : 'Not found'
          }
        />
        <InfoRow
          label="Correlation score"
          value={
            analysis.correlation !== null
              ? analysis.correlation.toFixed(3)
              : '—'
          }
        />
        <InfoRow
          label="Noise floor (pre-beep)"
          value={
            analysis.noiseFloor !== null ? analysis.noiseFloor.toFixed(4) : '—'
          }
        />
        <InfoRow
          label="Delta"
          value={
            analysis.deltaMs !== null
              ? `${analysis.deltaMs.toFixed(2)} ms`
              : '—'
          }
        />
      </Card>

      <WaveformPlot
        title="Generated reference pattern"
        caption="Amplitude envelope of the synthesized beep pattern. Gaps are true silence in the generated signal."
        width={plotWidth}
        series={[analysis.referencePlot]}
        xAxisLabel={`Duration ${formatMs(analysis.referenceDurationSeconds)}`}
      />

      <WaveformPlot
        title="Microphone recording window"
        caption="Denoised amplitude envelope zoomed around the expected arrival time."
        width={plotWidth}
        series={[analysis.recordedWindowPlot]}
        markers={
          analysis.bestAlignmentMarkerRatio !== null
            ? [
                {
                  ratio: analysis.bestAlignmentMarkerRatio,
                  label: 'Best alignment',
                  color: colors.blue,
                },
              ]
            : undefined
        }
        xAxisLabel={`Window duration ${formatMs(analysis.recordedDurationSeconds)}`}
      />

      {analysis.overlayPlots.length > 0 ? (
        <WaveformPlot
          title="Aligned overlay (correlation match)"
          caption="Reference envelope overlaid with the microphone signal shifted to the best correlation lag."
          width={plotWidth}
          series={analysis.overlayPlots}
          sharedScale
          xAxisLabel="Same time axis after correlation shift"
        />
      ) : null}

      <WaveformPlot
        title="System-latency overlay"
        caption="Reference envelope overlaid with the microphone signal shifted by the system-reported latency (schedule + base + output + input)."
        width={plotWidth}
        series={analysis.systemShiftOverlayPlots}
        sharedScale
        markers={
          analysis.systemShiftMarkerRatio !== null
            ? [
                {
                  ratio: analysis.systemShiftMarkerRatio,
                  label: 'Expected arrival',
                  color: colors.yellow,
                },
              ]
            : undefined
        }
        xAxisLabel="Same time axis after system-latency shift"
      />

      {analysis.correlationProfile.length > 0 ? (
        <CorrelationPlot
          title="Correlation search"
          caption="Normalized cross-correlation between downsampled signal envelopes. The peak marks the best lag; yellow dashed line is the latency-based expectation."
          width={plotWidth}
          points={analysis.correlationProfile}
          bestLagMs={
            analysis.measuredLatencySeconds !== null
              ? analysis.measuredLatencySeconds * 1000
              : null
          }
          expectedLagMs={analysis.expectedLatencySeconds * 1000}
        />
      ) : null}

      <TestUI.ScenarioResults
        title="Test verdict"
        scenarios={[analysis.scenario]}
        emptyMessage=""
      />
    </>
  );
};

const styles = StyleSheet.create({
  infoLabel: {
    color: colors.textSecondary,
    flex: 1,
    fontSize: 13,
  },
  infoRow: {
    flexDirection: 'row',
    gap: 12,
    justifyContent: 'space-between',
  },
  infoValue: {
    color: colors.textPrimary,
    flex: 1,
    fontSize: 13,
    fontVariant: ['tabular-nums'],
    textAlign: 'right',
  },
});

export default LoopbackAnalysisPanel;
