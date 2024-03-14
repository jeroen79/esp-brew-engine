import { IMashStep } from './IMashStep';
import { INotification } from './INotification';

export interface IImportedBeer {
  name: string | null;

  mashSteps: Array<IMashStep>;
  mashNotifications: Array<INotification>;
  mashNotificationsGrouped: Array<INotification>;

  boilSteps: Array<IMashStep>;
  boilNotifications: Array<INotification>;
  boilNotificationsGrouped: Array<INotification>;
}
