export interface IMashStep {
  index: number;
  name: string;
  temperature: number;
  stepTime: number;
  time: number;
  extendStepTimeIfNeeded: boolean;
}
