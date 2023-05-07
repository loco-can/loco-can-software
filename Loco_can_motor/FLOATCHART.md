# Motor floatchart

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
        globalvars>int target_speed\nint currend_speed\nint target_break\nint current_break\nbool target_dir\nbool current_dir\nint ramp_time\nbool drive_nulled\nbool emergency] --- globaltimeout;
        globaltimeout>INTELLITIMEOUT heartbeat_timeout\nINTELLITIMEOUT standing_timeout\nINTELLITIMEOUT voltage_timeout] --- globalpwm;
        globalpwm>PWM_RAMP pwm_drive\nPWM_RAMP pwm_break] --- globalmeasure;
        globalmeasure>MEASURE voltage\nMEASURE batt_volt_1\nMEASURE batt__volt_2] --- globalflags;
        globalflags>FLAGS switches\nFLAGS status];
    end

    %% node definitions
    POWERUP((power up));
    SETRAMP>pwm_drive.ramp = 1000];
    SETSPEED>target_speed = 127];
    SETBREAK>target_break = 0];
    SETDIR>target_dir = 1];
    SETNULLED>drive_nulled = false];
    ENDPOWERUP([main loop]);

    %% flow
    POWERUP --> SETRAMP --> SETSPEED --> SETBREAK --> SETDIR --> SETNULLED --> ENDPOWERUP;
```

## set status

```mermaid
graph TD;

%% definitions
LOOP([set status]);
STAT_OFF>status = off];
STAT_READY>status = ready];
STAT_STANDBY>status = standby];
STAT_DRIVING>status = driving];
STAT_BREAKING>status = breaking];
STAT_EMERGENCY>status = emergency];
STAT_SETUP>status = setup];
IS_HEARTBEAT{heartbeat.timeout};
IS_SETUP{CAN.ID = setup message};
IS_MAINS{CAN.mains == true};
MAINS_ON>mains = true];
MAINS_OFF>mains = false];
STANDING{"STANDING?\nmotor.voltage < MIN &&\ntarget_speed == 0"};
IS_DRIVE{CAN.drive = true};
IS_BREAK{CAN.break != 0};
ENDLOOP([return]);

%% flow
LOOP --> IS_HEARTBEAT;
IS_HEARTBEAT --> |Y| STAT_EMERGENCY --> MAINS_OFF;
IS_HEARTBEAT --> |N| IS_SETUP;
IS_SETUP --> |N| IS_MAINS;
IS_SETUP --> |Y| STAT_SETUP --> ENDLOOP;
IS_MAINS --> |N| STAT_OFF --> MAINS_OFF --> ENDLOOP;
IS_MAINS --> |Y| MAINS_ON --> IS_DRIVE;
IS_DRIVE --> |N| STAT_STANDBY --> ENDLOOP;
IS_DRIVE --> |Y| STANDING;
STANDING --> |N| IS_BREAK;
STANDING --> |Y| STAT_READY --> ENDLOOP;
IS_BREAK --> |N| STAT_DRIVING --> ENDLOOP;
IS_BREAK --> |Y| STAT_BREAKING --> ENDLOOP;
```


## update
```mermaid
graph TD;

%% definitions
START([update]);
IS_EMERGENCY{emergency};
IS_OFF{off};
IS_STANDBY{standby};
IS_READY{ready};
IS_DRIVING{driving};
IS_BREAKING{breaking};
IS_SETUP{setup};

ESTOP>"EMERGENCY STOP\nspeed = 0/nbreak-ramp = quick\nbreak = max"];
SETUP[[setup]];
STOP>"STOP\nspeed = 0/nbreak-ramp = normal\nbreak = 0"];
SEND[[send CAN data]];
DRIVING>"speed = CAN.speed\nbreak = 0\nbreak-ramp = normal"];
BREAKING>"break = CAN.break\nspeed = 0\nbreak-ramp = normal"];
HEARTBEAT[[send vehicle heartbeat]];

END[[update PWM]];
RETURN([return]);

