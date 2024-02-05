export interface ITempSensor {
  id: number;
  color: string;
  name: string;
  enabled: boolean;
  connected: boolean;
  offset: number;
  lastTemp: number;
}
