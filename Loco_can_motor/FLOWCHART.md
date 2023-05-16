# Motor flowchart

## Function
The motor module can be connected to a controller or another motor module. In the first case a controller heardbeat must be present, else an emergency break is triggert. The module can also be operated in slave mode getting all commands from another motor module.

## Legend
```mermaid
graph TD;
    %% legend
    BOOT((power up));
    METHOD([method start/end])
    ROUTINE[[subroutine]]
    VAL>set value]
    HARDWARE_ACCESS[/read/write hardware/];
    CAN{{send/receive\nmessage}}
```

## Init
```mermaid
graph TD;
    %% motor init

    subgraph global;
        globaltimeout>INTELLITIMEOUT heartbeat_timeout\nINTELLITIMEOUT standing_timeout\nINTELLITIMEOUT voltage_timeout] --- globalpwm;
        globalpwm>PWM_RAMP pwm_drive\nPWM_RAMP pwm_break] --- globalmeasure;
        globalmeasure>MEASURE voltage\nMEASURE batt_volt_1\nMEASURE batt__volt_2] --- globalflags;
        globalflags>FLAGS switches\nFLAGS status];

    end

    %% node definitions
    POWERUP((power up));
    SETDRIVERAMP>LOAD FROM EEPROM\npwm_drive.ramp = 1000];
    SETBREAKRAMP>LOAD FROM EEPROM\npwm_break.ramp = 300];
    SETSPEED>target_speed = 0];
    SETBREAK>target_break = 0];
    SETDISABLED>LOAD FROM EEPROM\ndisabled = false];
    SETSLAVE>slave = false];
    SETDIR>target_dir = 0];
    SETREVERSE>LOAD FROM EEPROM\nreverse = false];
    ENDPOWERUP[[main loop]];

    %% flow
    POWERUP --> SETDRIVERAMP --> SETBREAKRAMP --> SETSPEED --> SETBREAK --> SETDIR --> SETDISABLED --> SETSLAVE --> SETREVERSE --> ENDPOWERUP;
```

## Main loop
```mermaid
graph TD;
    %% motor main loop

    %% node definitions
    LOOP[/loop\];
    READ_CAN[[read CAN message]];
    SET_STATUS[[set status]];
    UPDATE_MOTOR[[update motor]];
    END[\end loop/];

    %% flow
    LOOP --> READ_CAN --> SET_STATUS --> UPDATE_MOTOR --> END;
```

## Status parser

The motor module can be in different stati, that depend on the locomotive movement and the CAN drive message.

The standing status (if the vehicle is not moving) is stored in a bool variable. The status is independent of the motor status.
The drive status (main power relais) is stored in a bool variable.

* emenrgency: controller heartbeat has timed out

All following stati depend on a valid controller heartbeat:

* off: mains is off
* standby: mains = on but no direction selected
* idle: mains = on, direction selected and moving without drive or break
* driving: mains = on, direction selected, target_speed set
* breaking: mains = on, direction selected, target_break set

* setup: the controller is in loco setup mode

If the controller sends a multi traktion uuid (bit 7 of byte 2 = true), and the uuid is not the own,
light or pantograph functions are disabled.


