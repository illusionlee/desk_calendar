#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <u8g2_port.h>
#include "lunar_calendar.h"
#include "qrcode_array.h"
#include "defines.h"
extern rt_uint8_t RUN_FLAG;

#define WIDTH_T             296
#define HEIGHT_T            128
#define DAYS_AREA_WIDTH     76
#define QRCODE_WIDTH        100
#define QRCODE_HEIGHT       100

#define OLED_SPI_PIN_CLK    21 /* PB16 21 */
#define OLED_SPI_PIN_MOSI   23 /* PB18 23 */
#define OLED_SPI_PIN_RES    19 /* PB14 19 */
#define OLED_SPI_PIN_DC     22 /* PB17 22 */
#define OLED_SPI_PIN_CS     20 /* PB15 20 */
#define OLED_SPI_PIN_BUSY   18 /* PB13 18 */

const char *week_day[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
const char *month[] = {"1月", "2月", "3月", "4月", "5月", "6月", "7月", "8月", "9月", "10月", "11月", "12月"};
const char *day_str[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
    "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
    "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31"
};
CALENDAR calendar_data;

rt_err_t ret = RT_EOK;
u8g2_t u8g2;
u8g2_uint_t w_month = 0;
u8g2_uint_t font_height = 0;

int s_year, s_month, s_day;
int hr_blank = 3;
int hr_width = 2;
int line_num = 0;
int frame_size = 14;
int select_frame_width = 0;
int select_frame_height = 0;

static char pub_date_str[32] = {0};
static int select_frame_pos = 0;

rt_uint8_t REFRESH_END_FLAG = 0;

void welcome_message(void) {
    // u8g2_SetPowerSave(&u8g2, 0);
    u8g2_DrawUTF8(&u8g2, 10, 50, "设备启动中 正在联网 请稍候");
    u8g2_DrawUTF8(&u8g2, 60, 80, "如需重设WIFI 请点击配网按钮");
    u8g2_SendBuffer(&u8g2);
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetPowerSave(&u8g2, 1);
}

static void init_screen(void) {
    // Initialization
    u8g2_Setup_il3820_v2_296x128_f( &u8g2, U8G2_R2, u8x8_byte_4wire_sw_spi, u8x8_rt_gpio_and_delay);
    u8x8_SetPin(u8g2_GetU8x8(&u8g2), U8X8_PIN_SPI_CLOCK, OLED_SPI_PIN_CLK);
    u8x8_SetPin(u8g2_GetU8x8(&u8g2), U8X8_PIN_SPI_DATA, OLED_SPI_PIN_MOSI);
    u8x8_SetPin(u8g2_GetU8x8(&u8g2), U8X8_PIN_CS, OLED_SPI_PIN_CS);
    u8x8_SetPin(u8g2_GetU8x8(&u8g2), U8X8_PIN_DC, OLED_SPI_PIN_DC);
    u8x8_SetPin(u8g2_GetU8x8(&u8g2), U8X8_PIN_RESET, OLED_SPI_PIN_RES);

    u8g2_InitDisplay(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_wqy14_t_gb2312);
    // u8g2_SetPowerSave(&u8g2, 1);
}
void holiday_str(void) {
    int font_width = 1;
    font_width = u8g2_GetUTF8Width(&u8g2, calendar_data.festival);
    u8g2_DrawUTF8(
        &u8g2,
        (DAYS_AREA_WIDTH - font_width) > 0 ? ((DAYS_AREA_WIDTH - font_width) / 2) : 0,
        104,
        calendar_data.festival
    );
}
void draw_lunar(int year, int m, int day) {
    int font_width = 1;
    char str[32] = {'\0'};
    sun2lunar(year, m, day, str);
    printf("%s\n", str);
    font_width = u8g2_GetUTF8Width(&u8g2, str);
    u8g2_DrawUTF8(
        &u8g2,
        (DAYS_AREA_WIDTH - font_width) > 0 ? ((DAYS_AREA_WIDTH - font_width) / 2) : 0,
        124,
        str
    );
}
void draw_words(int line_num) {
    ++line_num;
    int len = strlen(calendar_data.words);
    if (((len / (15 * 3) + 1) + line_num) > 8) {
        if (line_num < 9) {
            u8g2_DrawUTF8(&u8g2,
                          (DAYS_AREA_WIDTH + 2 + hr_width + 2),
                          8 * (frame_size + 2) - 2,
                          "别让自己太累，记得休息。");
        }
        return;
    }
    int font_width = 1;
    int i;
    for ( i = 0; (i * 15 * 3) < len; ++i) {
        char dest[64];
        strncpy(dest, calendar_data.words + i * 15 * 3, 15 * 3);
        u8g2_DrawUTF8(&u8g2,
                      (DAYS_AREA_WIDTH + 2 + hr_width + 2),
                      (line_num + 1 + i) * (frame_size + 2) - 2,
                      dest);
    }
}

static void draw_day(rt_uint8_t m, rt_uint8_t day, rt_uint8_t wday) {
    int font_width = 1;

    u8g2_SetFont(&u8g2, u8g2_font_logisoso62_tn);

    font_width = u8g2_GetUTF8Width(&u8g2, day_str[day]);
    int x = (DAYS_AREA_WIDTH - font_width) > 0 ? ((DAYS_AREA_WIDTH - font_width) / 2) : 0;
    u8g2_DrawUTF8(&u8g2, x, 85, day_str[day]);

    u8g2_SetFont(&u8g2, u8g2_font_wqy14_t_gb2312);
    font_width = u8g2_GetUTF8Width(&u8g2, month[m]);
    u8g2_DrawUTF8(&u8g2, (DAYS_AREA_WIDTH - font_width), 12, month[m]);

    u8g2_DrawUTF8(&u8g2, 2, 12, week_day[wday]);

    holiday_str();
    u8g2_DrawBox(&u8g2, DAYS_AREA_WIDTH + 2, hr_blank, hr_width, HEIGHT_T - hr_blank * 2);
}
static void draw_todos(int pos, unsigned char change_status) {
    if (change_status) { // change_status 是1时，需要提交更新
        calendar_data.todo_list[select_frame_pos].status = (calendar_data.todo_list[select_frame_pos].status + 1) % 2;
        mqtt_update_things();
    }
    for (line_num = 0; line_num  < MAX_TODO_SIZE; line_num++) {
        if (rt_strcmp(calendar_data.todo_list[line_num].content, "")) {
            if (calendar_data.todo_list[line_num].status) {
                u8g2_DrawRBox(&u8g2,
                              (DAYS_AREA_WIDTH + 2 + hr_width + 2),
                              (line_num) * (frame_size + 2) + 2,
                              frame_size, frame_size, 3);
            } else {
                u8g2_DrawRFrame(&u8g2,
                                (DAYS_AREA_WIDTH + 2 + hr_width + 2),
                                (line_num) * (frame_size + 2) + 2,
                                frame_size, frame_size, 3);
            }

            u8g2_DrawUTF8(&u8g2,
                          (DAYS_AREA_WIDTH + 2 + hr_width + 2 + frame_size + 2),
                          (line_num + 1) * (frame_size + 2) - 2,
                          calendar_data.todo_list[line_num].content);
        } else {
            break;
        }
    }
    draw_words(line_num);

    select_frame_width = WIDTH_T - (DAYS_AREA_WIDTH + 2 + hr_width + 2);
    select_frame_height = frame_size;

    select_frame_pos += pos;
    if (line_num > 0) {
        if (select_frame_pos < 0) {
            select_frame_pos = line_num - 1;
        } else if (select_frame_pos >= line_num) {
            select_frame_pos = 0;
        }

        u8g2_DrawRFrame(&u8g2,/*  */
                        (DAYS_AREA_WIDTH + 2 + hr_width + 1),/* x */
                        (select_frame_pos) * (frame_size + 2) + 1, /* y */
                        select_frame_width,/* width */
                        select_frame_height + 2, /* height */
                        3/* r */
                       );
    }
}

void refresh_screen(void *parameter) {
    while (RUN_FLAG) {
        rt_sem_take(sem_drawscreen, RT_WAITING_FOREVER);
        u8g2_SetPowerSave(&u8g2, 0);
        u8g2_SendBuffer(&u8g2);
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetPowerSave(&u8g2, 1);
        REFRESH_END_FLAG = 1;
    }
}
void got_date_changed(void *parameter) {
    while (RUN_FLAG) {
        rt_sem_take(sem_getdate, RT_WAITING_FOREVER);

        sscanf(calendar_data.date, "%04d%02d%02d", &s_year, &s_month, &s_day);

        display_tm->tm_mday = s_day;
        display_tm->tm_mon = s_month - 1;
        display_tm->tm_year = s_year - 1900;
        display_t = mktime(display_tm);
        display_tm = localtime(&display_t);


        draw_day(s_month - 1, s_day - 1, display_tm->tm_wday);
        draw_lunar(s_year, s_month, s_day);
        draw_todos(0, 0);
        rt_sem_release(sem_drawscreen);
    }
}
static void update_day(rt_uint8_t year, rt_uint8_t month, rt_uint8_t day) { //需要获取日期和内容
    rt_snprintf(pub_date_str, sizeof(pub_date_str), "{\"date\":\"%04d%02d%02d\"}", (year + 1900), (month + 1), day);
    mqtt_get_date(pub_date_str);
}
void go_today(void) {
    real_t = time(NULL);
    real_tm = localtime(&real_t);
    display_t = real_t;
    display_tm = localtime(&display_t);
    update_day(display_tm->tm_year, display_tm->tm_mon, display_tm->tm_mday);
}
static void change_day(int pos) {
    display_t = mktime(display_tm);
    display_t += pos * 60 * 60 * 24;
    display_tm = localtime(&display_t);
    update_day(display_tm->tm_year, display_tm->tm_mon, display_tm->tm_mday);
}
static void check_day(unsigned int day) {
    display_tm->tm_mday = day;
    display_t = mktime(display_tm);
    display_tm = localtime(&display_t);
    update_day(display_tm->tm_year, display_tm->tm_mon, display_tm->tm_mday);
}

static void change_todos(int pos, int status) {
    draw_day(display_tm->tm_mon, (display_tm->tm_mday - 1), display_tm->tm_wday);
    draw_lunar(display_tm->tm_year + 1900, display_tm->tm_mon + 1, display_tm->tm_mday);
    draw_todos(pos, status); // status 是1时，需要提交更新
    rt_sem_release(sem_drawscreen);
}
static void screen_mq_func(void *parameter) {
    char buf = 0;
    while (RUN_FLAG) {
        if (rt_mq_recv(&mq, &buf, sizeof(buf), RT_WAITING_FOREVER) == RT_EOK) {
            if (KEY_LEFT == buf) {          // 左
                change_day(-1);
            } else if (KEY_RIGHT == buf) {  // 右
                change_day(1);
            } else if (KEY_UP == buf) {     // 上
                change_todos(-1, 0);
            } else if (KEY_DOWN == buf) {   // 下
                change_todos(1, 0);
            } else if (KEY_OK == buf) {     // 确定
                change_todos(0, 1);
            } else {
                check_day(buf);
            }
        }
        rt_thread_mdelay(50);
    }
    rt_mq_detach(&mq);
}
static void init_todo_lists(void) {
    rt_uint8_t i;
    for (i = 0; i < MAX_TODO_SIZE; ++i) {
        sprintf(calendar_data.todo_list[i].content, "%s", "");
        calendar_data.todo_list[i].status = 0;
    }
}
void screen_func(void) {
    init_todo_lists();
    init_screen();
    welcome_message();

    /* 屏幕接收按键事件线程 */
    static rt_thread_t screen_mq_thread = RT_NULL;
    screen_mq_thread = rt_thread_create("t_scr_mq", screen_mq_func, RT_NULL, 2048, 21, 10);
    if (screen_mq_thread != RT_NULL)
        rt_thread_startup(screen_mq_thread);


    static rt_thread_t tid_draw_screen = RT_NULL;
    tid_draw_screen = rt_thread_create("t_draw_scr", refresh_screen, RT_NULL, 1024, 20, 10);
    if (tid_draw_screen != RT_NULL)
        rt_thread_startup(tid_draw_screen);

    static rt_thread_t tid_gotdate = RT_NULL;
    tid_gotdate = rt_thread_create("t_gotdate", got_date_changed, RT_NULL, 2048, 19, 10);
    if (tid_gotdate != RT_NULL)
        rt_thread_startup(tid_gotdate);
}
void get_dev_mac(char *mac_addr) {
    rt_uint8_t mac[6] = {0};
    /* 获得 MAC 地址 */
    if (rt_wlan_get_mac((rt_uint8_t *)mac) != RT_EOK) {
        rt_kprintf("get mac addr err!!");
        return ;
    }
    sprintf(mac_addr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
void screen_show_qrcode(void) {
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawXBMP(&u8g2, 14, 14, QRCODE_WIDTH, QRCODE_HEIGHT, qrcode_array);
    u8g2_SetFont(&u8g2, u8g2_font_wqy14_t_gb2312);
    u8g2_DrawUTF8(&u8g2, 128, 56, "微信扫码联网");

    char mac_addr[16];
    get_dev_mac(mac_addr);
    u8g2_DrawUTF8(&u8g2, 130, 80, mac_addr);

    u8g2_SendBuffer(&u8g2);
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetPowerSave(&u8g2, 1);
}
