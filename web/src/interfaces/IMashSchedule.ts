import type { IMashStep } from "./IMashStep";
import type { INotification } from "./INotification";

export interface IMashSchedule {
  name: string;
  boil: boolean;
  temporary: boolean;
  steps: Array<IMashStep>;
  notifications: Array<INotification>;
}