```mermaid
graph TD;
%% motor status parser

%% global variables
subgraph global
    globalmains>bool mains = false] --- globalpower;
    globalpower>bool drive = false] --- globalstatus;
    globalstatus>int status = emergency] --- globalstanding;
    globalstanding>bool standing = false];
end

%% definitions
LOOP([set status]);

subgraph standing
    IS_STANDING{"STANDING?\nmotor.voltage < MIN &&\ntarget_speed == 0"};
    SET_STANDING>standing = true];
    SET_MOVING>standing = false];
    DRIVE_OFF_TIMEOUT{drive off timed out};
    DRIVE_OFF_RETRIGGER>retrigger drive off timeout];
    DRIVE_OFF>drive = false];
end

IS_HEARTBEAT{CAN.heartbeat timeout};

subgraph inactive
    STAT_EMERGENCY>status = emergency];
    STAT_OFF>status = off];
    MAINS_OFF>mains = false];
end
subgraph active
    IS_SETUP{CAN.ID = setup message &&\nCAN.setup.uuid == uuid};
    IS_MAINS{CAN.mains == true};
    IS_DRIVE{"DIR SELECTED\nCAN.drive = true"};
    DRIVE_ON>drive = true];
    IS_BREAKING{CAN.break != 0};
    IS_DRIVING{CAN.drive != 0};
    STAT_STANDBY>status = standby];
    STAT_READY>status = idle];
    STAT_DRIVING>status = driving];
    STAT_BREAKING>status = breaking];
    STAT_SETUP>status = setup];

    subgraph "slave mode => move to CAN receive"
        MAINS_ON>mains = true];
        IS_MULTI{"CAN.status.multi == false ||\n(CAN.status.multi == true &&\nCAN.multi.uuid == UUID)"};
        SET_SLAVE>slave = true];
        CLEAR_SLAVE>slave = false];
    end
end

ENDLOOP([return]);

%% flow
LOOP --> IS_STANDING;
IS_STANDING ==> |N| DRIVE_OFF_RETRIGGER --> SET_MOVING --> IS_HEARTBEAT;
IS_STANDING --> |Y| SET_STANDING --> DRIVE_OFF_TIMEOUT;
DRIVE_OFF_TIMEOUT --> |N| IS_HEARTBEAT;
DRIVE_OFF_TIMEOUT --> |Y| DRIVE_OFF --> IS_HEARTBEAT;
IS_HEARTBEAT --> |Y| STAT_EMERGENCY --> MAINS_OFF;
IS_HEARTBEAT ==> |N| IS_SETUP;
IS_SETUP ==> |N| IS_MAINS;
IS_SETUP --> |Y| STAT_SETUP -->ENDLOOP;
IS_MAINS --> |N| STAT_OFF --> MAINS_OFF --> ENDLOOP;
IS_MAINS ==> |Y| MAINS_ON ==> IS_MULTI;
IS_MULTI --> |N| CLEAR_SLAVE --> IS_DRIVE;
IS_MULTI ==> |N| SET_SLAVE ==> IS_DRIVE;
IS_DRIVE --> |N| STAT_STANDBY;
IS_DRIVE ==> |Y| DRIVE_ON;
STAT_STANDBY --> ENDLOOP;
DRIVE_ON --> IS_BREAKING;
IS_BREAKING ==> |N| IS_DRIVING;
IS_BREAKING ==> |Y| STAT_BREAKING --> ENDLOOP;
IS_DRIVING --> |N| STAT_READY --> ENDLOOP;
IS_DRIVING ==> |Y| STAT_DRIVING ==> ENDLOOP;
```


## update motor
```mermaid
graph TD;
%% motor update

%% definitions
START([update motor]);
IS_EMERGENCY{"status == emergency || \nstatus == off"};

subgraph inactive
    BREAK_RAMP_MAX>break_ramp = max];
    HEARTBEAT[[send vehicle heartbeat]];
    ESTOP>"EMERGENCY STOP\ntarget_speed = 0\ntarget_break = max"];
end

subgraph active

    IS_STANDBY{"status = standby ||\nstatus = ready"};
    BREAK_RAMP_NORMAL>break_ramp = normal];

    subgraph stop
        STOP>"STOP\nspeed = 0\nbreak = 0"];
    end

    subgraph drive
        IS_STANDING_DIR{standing == true};
        IS_DRIVING{status == driving};
        IS_BREAKING{status == breaking};
        IS_IDLE{status == idle};
        IDLE>"speed = 0\nbreak = 0"];
        DRIVING>"speed = CAN.speed\nbreak = 0"];
        BREAKING>"break = CAN.break\nspeed = 0"];
        IS_REVERSE{reverse == true};
        SET_DIR>target_dir = CAN.dir];
        SET_REV_DIR>target_tir = !CAN.dir];
    end

    SEND[[send CAN data]];
end

subgraph setup
    SETUP_STOP>"speed = 0\nbreak = 0"];
    SETUP[[setup]];
    SEND_SETUP[[send setup CAN data]];
end

IS_SETUP{status == setup};

END[[update PWM]];
RETURN([return]);

%% flow
START --> IS_SETUP;
IS_SETUP --> |N| IS_EMERGENCY;
IS_SETUP --> |Y| SETUP_STOP --> SETUP --> SEND_SETUP --> END;
IS_EMERGENCY --> |N| BREAK_RAMP_NORMAL --> IS_STANDBY;
IS_EMERGENCY --> |Y| BREAK_RAMP_MAX --> ESTOP --> HEARTBEAT --> END;
IS_STANDBY --> |N| IS_STANDING_DIR;
IS_STANDBY --> |Y| STOP;
IS_IDLE --> |N| IS_BREAKING;
IS_IDLE --> |Y| IDLE --> SEND;
STOP --> SEND;

IS_STANDING_DIR --> |N| IS_IDLE;
IS_STANDING_DIR --> |Y| IS_REVERSE;
IS_REVERSE --> |N| SET_DIR --> SEND;
IS_REVERSE --> |Y| SET_REV_DIR --> SEND;
IS_BREAKING --> |N| IS_DRIVING;
IS_BREAKING --> |Y| BREAKING --> SEND;
IS_DRIVING --> |N| SEND;
IS_DRIVING --> |Y| DRIVING --> SEND;
SEND --> END;
END --> RETURN;
```

