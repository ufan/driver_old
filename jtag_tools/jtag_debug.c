/*
 * $ZEL: jtag_debug.c,v 1.1 2003/08/30 21:23:17 wuestner Exp $
 */

#include "jtag_tools.h"

enum tap_state {
    TEST_LOGIC_RESET,
    RUN_TEST_IDLE,
    SELECT_DR_SCAN,
    CAPTURE_DR,
    SHIFT_DR,
    EXIT1_DR,
    PAUSE_DR,
    EXIT2_DR,
    UPDATE_DR,
    SELECT_IR_SCAN,
    CAPTURE_IR,
    SHIFT_IR,
    EXIT1_IR,
    PAUSE_IR,
    EXIT2_IR,
    UPDATE_IR,
};

static const char* tap_names[16]={
    "TEST_LOGIC_RESET",
    "RUN_TEST_IDLE",
    "SELECT_DR_SCAN",
    "CAPTURE_DR",
    "SHIFT_DR",
    "EXIT1_DR",
    "PAUSE_DR",
    "EXIT2_DR",
    "UPDATE_DR",
    "SELECT_IR_SCAN",
    "CAPTURE_IR",
    "SHIFT_IR",
    "EXIT1_IR",
    "PAUSE_IR",
    "EXIT2_IR",
    "UPDATE_IR",
};

static enum tap_state tap_trans[16][2]= {
    /*TEST_LOGIC_RESET*/ {RUN_TEST_IDLE, TEST_LOGIC_RESET},
    /*RUN_TEST_IDLE   */ {RUN_TEST_IDLE, SELECT_DR_SCAN},
    /*SELECT_DR_SCAN  */ {CAPTURE_DR, SELECT_IR_SCAN},
    /*CAPTURE_DR      */ {SHIFT_DR, EXIT1_DR},
    /*SHIFT_DR        */ {SHIFT_DR, EXIT1_DR},
    /*EXIT1_DR        */ {PAUSE_DR, UPDATE_DR},
    /*PAUSE_DR        */ {PAUSE_DR, EXIT2_DR},
    /*EXIT2_DR        */ {SHIFT_DR, UPDATE_DR},
    /*UPDATE_DR       */ {RUN_TEST_IDLE, SELECT_DR_SCAN},
    /*SELECT_IR_SCAN  */ {CAPTURE_IR, TEST_LOGIC_RESET},
    /*CAPTURE_IR      */ {SHIFT_IR, EXIT1_IR},
    /*SHIFT_IR        */ {SHIFT_IR, EXIT1_IR},
    /*EXIT1_IR        */ {PAUSE_IR, UPDATE_IR},
    /*PAUSE_IR        */ {PAUSE_IR, EXIT2_IR},
    /*EXIT2_IR        */ {SHIFT_IR, UPDATE_IR},
    /*UPDATE_IR       */ {RUN_TEST_IDLE, SELECT_DR_SCAN},
};

static enum tap_state currentstate=TEST_LOGIC_RESET;
static int counter=0;

const char*
newstate(int tms, int* count)
{
    enum tap_state new_state;
    new_state=tap_trans[currentstate][!!tms];
    if (new_state==currentstate)
        counter++;
    else
        counter=0;
    currentstate=new_state;
    *count=counter;
    return tap_names[new_state];
}
