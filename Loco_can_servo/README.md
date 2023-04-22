## LOCO-Servo

The module read values of CAN messages and set the motors. The four servos can be configured to react to different messages.

The min and max positions as well as a mapping of the message values provides a wide range of usecases like drive or break setting or measure values.

There are different types of operation that can be selected from.

* An analog value of a analog message value can be mapped to a certain angle area, the motor will cover.
* Up to 16 areas of an analog value can be assigned to descrete servo positions.
* For binary messages the positions for false and true can be set.
* When using a status message, up to 8 bits can be assigned to discrete servo motor positions.