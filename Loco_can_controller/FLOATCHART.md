# Controller floatchart

## Function
In a train setup many controllers and many locomotives can be connected but only one controller can be active. If more than one locomotive is connected (multi traktion) the telemetry data are displayed from only one locomotive. The headlights are only activated from the selected loco, all others keep dark. The active locomotive can be selected in the loco setup mode.

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
    %% controller init

    subgraph global;
        globalvars>bool active\nint status\n] --- globalmeter;
        globalmeter>INTELLILED status_led\nMETER volt\nMETER ampere\nMETER speed] --- globalbutton;
        globalbutton>ANALOGSWITCH mains\nANALOGSWITCH dir\nANALOGSWITCH light];
    end

    %% power up
    %% node definitions
    START((power up));
    INIT_VAR>active = false\nstatus = off];
    INIT_LED[/"status_led.begin()"/];
    INSTR_ON[/instrument light on/];
    METER_MAX[["meter test(max)"]];
    LED_TEST[["status led test"]];
    METER_MIN[["meter test(min)"]];
    INSTR_OFF[/instrument light off/];
    LOAD_SETTINGS[[load settings]];
    INITEND([main loop]);

    %% flow
    START --> INIT_VAR --> INIT_LED --> INSTR_ON;
    INSTR_ON --> METER_MAX --> LED_TEST --> METER_MIN --> INSTR_OFF;
    INSTR_OFF --> LOAD_SETTINGS;
    LOAD_SETTINGS --> INITEND;

    %% led test
    %% node definitions
    LEDTEST([status led test]);
    LED_RED[/status_led RED/];
    LED_ORANGE[/status_led ORANGE/];
    LED_GREEN[/status_led GREEN/];
    LED_OFF[/status_led off/];
    WAIT_RED[wait 500ms];
    WAIT_ORANGE[wait 500ms];
    WAIT_GREEN[wait 500ms];

    %% flow
    LEDTEST --> LED_RED --> WAIT_RED --> LED_ORANGE --> WAIT_ORANGE --> LED_GREEN --> WAIT_GREEN --> LED_OFF;

    %% meter test
    %% node definitions
    METER_TEST(["meter test(val)"]);
    METER_TEST_VOLT[/"volt(val)"/];
    METER_TEST_AMP[/"ampere(val)"/]
    METER_TEST_SPEED[/"speed(val)"/];
    METER_TEST_END([return]);

    %% flow
    METER_TEST --> METER_TEST_VOLT --> METER_TEST_AMP --> METER_TEST_SPEED --> METER_TEST_END;

    %% load settings
    %% node definitions
    LOADSETTINGS([load settings]);
    ENDLOAD([return]);
    
    %% flow
    LOADSETTINGS --> ENDLOAD;
  ```

## Main
### Main loop
```mermaid
graph TD;
    %% controller main loop

    subgraph global
        globalvars>METER speed];
    end

    %% node definitions
    LOOPSTART[/main loop\];

    subgraph get data
        GET_DATA[[get CAN data]];
        GET_SWITCHES[[get switches]];
        GET_POTS[[get potentiometer]];
        GET_STATI[[set status]];
    end

    SWITCH_STATUS{action by status};
    ACTION_OFF[[action off]];
    ACTION_STANDBY[[action standby]];

    subgraph controller active
        ACTION_ON[[action on]];
        ACTION_READY[[action ready]];
        ACTION_MOVING[[action moving]];
        SET_METERS[/"volt.set(message.volt)\nampere.set(message.ampere)"/];
        SEND_DATA{{"send controller data:{\nstatus,\ndrive_val,\nbreak_val,\npower_val}"}};
    end

    SET_SPEEDO[/"speed.set(message.speed)"/];
    ENDLOOP[\end main loop/];

    %% flow
    LOOPSTART --> GET_DATA --> GET_SWITCHES --> GET_POTS --> GET_STATI --> SWITCH_STATUS;
    SWITCH_STATUS --> |off| ACTION_OFF;
    SWITCH_STATUS --> |standby| ACTION_STANDBY;
    SWITCH_STATUS --> |on| ACTION_ON;
    SWITCH_STATUS --> |ready| ACTION_READY;
    SWITCH_STATUS --> |moving| ACTION_MOVING;

    ACTION_OFF --> SET_SPEEDO;
    ACTION_STANDBY --> SET_SPEEDO;
    ACTION_ON --> SET_METERS;
    ACTION_READY --> SET_METERS;
    ACTION_MOVING --> SET_METERS;

    SET_METERS --> SEND_DATA --> SET_SPEEDO --> ENDLOOP;
