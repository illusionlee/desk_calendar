#include "defines.h"

time_t display_t;
time_t real_t;
struct tm *display_tm;
struct tm *real_tm;

rt_uint8_t RUN_FLAG = 1;

/* 指向信号量的指针 */
rt_sem_t sem_getdate = RT_NULL;
rt_sem_t sem_drawscreen = RT_NULL;

/* 消息队列控制块 */
struct rt_messagequeue mq;

/* 消息队列中用到的放置消息的内存池 */
rt_uint8_t msg_pool[32];

int init_mq(void) {
    rt_err_t result;
    /* 初始化消息队列 */
    result = rt_mq_init(&mq,
                        "key",
                        &msg_pool[0],           /* 内存池指向 msg_pool */
                        1,                      /* 每个消息的大小是 1 字节 */
                        sizeof(msg_pool),       /* 内存池的大小是 msg_pool 的大小 */
                        RT_IPC_FLAG_FIFO);      /* 按照先来先得到的方法分配消息 */
    if (result != RT_EOK) {
        rt_kprintf("init message queue failed.\n");
        return -1;
    }
    return 0;
}
void init_sem() {
    /* 创建一个动态信号量，初始值是 0 */
    sem_getdate = rt_sem_create("sem_getdate", 0, RT_IPC_FLAG_FIFO);
    sem_drawscreen = rt_sem_create("sem_drawscreen", 0, RT_IPC_FLAG_FIFO);
}
/* 集中初始化 */
void init_all(void) {
    fal_init();
    easyflash_init();
    init_mq();
    init_sem();
    init_keyboard();
    hwtimer_init();
}

void sync_app_time(void) {
    ntp_sync_to_rtc(NULL);
    display_t = time(NULL);
    display_tm = localtime(&display_t);
    real_t = time(NULL);
    real_tm = localtime(&real_t);
}

void stop_all(void) {
    RUN_FLAG = 0;
}
