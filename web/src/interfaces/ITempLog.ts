import type { IDataPacket } from "./IDataPacket";

export interface ITempLog {
  sensor: string; // js doesn't support uint64_t, so we convert to string
  color: string;
  temps: Array<IDataPacket>;
}
