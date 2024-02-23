import { IMashStep } from './IMashStep';

export interface IMashSchedule {
  name: string;
  boil: boolean;
  steps: Array<IMashStep>;
}
