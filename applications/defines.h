#ifndef _DEFINES_H_
#define _DEFINES_H_
#include <stdlib.h>
#include <easyflash.h>
#include <fal.h>
#include <paho_mqtt.h>
#include "board.h"

extern rt_uint8_t NETWORK_STATUS;
extern rt_uint8_t RUN_FLAG;
extern rt_uint8_t CONFIG_WIFI_FLAG;

/* 消息队列控制块 */
extern struct rt_messagequeue mq;

/* 消息队列中用到的放置消息的内存池 */
extern rt_uint8_t msg_pool[32];

#define MAX_TODO_SIZE  8
typedef struct {
	char content[64];
	short status;
} TODO;
typedef struct {
	char date[16];
	TODO todo_list[MAX_TODO_SIZE];
	char festival[64];
	char words[256];
} CALENDAR;
extern CALENDAR calendar_data;

/* 指向信号量的指针 */
extern rt_sem_t sem_getdate;
extern rt_sem_t sem_drawscreen;

/* timer */
/* 定时器初始化 */
extern int hwtimer_init(void);
/* 开始计时sec秒 */
extern rt_err_t start_timer(int sec);
/* timer */

/* screen */
/* 屏幕是否刷新完成标志位 */
extern rt_uint8_t REFRESH_END_FLAG;
/* 显示配网二维码 */
extern void screen_show_qrcode(void);
/* 启动屏幕 */
extern void screen_func(void);
/* 将屏幕显示今天的日期 */
extern void go_today(void);
/* screen */

/* keyboard */
/* 键值定义 */
#define PIN_KEY0    13
#define LED_PIN     14
#define KEY_LEFT    32
#define KEY_UP      33
#define KEY_OK      34
#define KEY_DOWN    35
#define KEY_RIGHT   36
/* 初始化触摸键盘 */
extern void init_keyboard(void);
/* keyboard */

/* network */
/* 配网函数 */
extern void airkiss_config(void);
/* 自动联网 */
extern void auto_connect_wifi(void);
/* 开启配网状态 */
extern int airkiss_demo_start(void);
/* network */

/* mqtt */
/* 参数配置 */
#define MQTT_URI                "tcp://123.456.789.012:1883"
#define MQTT_USERNAME           "username"
#define MQTT_PASSWORD           "password"
#define MQTT_WILLMSG            "bye"
/* 订阅和发布主题 */
#define MQTT_SUBTOPIC           "get/things"
#define MQTT_PUBTOPIC_GETDATE   "get/date"
#define MQTT_PUBTOPIC_UPDATE    "update/things"
/* 启动MQTT */
extern int mqtt_start(void);
/* 发布日期消息 */
extern int mqtt_get_date(char *msg);
/* 发布备忘录消息 */
extern int mqtt_update_things(void);
/* mqtt */

/* init */
/* 日期时间配置变量 */
extern time_t display_t;
extern time_t real_t;
extern struct tm *display_tm;
extern struct tm *real_tm;
/* 集中初始化 */
extern void init_all(void);
/* 同步网络时间 */
extern void sync_app_time(void);
/* init */

/* logic */
/* 开启配网 */
extern void start_airkiss(void);
/* 启动日历主函数 */
extern void start_calendar(void);
/* logic */

#endif