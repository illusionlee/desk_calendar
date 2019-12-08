#include <rtthread.h>
#include <rtdevice.h>
#include "defines.h"

int start_screen(void) {
    screen_func();
    return 0;
}

int check_net_mode(void) {
    config_wifi_check(); // 检测配网按钮是否按下的线程
    return 0;
}

void start_airkiss(void) {
    screen_show_qrcode();
    airkiss_config();
}

int start_network(void) {
    check_net_mode();
    auto_connect_wifi();
    return 0;
}

int start_mqtt(void) {
    mqtt_start();
    return 0;
}

int start_keyboard(void) {
    keyscan_thread_func();
    return 0;
}

/* 启动日历主函数 */
void start_calendar(void) {
    start_screen();
    start_network();
    while (!NETWORK_STATUS) {   // 等待联网成功
        rt_kprintf("Waiting NETWORK_STATUS...\n");
        rt_thread_mdelay(1000); // sleep 1 second.
    }
    start_mqtt();
}
