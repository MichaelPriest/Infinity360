// No-GPU launch target for verifying a title transition without a retail game.
#include "xsdk/log.h"
#include "xsdk/kernel.h"
#include "xsdk/thread.h"
int main(void){
    xsdk_log_write("PASS: INFINITY 360 launch target entered");
    for(unsigned i=0;i<120;i++){
        INPUT_STATE input={0};
        if(XamInputGetState(0,0,&input)==ERROR_SUCCESS && (input.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK)))break;
        (void)xsdk_thread_sleep(25);
    }
    xsdk_log_write("PASS: INFINITY 360 launch target exited");
    return 0;
}
