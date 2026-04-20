import { colors } from '../styles';
import type { ScenarioStatus, TestStatus } from './types';

export const statusTextColors: Record<TestStatus, string> = {
  pass: '#7bd88f',
  fail: '#ff8d8d',
  info: '#8fc8ff',
  skipped: colors.gray,
};

export const statusPillBackgroundColors: Record<ScenarioStatus, string> = {
  pass: '#28794a',
  fail: '#a63f3f',
  skipped: '#5b5f6c',
};
