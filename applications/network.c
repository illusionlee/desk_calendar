#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <smartconfig.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

extern void smartconfig_demo(void);
rt_uint8_t NETWORK_STATUS = 0;

void airkiss_config(void) {
    rt_err_t result;
    /* 配置 wifi 工作模式 */
    result = rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);
    if (result == RT_EOK) {
        LOG_D("Start airkiss...");
        /* 一键配网 demo */
        // smartconfig_demo(); // 可以考虑替换为airkissOpen包的
        airkiss_demo_start();
    }
}
static int network_status_callback(int argc, char **argv) {
    NETWORK_STATUS = 1;
    return 0;
}

void auto_connect_wifi(void) {
    /* 配置 wifi 工作模式 */
    rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);
    /* 注册 MQTT 启动函数为 WiFi 连接成功的回调函数 */
    rt_wlan_register_event_handler(RT_WLAN_EVT_READY, (void (*)(int, struct rt_wlan_buff *, void *))network_status_callback, RT_NULL);
    /* 初始化自动连接功能 */
    wlan_autoconnect_init();
    /* 使能 wlan 自动连接 */
    rt_wlan_config_autoreconnect(RT_TRUE);
}

MSH_CMD_EXPORT(airkiss_config, airkiss_config sample);