import TemperatureScale from '@/enums/TemperatureScale';

export interface ISystemSettings {
  onewirePin: number;
  stirPin: number;
  invertOutputs: boolean;
  mqttUri: string;
  temperatureScale: TemperatureScale;
}
