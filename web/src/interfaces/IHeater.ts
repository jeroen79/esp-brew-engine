export interface IHeater {
  id: number;
  name: string;
  preference: number;
  pinNr: number;
  watt: number;
  useForMash: boolean;
  useForBoil: boolean;
}
