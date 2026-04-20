import React, { FC } from 'react';
import { StyleSheet, View } from 'react-native';

import EmptyState from './EmptyState';
import ScenarioCard from './ScenarioCard';
import SectionTitle from './SectionTitle';
import type { ScenarioItem } from './types';

interface ScenarioResultsProps {
  scenarios: ScenarioItem[];
  emptyMessage: string;
  title?: string;
}

const ScenarioResults: FC<ScenarioResultsProps> = ({
  scenarios,
  emptyMessage,
  title = 'Scenario Results',
}) => (
  <View style={styles.sectionContent}>
    <SectionTitle title={title} />
    {scenarios.length === 0 ? (
      <EmptyState message={emptyMessage} />
    ) : (
      scenarios.map((scenario) => (
        <ScenarioCard key={scenario.id} scenario={scenario} />
      ))
    )}
  </View>
);

const styles = StyleSheet.create({
  sectionContent: {
    gap: 12,
  },
});

export default ScenarioResults;
