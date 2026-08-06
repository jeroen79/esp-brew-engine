import type TemperatureScale from "@/enums/TemperatureScale";

export interface ISystemSettings {
  onewirePin: number;
  displaySclPin: number;
  displaySdaPin: number;
  stirPin: number;
  buzzerPin: number;
  buzzerTime: number;
  invertOutputs: boolean;
  mqttUri: string;
  temperatureScale: TemperatureScale;
}
