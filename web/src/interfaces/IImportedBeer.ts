import { IMashStep } from './IMashStep';
import { INotification } from './INotification';

export interface IImportedBeer {
  name: string | null;

  mashSteps: Array<IMashStep>;
  mashNotifications: Array<INotification>;

  boilSteps: Array<IMashStep>;
  boilNotifications: Array<INotification>;
}
