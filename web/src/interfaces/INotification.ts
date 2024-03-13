export interface INotification {
  name: string;
  message: string;
  timeFromStart: number;
  timePoint: number;
  buzzer: boolean;
}

export const defaultNotification:INotification = {
  name: 'New Notification',
  message: '',
  timeFromStart: 0,
  timePoint: 0,
  buzzer: true,
};
