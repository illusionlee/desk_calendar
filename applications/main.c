#include <rtthread.h>
#include <rtdevice.h>
#include "defines.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

int main(void) {
    init_all();
    start_calendar();
    return 0;
}
