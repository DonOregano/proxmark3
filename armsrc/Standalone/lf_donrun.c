//-----------------------------------------------------------------------------
// Lars Hagstrom, 2019
//
// This code is licensed to you under the terms of the GNU GPL, version 2 or,
// at your option, any later version. See the LICENSE.txt file for the text of
// the license.
//-----------------------------------------------------------------------------
// Code for a em410x mode
//-----------------------------------------------------------------------------
#include "lf_icerun.h"

void ModInfo(void) {
    DbpString("  LF EM410x mode - aka donrun (DonOregano)");
}

enum State {
            Read = 0,
            Write,
            Simulate,
            Unused_State,
            Error
};



void setLeds(enum State state)
{
    LEDsoff();
    switch(state)
    {
    case Read:
        LED(LED_A, 0);
        break;
    case Write:
        LED(LED_B, 0);
        break;
    case Simulate:
        LED(LED_C, 0);
        break;
    case Unused_State:
        LED(LED_D, 0);
        break;
    case Error:
        LED(LED_A, 0);
        LED(LED_B, 0);
        LED(LED_C, 0);
        LED(LED_D, 0);
        break;
    }
}

void blinkLeds(enum State state)
{
    LEDsoff();
    while (BUTTON_PRESS())
        WDT_HIT();
    SpinDelay(300);

    for (int i = 0; i < 3; ++i)
    {
        //turn led on
        setLeds(state);
        SpinDelay(300);

        //turn led off
        LEDsoff();
        SpinDelay(300);
    }

    setLeds(state);

}

int read();
int write();
int simulate();

void RunMod() {
    StandAloneMode();
    Dbprintf("[=] LF EM410x mode - aka donrun started");
    FpgaDownloadAndGo(FPGA_BITSTREAM_LF);

    enum State state = 0;

    uint32_t high = 0;
    uint32_t low = 0;

    // the main loop for your standalone mode
    for (;;) {
        setLeds(state);
        WDT_HIT();

        // exit from IceRun,   send a usbcommand.
        if (data_available()) break;

        SpinDelay(300);

        // Was our button held down or pressed?
        int button_pressed = BUTTON_HELD(1000);

        //If we have an error we clear our state and go again.
        if (button_pressed != 0 && state == Error)
        {
            while (BUTTON_PRESS())
                WDT_HIT();
            state = 0;
            continue;
        }

        if (button_pressed == -1)
        {
            Dbprintf("button pressed");

            state++;
            if (state >= Unused_State)
            {
                state = 0;
            }
        }
        else if (button_pressed == 1)
        {
            Dbprintf("button pressed and held");
            // wait for button to be released
            blinkLeds(state);
            int res = 0;

            switch (state)
            {
            case Read:
                res = read(&high, &low);
                break;

            case Write:
                res = write(high, low);
                break;

            case Simulate:
                res = simulate();
                break;

            default:
                res = 1;
            }

            if (res != 0)
            {
                state = Error;
            }
            else
            {
                blinkLeds(state);
            }

        }
    }

    DbpString("[=] exiting");
    LEDsoff();
}


int read(uint32_t* high, uint32_t* low)
{
    uint32_t tempHigh = 0;
    uint64_t tempLow = 0;

    CmdEM410xdemod(1, &tempHigh, &tempLow, 0);

    *low = tempLow & 0xffffffff;
    *high = (tempLow >> 32) & 0xff;

    if (tempLow == 0 && tempHigh == 0)
    {
        Dbprintf("[=] Error reading em410x card");
        return 1;
    }

    Dbprintf("[=] Read 0x%02x%08x", *high, *low);

    return 0;
}

int write(uint32_t high, uint32_t low)
{
    if (high == 0 && low == 0)
    {
        Dbprintf("[=] No data to write");
        return 1;
    }

    Dbprintf("[=] Attempting to write 0x%02x%08x", high, low);

    WriteEM410x(1, high, low);

    //sleep a bit before verification
    SpinDelay(1000);

    uint32_t verifyHigh, verifyLow;
    read(&verifyHigh, &verifyLow);

    if (verifyHigh != high || verifyLow != low)
    {
        Dbprintf("[=] Verification failed!");
        return 1;
    }
    Dbprintf("[=] Verification succeeded!");

    return 0;
}

int simulate()
{
    //Not implemented
    return 1;
}