## Update PWM
```mermaid
graph TD

    START([update PWM]);
    END([return]);

    subgraph power switch
        IS_DRIVE_ON{drive == true};
        MAIN_ON[/main relais on/];
        MAIN_IS_STANDING{standing == true};
        MAIN_OFF[/main relais off/];
    end

    %% definitions
    subgraph breaking
        IS_BREAKING{"IS BREAKING\n_target_break != 0"};
        STOP_DRIVE[/"STOP DRIVE PWM\n_pwm_drive.writePWM(0)"/];
        IS_DRIVE_STOPPED{"DRIVE STOPPED?\n_drive_pwm.stopped() == true"};
        SET_BREAK>"_pwm_break.pwmWrite(target_break)"];
    end

    subgraph driving
        STOP_BREAK[/"STOP BREAK PWM\n_pwm_break.writePWM(0)"/];
        IS_DRIVING{"IS DRIVING\nstatus == driving"};
        IS_BREAK_STOPPED{"BREAK STOPPED?\n_break_pwm.stopped() == true"};
        SET_DRIVE>"_pwm_drive.pwmWrite(target_speed)"];
    end

    UPDATE[/"drivePWM.uptate()\nbreakPWM.update()"/];


    %% flow
    START --> IS_DRIVE_ON;
    IS_DRIVE_ON --> |N| MAIN_IS_STANDING;
    MAIN_IS_STANDING --> |N| IS_BREAKING;
    MAIN_IS_STANDING --> |Y| MAIN_OFF --> IS_BREAKING;
    IS_DRIVE_ON --> |Y| MAIN_ON --> IS_BREAKING;

    IS_BREAKING --> |N| IS_DRIVING;
    IS_BREAKING --> |Y| STOP_DRIVE --> IS_DRIVE_STOPPED;
    IS_DRIVE_STOPPED --> |N| UPDATE;
    IS_DRIVE_STOPPED --> |Y| SET_BREAK --> UPDATE;
    IS_DRIVING --> |N| UPDATE;
    IS_DRIVING --> |Y| STOP_BREAK --> IS_BREAK_STOPPED;
    IS_BREAK_STOPPED --> |N| UPDATE;
    IS_BREAK_STOPPED --> |Y| SET_DRIVE --> UPDATE;
    UPDATE --> END;

```

# Setup
```mermaid
graph TD

    %% motor setup

    %% definitions
    SETUP([setup]);
    IS_SELECT{CONTROLLER DIR IN MIDDLE\nCAN.setup.status == SELECT};

    subgraph reverse
        SET_REV>CONTROLLER DIR ON REVERSE\nreverse = CAN.setup.reverse];
        BLINK_FORWARD[["reverse = false: blink forward\nelse blink backside\nblink(reverse)"]];
    end

    subgraph select
        BLINK_ALL[[blink all]];
        IS_DISABLE{CAN.setup.disable = true};
        DISABLE>TOGGLE DISABLED STATUS\ndisable = !disable];
    end

    END([return]);

    %% flow
    SETUP --> IS_SELECT;
    IS_SELECT --> |N| SET_REV --> BLINK_FORWARD --> END;
    IS_SELECT --> |Y| IS_DISABLE;
    IS_DISABLE --> |N| BLINK_ALL --> END;
    IS_DISABLE --> |Y| DISABLE --> END;
```
