import React, { FC, PropsWithChildren } from 'react';
import { ScrollView, StyleSheet, Text, View } from 'react-native';

import { Button, Card, Container } from '../components';
import { colors, typography } from '../styles';
import type { ControlAction, SummaryItem } from './types';

interface TestLayoutProps {
  /** What the screen checks; the native header already shows its name. */
  description: string;
  actions: ControlAction[];
  summary?: SummaryItem[];
  currentStep?: string;
}

/**
 * Test screen frame: description, summary badges, current step and controls
 * stay pinned while the results (children) scroll beneath them.
 */
const TestLayout: FC<PropsWithChildren<TestLayoutProps>> = ({
  description,
  actions,
  summary,
  currentStep,
  children,
}) => (
  <Container style={styles.screen}>
    <Text style={typography.hint}>{description}</Text>
    {summary ? (
      <View style={styles.row}>
        {summary.map((item) => (
          <Card key={item.label} style={styles.badge}>
            <Text style={typography.caption}>{item.label}</Text>
            <Text style={styles.badgeValue}>{item.value}</Text>
          </Card>
        ))}
      </View>
    ) : null}
    {currentStep ? (
      <Card>
        <Text style={typography.caption}>Current step</Text>
        <Text style={styles.currentStep}>{currentStep}</Text>
      </Card>
    ) : null}
    <View style={styles.row}>
      {actions.map((action) => (
        <Button
          key={action.title}
          title={action.title}
          onPress={action.onPress}
          disabled={action.disabled}
          minWidth={action.minWidth}
        />
      ))}
    </View>
    <ScrollView contentContainerStyle={styles.results}>{children}</ScrollView>
  </Container>
);

export default TestLayout;

const styles = StyleSheet.create({
  screen: {
    gap: 12,
  },
  row: {
    flexDirection: 'row',
    gap: 8,
  },
  badge: {
    flex: 1,
    gap: 2,
    padding: 12,
    borderRadius: 16,
  },
  badgeValue: {
    color: colors.textPrimary,
    fontSize: 15,
    fontWeight: '700',
  },
  currentStep: {
    color: colors.textPrimary,
    fontSize: 14,
    lineHeight: 20,
  },
  results: {
    gap: 12,
    paddingBottom: 32,
  },
});
