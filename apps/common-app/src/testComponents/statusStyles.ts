import { colors } from '../styles';
import type { ScenarioStatus, TestStatus } from './types';

export const statusTextColors: Record<TestStatus, string> = {
  pass: colors.green,
  fail: colors.danger,
  info: colors.blue,
  skipped: colors.textMuted,
};

export const statusPillBackgroundColors: Record<ScenarioStatus, string> = {
  pass: colors.green,
  fail: colors.danger,
  skipped: colors.textMuted,
};
