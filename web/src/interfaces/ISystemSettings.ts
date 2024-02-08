import TemperatureScale from '@/enums/TemperatureScale';

export interface ISystemSettings {
  onewirePin: number;
  heat1Pin: number;
  heat2Pin: number;
  stirPin: number;
  invertOutputs: boolean;
  mqttUri: string;
  temperatureScale: TemperatureScale;
}
