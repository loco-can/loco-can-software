# Controller flowchart
## Init
```mermaid
graph TD;
    START((power up)) --> INIT>active = false\nstatus = off];
    INIT --> INSTR_ON[/instrument light on/];
    INSTR_ON --> METER_MAX[["meter test(max)"]];
    METER_MAX --> LED[["status led test"]] --> METER_MIN[["meter test(min)"]];
    METER_MIN --> INSTR_OFF[/instrument light off/];
    INSTR_OFF --> INITEND[[main loop]];

    LED_TEST([status led test]) --> LED_RED;
    LED_RED[/status led RED/] --> WAIT_RED[wait 500ms] --> LED_ORANGE;
    LED_ORANGE[/status led ORANGE/] --> WAIT_ORANGE[wait 500ms] --> LED_GREEN;
    LED_GREEN[/status led GREEN/] --> WAIT_GREEN[wait 500ms] --> LED_OFF[/status led off/] --> LED_TEST_END([return]);

    METER_TEST(["meter test(val)"]) --> METER_TEST_LOOP[/loop meters\];
    METER_TEST_LOOP --> METER_SET[set meter to val] --> METER_TEST_LOOP_END[\loop meters end/];
    METER_TEST_LOOP_END --> METER_TEST_END([return]);
  ```

## Main
```mermaid
graph TD;

    LOOPSTART[/main loop\] --> GET_DATA[[get CAN data]] --> GET_SWITCHES[[get switches]] --> GET_POTS[[get potentiometer]] --> GET_STATI[[set status]] --> SWITCH_STATUS;
    SWITCH_STATUS{action by status};
    SWITCH_STATUS --> |off| ACTION_OFF;
    SWITCH_STATUS --> |standby| ACTION_STANDBY;
    SWITCH_STATUS --> |on| ACTION_ON;
    SWITCH_STATUS --> |ready| ACTION_READY;
    SWITCH_STATUS --> |moving| ACTION_MOVING;
    ACTION_OFF[[action off]] --> ENDLOOP;
    ACTION_STANDBY[[action standby]] --> ENDLOOP;
    ACTION_ON[[action on]] --> SEND_DATA;
    ACTION_READY[[action ready]] --> SEND_DATA;
    ACTION_MOVING[[action moving]] --> SEND_DATA;
    SEND_DATA[[send controller data]] --> ENDLOOP;
    ENDLOOP[\end main loop/];

    GET_CAN([get CAN data]) --> MESSAGE_AVAILABLE{message available};
    MESSAGE_AVAILABLE --> |N| PARSE_MESSAGES;
    MESSAGE_AVAILABLE --> |Y| GET_MESSAGE;
    GET_MESSAGE>"message_buffer[uuid] = message"] --> PARSE_MESSAGES;
    PARSE_MESSAGES[parse messages] --> END_CAN;
    END_CAN([return]);

    GETSWITCHES([get switches]) --> FETCH_DIGITAL>"flags.switches.mains = mains.get()\nflags.switches.dir = dir.get()\nflags.switches.light = light.get()\nflags.switches.light_1 = light_1.get()"] --> ENDSWITCHES;
    ENDSWITCHES([return]);

    GETPOTS([get pots]) --> FETCH_ANALOG>"driveval = analog.drive\nanalogval = analog.break\npowerval = analog.powe"] --> ENDPOTS;
    ENDPOTS([return]);

    GET_STATUS([set status]) --> CHECK_ACTIVE[["check for activatable"]] --> IS_ACTIVE{can be activated?};
    IS_ACTIVE --> |N| SET_ACT_OFF>active = false] --> STATUS_OFF>status = off] --> LED_STATUS;
    IS_ACTIVE --> |Y| IS_MAINS{can.status.mains = on?};
    IS_MAINS --> |N| STATUS_STANDBY>status = standby] --> LED_STATUS;
    IS_MAINS --> |Y| SET_ACT_ON>active = true] --> IS_DRIVE{drive on?};
    IS_DRIVE --> |N| STATUS_NO_DIR>status = no direction] --> LED_STATUS;
    IS_DRIVE --> |Y| CHECK_MOVING{is moving?};
    CHECK_MOVING --> |N| STATUS_READY>status = ready] --> LED_STATUS;
    CHECK_MOVING --> |Y| STATUS_MOVING>status = moving] --> LED_STATUS;
    LED_STATUS[[set status led]] --> END_STATUS([return]);

    SENDDATA([send controller data]) --> DATA_TIMEOUT{data timeout?};
    DATA_TIMEOUT --> |N| HB_TIMEOUT;
    DATA_TIMEOUT --> |Y| DATA;
    DATA{{"send data:\nstatus\nswitches\npotentiometer"}} --> HB_TIMEOUT{"controller\nheartbeat\ntimeout?"};
    HB_TIMEOUT --> |Y| SEND_HB{{send heartbeat}} --> ENDSENDDATA;
    HB_TIMEOUT -->|N| ENDSENDDATA;
    ENDSENDDATA([return]);
```
