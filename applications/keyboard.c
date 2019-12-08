#include <rtthread.h>
#include <rtdevice.h>
#include "bs8116a.h"
#include "defines.h"

void config_wifi(void *args) {
    // rt_pin_detach_irq(PIN_KEY0); // 考虑在进入的时候，调用这个函数。
    rt_thread_mdelay(50);
    if (rt_pin_read(PIN_KEY0) == PIN_LOW) {
        rt_thread_mdelay(50);
        if (rt_pin_read(PIN_KEY0) == PIN_LOW) {
            rt_kprintf("KEY0 pressed!\n");
            start_airkiss();
        }
    }
    // rt_pin_attach_irq(PIN_KEY0, PIN_IRQ_MODE_FALLING, config_wifi, RT_NULL); // 如果使用了rt_pin_detach_irq，需要再重新绑定函数。
}

void config_wifi_check(void) {
    /* 按键0引脚为输入模式 */
    rt_pin_mode(PIN_KEY0, PIN_MODE_INPUT_PULLUP);
    /* 绑定中断，下降沿模式，回调函数名为beep_on */
    rt_pin_attach_irq(PIN_KEY0, PIN_IRQ_MODE_FALLING, config_wifi, RT_NULL);
    /* 使能中断 */
    rt_pin_irq_enable(PIN_KEY0, PIN_IRQ_ENABLE);
}

void touch_led(void) {
    rt_pin_mode(LED_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(LED_PIN, PIN_LOW);
    rt_kprintf("LED_PIN_LOW\n");
    rt_thread_mdelay(500);
    rt_pin_write(LED_PIN, PIN_HIGH);
    rt_kprintf("LED_PIN_HIGH\n");
    rt_thread_mdelay(500);
}

/* 芯片1对应的键值 */
static rt_uint8_t keyvalue1[15] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
/* 芯片2对应的键值 */
static rt_uint8_t keyvalue2[15] = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30};
/* 芯片3对应的键值 */
static rt_uint8_t keyvalue3[6]  = {31, KEY_LEFT, KEY_UP, KEY_OK, KEY_DOWN, KEY_RIGHT};
/* 按键的初始化数组 */
static rt_uint8_t touch_key_init_buff[22] = {
    0x00,                           /* 触发模式 */
    0x00, 0x83, 0xF3,               /* 固定协议 */
    0x98,                           /* 省电模式 */
    0x38, 0x38, 0x38, 0x38, 0x38,   /* 触摸阈值 */
    0x38, 0x38, 0x38, 0x38, 0x38,   /* 触摸阈值 */
    0x38, 0x38, 0x38, 0x38, 0x38,   /* 触摸阈值 */
    0x40,                           /* 中断模式 */
    0x96                            /* SUM校验和 */
};
/* 使用BS8116A芯片的数量 */
rt_uint8_t BS8116A_CHIP_NUMS = 3;

/* 按键触发调用函数 */
void got_key_callback(rt_uint8_t key_code) {
    if (REFRESH_END_FLAG == 1) {
        /* 发送消息到消息队列中 */
        int result = rt_mq_send(&mq, &key_code, 1);
        if (result != RT_EOK) {
            rt_kprintf("rt_mq_send ERR\n");
        }
        touch_led();
        start_timer(30);
        REFRESH_END_FLAG = 0;
    }
}

void init_keyboard(void) {
    /* 需要与 BS8116A_CHIP_NUMS 数量对应*/
    static struct bs8116a_irq_arg b_i_a[3];

    /* 芯片1的参数配置*/
    static struct bs8116a_key_arg b_k_a_1 = {
        .key_value = keyvalue1,
        .key_size = 15,
        .init_buff = touch_key_init_buff,
    };
    b_i_a[0].key = &b_k_a_1;
    b_i_a[0].irq_pin_id = 29;
    strcpy(b_i_a[0].bus_name, "i2c1soft");

    /* 芯片2的参数配置*/
    static struct bs8116a_key_arg b_k_a_2 = {
        .key_value = keyvalue2,
        .key_size = 15,
        .init_buff = touch_key_init_buff,
    };
    b_i_a[1].key = &b_k_a_2;
    b_i_a[1].irq_pin_id = 28;
    strcpy(b_i_a[1].bus_name, "i2c2soft");

    /* 芯片3的参数配置*/
    static struct bs8116a_key_arg b_k_a_3 = {
        .key_value = keyvalue3,
        .key_size = 6,
        .init_buff = touch_key_init_buff,
    };
    b_i_a[2].key = &b_k_a_3;
    b_i_a[2].irq_pin_id = 27;
    strcpy(b_i_a[2].bus_name, "i2c3soft");

    /* 申请i2c总线函数 */
    keypad_init(b_i_a);
    /* 对BS8116A芯片进行初始化配置，需要在上电后的8秒内调用 */
    init_buf_to_bus(b_i_a);
    /* 注册中断引脚 绑定中断函数 */
    key_irq_entry(b_i_a);
}
