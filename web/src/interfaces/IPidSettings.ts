export interface IPidSettings {
  kP: number;
  kI: number;
  kD: number;
  boilkP: number;
  boilkI: number;
  boilkD: number;
  pidLoopTime: number;
  stepInterval: number;
  boostModeUntil: number;
}