```

### CAN communication
```mermaid
graph TD;
    %% CAN communication

    %% get CAN message
    %% node definitions
    GET_CAN([get CAN data]);
    MESSAGE_AVAILABLE{message available};
    SWITCH_MESSAGE{select by type};
    GET_MOTOR>"register in vehicle list"];
    GET_VOLTAGE>"voltage = message.data"];
    GET_CURRENT>"current = message.data"];
    GET_SPEED>"speed = message.data"];
    PARSE_MESSAGES[[parse messages]];
    END_CAN([return]);

    %% flow
    GET_CAN --> MESSAGE_AVAILABLE;
    MESSAGE_AVAILABLE --> |N| PARSE_MESSAGES;
    MESSAGE_AVAILABLE --> |Y| SWITCH_MESSAGE;
    SWITCH_MESSAGE --> |vehicle| GET_MOTOR;
    SWITCH_MESSAGE --> |voltage| GET_VOLTAGE;
    SWITCH_MESSAGE --> |current| GET_CURRENT;
    SWITCH_MESSAGE --> |speed| GET_SPEED;
    GET_MOTOR --> PARSE_MESSAGES;
    GET_VOLTAGE --> PARSE_MESSAGES;
    GET_CURRENT --> PARSE_MESSAGES;
    GET_SPEED --> PARSE_MESSAGES;
    PARSE_MESSAGES --> END_CAN;

    %% flow

    %% send CAN message
    %% node definitions
    SENDDATA([send CAN data]);
    DATA_TIMEOUT{data timeout?};
    DATA{{"send data:\nstatus\nswitches\npotentiometer"}};
    HB_TIMEOUT{"controller\nheartbeat\ntimeout?"};
    SEND_HB{{send heartbeat}};
    ENDSENDDATA([return]);

    %% flow
    SENDDATA --> DATA_TIMEOUT;
    DATA_TIMEOUT --> |N| HB_TIMEOUT;
    DATA_TIMEOUT --> |Y| DATA;
    DATA --> HB_TIMEOUT;
    HB_TIMEOUT --> |Y| SEND_HB --> ENDSENDDATA;
    HB_TIMEOUT -->|N| ENDSENDDATA;
```

### Hardware access

```mermaid
graph TD;
    %% get hardware data

    %% node definitions
    GETSWITCHES([get switches]);
    FETCH_DIGITAL>"flags.switches.mains = mains.get()\nflags.switches.dir = dir.get()\nflags.switches.light = light.get()\nflags.switches.light_1 = light_1.get()\nflags.switches.horn = horn.get()"];
    ENDSWITCHES([return]);

    GETPOTS([get pots]);
    FETCH_ANALOG>"driveval = analog.drive\nanalogval = analog.break\npowerval = analog.power"];
    ENDPOTS([return]);

    %% flow
    GETSWITCHES --> FETCH_DIGITAL --> ENDSWITCHES;
    GETPOTS --> FETCH_ANALOG --> ENDPOTS;
```

### Status parser
```mermaid
graph TD;
    %% status parser
    
    %% node definitions
    GET_STATUS([set status]);
    CHECK_ACTIVE[["check for activatable"]];

    subgraph active
        IS_ACTIVE{can be activated?};
        IS_SETUP{status == setup};
        IS_MAINS{switches.mains = on?};
        SWITCHING_ON{is activated};
        COLLECT_VEHICLES>write current vehicle in list];
        STATUS_STANDBY>status = standby];
        CHECK_SETUP{"horn == true &&\nactive == false"};
        SET_SETUP>status = setup];
        SET_ACT_ON>active = true];
        IS_DRIVE{drive on?};
        STATUS_STANDBY>status = standby];
        CHECK_MOVING{is moving?};
        STATUS_READY>status = ready];
        STATUS_MOVING>status = moving];
    end

    subgraph inactive
        STATUS_OFF>status = off];
        SET_ACT_OFF>active = false\nclear vehicle list];
        STATUS_LOCKED>status = locked];
    end

    STATUS_OFF>status = off];
    LED_STATUS[[set status led]];
    END_STATUS([return]);

    %% flow
    GET_STATUS ==> CHECK_ACTIVE ==> IS_MAINS;
    IS_MAINS --> |N| STATUS_OFF -->  SET_ACT_OFF;
    IS_MAINS ==> |Y| IS_ACTIVE;
    IS_ACTIVE --> |N| STATUS_LOCKED --> SET_ACT_OFF;
    IS_ACTIVE ==> |Y| SWITCHING_ON;
    SWITCHING_ON --> |N| COLLECT_VEHICLES -->IS_SETUP;
    SWITCHING_ON ==> |Y| IS_SETUP;
    SET_ACT_OFF --> LED_STATUS
    IS_SETUP --> |N| CHECK_SETUP;
    IS_SETUP --> |Y| LED_STATUS;
    CHECK_SETUP ==> |N| SET_ACT_ON;
    CHECK_SETUP --> |Y| SET_SETUP;
    SET_SETUP --> SET_ACT_ON --> IS_DRIVE;
    IS_DRIVE --> |N| STATUS_STANDBY --> LED_STATUS;
    IS_DRIVE ==> |Y| CHECK_MOVING;
    CHECK_MOVING --> |N| STATUS_READY --> LED_STATUS;
    CHECK_MOVING ==> |Y| STATUS_MOVING --> LED_STATUS;
    LED_STATUS --> END_STATUS;
