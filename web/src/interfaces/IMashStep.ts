export interface IMashStep {
  index: number;
  name: string;
  temperature: number;
  stepTime: number;
  time: number;
  extendStepTimeIfNeeded: boolean;
}

export const defaultMashStep:IMashStep = {
  index: 0,
  name: 'New Step',
  temperature: 0,
  stepTime: 0,
  time: 0,
  extendStepTimeIfNeeded: true,
};
