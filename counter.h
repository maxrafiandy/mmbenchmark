#ifndef COUNTER_H_INCLUDED
#define COUNTER_H_INCLUDED

#include <iostream>
#include <math.h>
#include <iomanip>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <assert.h>
#define EVENT_SIZE 256
#include "cpucounters.h"
#include "utils.h"

#define PCM_DELAY_DEFAULT 1.0 // in seconds
#define PCM_DELAY_MIN 0.015 // 15 milliseconds is practical on most modern CPUs
#define PCM_CALIBRATION_INTERVAL 50 // calibrate clock only every 50th iteration

#define L1_HITS     "cpu/event=0xD1,umask=0x01"
#define L2_HITS     "cpu/event=0xD1,umask=0x02"
#define L3_HITS     "cpu/event=0xD1,umask=0x04"

#define L1_MISSES   "cpu/event=0xD1,umask=0x08"
#define L2_MISSES   "cpu/event=0xD1,umask=0x10"
#define L3_MISSES   "cpu/event=0xD1,umask=0x20"

#define NUMBER_EVENTS 4
#define EVENT_SIZE 256

struct CoreEvent
{
    char name[256];
    uint64 value;
    uint64 msr_value;
    char * description;
};

struct CostumeEvent
{
    uint64 l1_hits;
    uint64 l1_misses;
    uint64 l2_hits;
    uint64 l2_misses;
    uint64 l3_hits;
    uint64 l3_misses;
    double ipc;
};

using namespace std;

CoreEvent events[NUMBER_EVENTS];
CostumeEvent core_events[8], system_event;

template <class StateType>
CostumeEvent print_custom_stats(const StateType & BeforeState, const StateType & AfterState ,bool csv)
{
    CostumeEvent current_event;

    uint64 cycles = getCycles(BeforeState, AfterState);
    uint64 instr = getInstructionsRetired(BeforeState, AfterState);

    cout << fixed << double(instr)/double(cycles) << "    ";

    current_event.ipc = double(instr)/double(cycles);
    current_event.l1_hits = getNumberOfCustomEvents(0, BeforeState, AfterState);
    current_event.l2_hits = getNumberOfCustomEvents(1, BeforeState, AfterState);
    current_event.l3_hits = getNumberOfCustomEvents(2, BeforeState, AfterState);
    current_event.l3_misses = getNumberOfCustomEvents(3, BeforeState, AfterState);
    current_event.l2_misses = current_event.l3_hits + current_event.l3_misses;
    current_event.l1_misses = current_event.l2_hits + current_event.l2_misses;

    cout << unit_format(current_event.l1_hits) << "    ";
    cout << unit_format(current_event.l1_misses) << "   ";
    cout << unit_format(current_event.l2_hits) << "    ";
    cout << unit_format(current_event.l2_misses) << "   ";
    cout << unit_format(current_event.l3_hits) << "   ";
    cout << unit_format(current_event.l3_misses) << "   ";

    cout << endl;
    return current_event;
}

void build_event(const char * argv, EventSelectRegister *reg, int idx)
{
    char *token, *subtoken, *saveptr1, *saveptr2;
    char name[EVENT_SIZE], *str1, *str2;
    int j, tmp;
    uint64 tmp2;
    reg->value = 0;
    reg->fields.usr = 1;
    reg->fields.os = 1;
    reg->fields.enable = 1;

    memset(name,0,EVENT_SIZE);
    strncpy(name,argv,EVENT_SIZE-1);

    for (j = 1, str1 = name; ; j++, str1 = NULL)
    {
        token = strtok_r(str1, "/", &saveptr1);
        if (token == NULL)
            break;
        if(strncmp(token,"cpu",3) == 0)
            continue;

        for (str2 = token; ; str2 = NULL)
        {
            tmp = -1;
            subtoken = strtok_r(str2, ",", &saveptr2);
            if (subtoken == NULL)
                break;
            if(sscanf(subtoken,"event=%i",&tmp) == 1)
                reg->fields.event_select = tmp;
            else if(sscanf(subtoken,"umask=%i",&tmp) == 1)
                reg->fields.umask = tmp;
            else if(strcmp(subtoken,"edge") == 0)
                reg->fields.edge = 1;
            else if(sscanf(subtoken,"any=%i",&tmp) == 1)
                reg->fields.any_thread = tmp;
            else if(sscanf(subtoken,"inv=%i",&tmp) == 1)
                reg->fields.invert = tmp;
            else if(sscanf(subtoken,"cmask=%i",&tmp) == 1)
                reg->fields.cmask = tmp;
            else if(sscanf(subtoken,"in_tx=%i",&tmp) == 1)
                reg->fields.in_tx = tmp;
            else if(sscanf(subtoken,"in_tx_cp=%i",&tmp) == 1)
                reg->fields.in_txcp = tmp;
            else if(sscanf(subtoken,"pc=%i",&tmp) == 1)
                reg->fields.pin_control = tmp;
            else if(sscanf(subtoken,"offcore_rsp=%llx",&tmp2) == 1)
            {
                if(idx >= 2)
                {
                    cerr << "offcore_rsp must specify in first or second event only. idx=" << idx << endl;
                    throw idx;
                }
                events[idx].msr_value = tmp2;
            }
            else if(sscanf(subtoken,"name=%255s",events[idx].name) == 1);
            else
            {
                cerr << "Event '" << subtoken << "' is not supported. See the list of supported events"<< endl;
                throw subtoken;
            }

        }
    }
    events[idx].value = reg->value;
}

#endif // COUNTER_H_INCLUDED
