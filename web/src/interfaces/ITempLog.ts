import { IDataPacket } from './IDataPacket';

export interface ITempLog {
  sensor: string,
  color: string,
  temps: Array<IDataPacket>,
}
