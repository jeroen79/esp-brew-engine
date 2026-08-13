import type TemperatureScale from "@/enums/TemperatureScale";

export interface ISystemSettings {
  onewirePin: number;
  displaySclPin: number;
  displaySdaPin: number;
  displayAddress: number;
  stirPin: number;
  stirButtonPin: number;
  scheduleButtonPin: number;
  nextScheduleButtonPin: number;
  buzzerPin: number;
  buzzerTime: number;
  invertOutputs: boolean;
  mqttUri: string;
  temperatureScale: TemperatureScale;
}
