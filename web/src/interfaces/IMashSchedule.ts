import { IMashStep } from './IMashStep';

export interface IMashSchedule {
  name: string,
  steps: Array<IMashStep>,
}
