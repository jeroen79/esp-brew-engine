import { IDataPacket } from './IDataPacket';

export interface ITempLog {
  sensor: number;
  color: string;
  temps: Array<IDataPacket>;
}
