export interface ITempSensor {
  id: string; // js doesn't support uint64_t, so we convert to string
  color: string;
  name: string;
  show: boolean;
  useForControl: boolean;
  connected: boolean;
  compensateAbsolute: number;
  compensateRelative: number;
  lastTemp: number;
}