%% flow
START --> IS_EMERGENCY;
IS_EMERGENCY --> |N| IS_SETUP;
IS_EMERGENCY --> |Y| ESTOP --> HEARTBEAT ---> END;
IS_SETUP --> |N| IS_OFF;
IS_SETUP --> |Y| SETUP --> ESTOP;
IS_OFF --> |N| IS_STANDBY;
IS_OFF --> |Y| ESTOP;
IS_STANDBY --> |N| IS_READY;
IS_STANDBY --> |Y| STOP --> SEND;
IS_READY --> |N| IS_BREAKING;
IS_READY --> |Y| STOP;
IS_BREAKING --> |N| IS_DRIVING;
IS_BREAKING --> |Y| BREAKING --> SEND;
IS_DRIVING --> |N| SEND;
IS_DRIVING --> |Y| DRIVING --> SEND;
SEND --> END;
END --> RETURN;
```

## Main
### Main loop
graph TD;
    %% motor main loop

    %% node definitions
    LOOPSTART[/main loop\];
    ERROR_OFF>status.ERROR_FLAG = false];
    GETCAN{{get CAN message}};
    PARSE_MESSAGE[[parse drive message]];
    START_SETUP[[start loco setup]];

    subgraph standby
        SETUP{"loco setup message &&\nstanding = true &&\nmains = off"};
        DRIVE_NULLED{target_speed = 0?};
        SET_NULLED>drive_nulled = true];
    end

    subgraph heartbeat failure
        CHECKHEARTBEAT{check heartbeat};
        EMERGENCY_STOP[[emergency stop]];
        DRIVE_NULLED_FALSE>dirve_nulled = false];
    end

    subgraph drive
        CHECK_MAINS{message.mains = on};
        SET[[set parameters]];
        STOP[[stop loco]];
    end

    UPDATE[[update motor]];
    SEND{{send parameters:\ncurrent_speed\ncurrent_dir}};
    ENDLOOP[\end main loop/];

    %% flow main loop
    LOOPSTART --> GETCAN --> PARSE_MESSAGE --> SETUP;

    SETUP --> |N| START_SETUP --> UPDATE;
    SETUP --> |Y| DRIVE_NULLED;

    DRIVE_NULLED --> |N| CHECKHEARTBEAT;
    DRIVE_NULLED --> |Y| SET_NULLED --> CHECKHEARTBEAT;

    CHECKHEARTBEAT --> |OK| CHECK_MAINS;
    CHECKHEARTBEAT --> |TIMEOUT| DRIVE_NULLED_FALSE --> EMERGENCY_STOP --> UPDATE --> SEND --> ENDLOOP;

    CHECK_MAINS --> |Y| SET --> UPDATE;
    CHECK_MAINS --> |N| STOP --> UPDATE;


    %% node definitions
    SETPARAMS([set parameters]);
    SET_SPEED>target_speed = message.target_speed];
    SET_BREAK>target_break = message.target_break];
    SET_DIR>target_dir = message.target_dir];
    ENDSET([return]);

    %% flow set parameters
    SETPARAMS --> SET_SPEED --> SET_BREAK --> SET_DIR --> ENDSET;

    STARTSTOP([stop loco]);
    SETSTOP>target_speed = 0];
    ENDSTOP([return]);

    EMERGENCYSTOP([stop loco]);
    SETEMERGSTOP>target_speed = 0\nbreak_speed = max\ntarget_break = max];
    ENDEMERGSTOP([return]);

    EMERGENCYSTOP --> SETEMERGSTOP --> ENDEMERGSTOP;

    UPDATESTART([update motor]);
    IS_STANDING{loco is standing?};
    CHANGE_DIR{target_dir != current_dir};
    SETDIR>current_dir = target_dir];
    WRITEDIR[/direction = current_dir/];
    UPDATEEND([return]);

    UPDATESTART --> IS_STANDING;
    IS_STANDING --> |N| UPDATEEND;
    IS_STANDING --> |Y| CHANGE_DIR;
    CHANGE_DIR --> |N| UPDATEEND;
    CHANGE_DIR --> |Y| SETDIR --> WRITEDIR --> UPDATEEND;
    
    PARSE([parse drive message]);
    PARSE_MAINS{message.mains = 1};
    ENDPARSE([return]);

    PARSE --> PARSE_MAINS;
    PARSE_MAINS --> |N| ENDPARSE;
    PARSE_MAINS --> |Y| ENDPARSE;
```

### Heartbeat
```mermaid
graph TD;
    %% check heartbeat

    %% node definitions
    CHECKSTART([check heartbeat]);
    TIMEOUT{heartbeat_timeout = true}
    LOST[[heartbeat lost]];
    RETURN([return]);

    %% flow
    CHECKSTART --> TIMEOUT;
    TIMEOUT --> |N| RETURN;
    TIMEOUT --> |Y| LOST --> RETURN;
```
