export interface ITempSensor {
  id: string; // js doesn't support uint64_t, so we convert to string
  color: string;
  name: string;
  enabled: boolean;
  connected: boolean;
  offset: number;
  lastTemp: number;
}