```

## Loco setup
Select locomotive in multi traktion and set direction of locomotive. To enter the loco setup mode the controller must be activateable, so no other controller can be switched on.

Enter setup mode:
1.  mains switch off
2.  push and hold the horn button
3.  mains switch on

The status led blinks orange fastly.

To exit the setup mode, the mains switch must be switched off. All settings are saved.

### Loco pairing
To select a loco as paired in a multi traktion setup, the direction switch must be in off position. The selected loco is blinking with all headlights. Short push the horn button to select the next loco.

### Loco direction setup
To synchronize the position of the direction switch on the controller and the actual driving direction of the loco, this setup is used. It can be done for more locos in a multi traktion setup by stepping from loco to loco using the horn button.

To select the forward direction of a loco, the direction switch has to be brought in the position, where the forward lights of the selected loco are blinking.

Short push the horn button to select the next loco.

```mermaid
graph TD;
    %% loco setup mode

    %% variables
    subgraph local
        localvars>bool new\nint index]
    end

    %% node definitions
    LOCOSETUP([loco setup]);
    LED_SETUP[/status_led SETUP/];
    SEL_FIRST>index = 0\nnew = true];
    subgraph wait for horn button release
        HORN_WAIT{horn button = true};
    end
    subgraph setup loop
        INIT_LOCO{{"send END SETUP to all locos"}};
        SETUP_LOOP[/loop\];
        GET_HORN_BUTTON[[get horn button]];
        GET_MAINS{mains = on?};
        IS_NEW{new == true};
        SELECT_LOCO{{"send SETUP MODE to loco[index]"}};
        END_NEW>new = false];
        subgraph select loco
            CHECK_HORN{horn button = true};
            STEP>index++];
            STEP_OVER{index < max_locos};
            RESET_INDEX>index = 0];
        end
        GET_DIR{dir switch};
        SET_FORWARD>dir = forward];
        SET_BACKWARD>dir = reverse];
        SEND_DIR{{send DIR to active loco}};
        END_LOOP[\end loop/];
    end
    END_SETUP[/send SAVE to active loco/];
    LED_OFF[/status led off/];
    RESET_LOCOS{{send END SETUP MODE\nto all locos}};
    ENDLOCOSETUP([return]);

    %% flow
    LOCOSETUP --> LED_SETUP --> SEL_FIRST -->|wait for horn button release| HORN_WAIT;
    HORN_WAIT --> |Y| HORN_WAIT;
    HORN_WAIT --> |N| SETUP_LOOP;

    SETUP_LOOP --> GET_HORN_BUTTON --> GET_MAINS{mains = on?};
    GET_MAINS --> |N| LED_OFF;
    GET_MAINS --> |Y| IS_NEW;
    IS_NEW --> |N| GET_DIR;
    IS_NEW --> |Y| INIT_LOCO --> SELECT_LOCO;
    SELECT_LOCO --> END_NEW --> GET_DIR;
    GET_DIR --> |off| CHECK_HORN;
    GET_DIR --> |forward| SET_FORWARD --> SEND_DIR;
    GET_DIR --> |backward| SET_BACKWARD --> SEND_DIR;
    CHECK_HORN --> |N| END_LOOP;
    CHECK_HORN --> |Y| STEP;
    SEND_DIR --> END_LOOP

    STEP --> STEP_OVER;
    STEP_OVER --> |N| RESET_INDEX --> END_LOOP;
    STEP_OVER --> |Y| END_LOOP;
    END_LOOP --> END_SETUP;
    END_SETUP --> LED_OFF --> RESET_LOCOS --> ENDLOCOSETUP;
```