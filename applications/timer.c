#include <rtthread.h>
#include <rtdevice.h>
#include "defines.h"

#define HWTIMER_DEV_NAME   "timer1"     /* 定时器名称 */
rt_device_t hw_dev = RT_NULL;           /* 定时器设备句柄 */

/* 定时器超时回调函数 */
static rt_err_t timeout_cb(rt_device_t dev, rt_size_t size) {
    go_today();
    return RT_EOK;
}

int hwtimer_init(void) {
    rt_err_t ret = RT_EOK;
    rt_hwtimer_mode_t mode;             /* 定时器模式 */

    /* 查找定时器设备 */
    hw_dev = rt_device_find(HWTIMER_DEV_NAME);
    if (hw_dev == RT_NULL) {
        return RT_ERROR;
    }

    /* 以读写方式打开设备 */
    ret = rt_device_open(hw_dev, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK) {
        rt_kprintf("open %s device failed!\n", HWTIMER_DEV_NAME);
        return ret;
    }

    /* 设置超时回调函数 */
    rt_device_set_rx_indicate(hw_dev, timeout_cb);
    /* 设置模式为单次定时器 */
    mode = HWTIMER_MODE_ONESHOT;
    ret = rt_device_control(hw_dev, HWTIMER_CTRL_MODE_SET, &mode);
    if (ret != RT_EOK) {
        rt_kprintf("set mode failed! ret is :%d\n", ret);
        rt_device_close(hw_dev);
        hw_dev = RT_NULL;
        return ret;
    }
    return ret;
}
rt_err_t start_timer(int sec) {
    rt_hwtimerval_t timeout_s;      /* 定时器超时值 */
    timeout_s.sec = sec;            /* 秒 */
    timeout_s.usec = 0;             /* 微秒 */
    if (hw_dev) {
        if (rt_device_write(hw_dev, 0, &timeout_s, sizeof(timeout_s)) != sizeof(timeout_s)) {
            rt_kprintf("set timeout value failed\n");
            rt_device_close(hw_dev);
            hw_dev = RT_NULL;
            return RT_ERROR;
        }
    } else {
        rt_kprintf("hw_dev is RT_NULL\n");
    }
    return RT_EOK;
}
