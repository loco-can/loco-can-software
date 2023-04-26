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
