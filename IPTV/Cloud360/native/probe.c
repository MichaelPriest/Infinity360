/*
 * No-GPU isolation test. A console that has its Aurora menu distorted after
 * this test has a problem unrelated to our 2D render code.
 *
 * Deliberately does NOT change display mode or take ownership of the GPU.
 */
#include <stdint.h>
#include "xsdk/kernel.h"
#include "xsdk/thread.h"
#include "xsdk/log.h"

int main(void)
{
    xsdk_log_write("PASS: Cloud360 no-GPU probe started");
    for(unsigned i=0;i<160;i++) {
        INPUT_STATE input={0};
        if(XamInputGetState(0,0,&input)==ERROR_SUCCESS &&
           (input.gamepad.buttons&(INPUT_GAMEPAD_BACK|INPUT_GAMEPAD_B)))
            break;
        (void)xsdk_thread_sleep(25);
    }
    xsdk_log_write("PASS: Cloud360 no-GPU probe returned cleanly");
    return 0;
}
