# Loco-CAN Programmer

The programmer is a module that is used to analyse the CAN communication and edit settings of the different modules. It is based on a ESP32 with a color OLED Display and a rotary encoder with a pushbutton for controlling the functions. The connection to the bus is compatible with all Loco-CAN modules.

In addition a webserver hosted on the ESP32, using a WIFI accesspoint, gives an more powerfull access to all internal functions. The webservice is responsive and can be used on smartphones, tablets and computers. All functions can be used locally and via the webservice.

The programmer has two modes, to analyse the CAN bus and to setup modules.

## CAN analysis mode

The programmer starts in the analysis mode. It scans the bus and shows the messages that are received. Messages with the same ID and UUID are summoned and only shown once. The type of the received message (its message ID) is translated to plain text. The transported values are formatted with a plain text label-value pair depending on the message type. Integer values, like speed or throttle, are shown as integer numbers, boolen values in a flag byte as switches.

## Module setup

In the setup mode all parameters of the connected modules can be set. The modules are pinged to collect informations about the type and UUID of the modules. Formatting and plain text labels are used for simple identification like in analysis mode.

A list shows the connected modules. When selecting a module, the available parameters are shown in the way of the analysis mode, can be modified and written back.
