# Motor floatchart

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
    ENDPOWERUP([main loop]);

    %% flow
    POWERUP --> SETRAMP --> SETSPEED --> SETBREAK --> SETDIR --> ENDPOWERUP;
```

## Main
### Main loop
```mermaid
graph TD;
    %% motor main loop

    %% node definitions
    LOOPSTART[/main loop\];
    ERROR_OFF>status.ERROR_FLAG = false];
    GETCAN{{get CAN message}};

    CHECKHEARTBEAT[[check heartbeat]];

    DRIVE_NULLED{target_speed = 0?};
    SET_NULLED>drive_nulled = true];
    ENDLOOP[\end main loop/];

    %% flow
    LOOPSTART --> ERROR_OFF --> GETCAN;
    GETCAN --> DRIVE_NULLED;
    DRIVE_NULLED --> |N| ENDLOOP;
    DRIVE_NULLED --> |Y| SET_NULLED --> ENDLOOP;

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
