import type { IImportedBeer } from "@/interfaces/IImportedBeer";
import type { IMashStep } from "@/interfaces/IMashStep";
import type { INotification } from "@/interfaces/INotification";

export default class ImportedBeer implements IImportedBeer {
  public name: string | null = null;

  public mashSteps: Array<IMashStep> = [];

  public mashNotifications: Array<INotification> = [];

  public mashNotificationsGrouped: Array<INotification> = [];

  public boilSteps: Array<IMashStep> = [];

  public boilNotifications: Array<INotification> = [];

  public boilNotificationsGrouped: Array<INotification> = [];
}
