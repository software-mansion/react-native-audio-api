import React, { FC } from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { TestUI } from '../../testComponents';
import { colors, layout } from '../../styles';
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

const LoopbackAnalysisPanel: FC<LoopbackAnalysisPanelProps> = ({ analysis }) => {
  if (!analysis) {
    return (
      <TestUI.EmptyState message="Run the test to generate the reference tone, record the microphone signal, and compare both waveforms." />
    );
  }

  const plotWidth = layout.screenWidth - 36;
  const { scenario } = analysis;

  return (
    <View style={styles.container}>
      <TestUI.SectionTitle title="How the comparison works" />
      <View style={styles.methodologyCard}>
        {analysis.methodology.map((step, index) => (
          <Text key={step} style={styles.methodologyStep}>
            {index + 1}. {step}
          </Text>
        ))}
      </View>

      <TestUI.SectionTitle title="Reported latencies" />
      <View style={styles.metricsCard}>
        <InfoRow
          label="baseLatency"
          value={formatMs(analysis.snapshot.base)}
        />
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
            analysis.noiseFloor !== null
              ? analysis.noiseFloor.toFixed(4)
              : '—'
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
      </View>

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
                  color: colors.main,
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

      <TestUI.SectionTitle title="Test verdict" />
      <TestUI.ScenarioResults
        scenarios={[
          {
            id: scenario.id,
            title: scenario.title,
            status: scenario.status,
            durationLabel: `${(scenario.durationMs / 1000).toFixed(2)}s`,
            steps: scenario.steps.map((step) => ({
              id: `${scenario.id}-${step.id}`,
              message: step.message,
              status: step.status,
              details: step.details,
            })),
          },
        ]}
        emptyMessage=""
      />
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    gap: 14,
  },
  infoLabel: {
    color: colors.gray,
    flex: 1,
    fontSize: 13,
  },
  infoRow: {
    flexDirection: 'row',
    gap: 12,
    justifyContent: 'space-between',
  },
  infoValue: {
    color: colors.white,
    flex: 1,
    fontSize: 13,
    fontVariant: ['tabular-nums'],
    textAlign: 'right',
  },
  methodologyCard: {
    backgroundColor: colors.backgroundLight,
    borderRadius: 8,
    gap: 8,
    padding: 12,
  },
  methodologyStep: {
    color: colors.gray,
    fontSize: 13,
    lineHeight: 18,
  },
  metricsCard: {
    backgroundColor: colors.backgroundLight,
    borderRadius: 8,
    gap: 10,
    padding: 12,
  },
});

export default LoopbackAnalysisPanel;
