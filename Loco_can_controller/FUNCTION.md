# Controller function
```mermaid
graph TD;
  Start --> Init[set inactive];
  Init --> Instr[instrument light on];
  Instr --> LED[["run LED init sequence\nred > orange > green"]];
  LED --> METER[["run Meter init sequence\nMax > Min"]];
  METER --> dark[instrument light off];
  DARK --> INITEND[start main loop];

```
