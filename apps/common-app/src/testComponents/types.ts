export type TestStatus = 'pass' | 'fail' | 'info' | 'skipped';
export type ScenarioStatus = 'pass' | 'fail' | 'skipped';

export interface SummaryItem {
  label: string;
  value: string;
}

export interface StepItem {
  id: string;
  message: string;
  status: TestStatus;
  details?: string;
}

export interface ScenarioItem {
  id: string;
  title: string;
  status: ScenarioStatus;
  durationLabel: string;
  steps: StepItem[];
}

export interface ControlAction {
  title: string;
  onPress: () => void;
  disabled?: boolean;
  width?: number;
}
