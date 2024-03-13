import { IImportedBeer } from '@/interfaces/IImportedBeer';
import { IMashStep } from '@/interfaces/IMashStep';
import { INotification } from '@/interfaces/INotification';

export default class ImportedBeer implements IImportedBeer {
  public name: string | null = null;

  public mashSteps: Array<IMashStep> = [];

  public mashNotifications: Array<INotification> = [];

  public boilSteps: Array<IMashStep> = [];

  public boilNotifications: Array<INotification> = [];
}
