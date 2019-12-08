#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "paho_mqtt.h"
#include "cJSON.h"
#include "defines.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* define MQTT client context */
static MQTTClient client;
static int is_started             = 0;
static rt_uint8_t mac[6]          = {0};
static char sub_get_things[48]    = {0};
static char pub_get_date[48]      = {0};
static char pub_update_things[48] = {0};

char* create_json_str(void) {
    int index         = 0;
    char *json_str    = NULL;
    cJSON *js_date    = NULL;
    cJSON *js_things  = NULL;
    cJSON *js_thing   = NULL;
    cJSON *js_content = NULL;
    cJSON *js_status  = NULL;

    cJSON *js_root = cJSON_CreateObject();
    if (js_root == NULL) {
        goto end;
    }

    js_date = cJSON_CreateString(calendar_data.date);
    if (js_date == NULL) {
        goto end;
    }
    cJSON_AddItemToObject(js_root, "date", js_date);

    js_things = cJSON_CreateArray();
    if (js_things == NULL) {
        goto end;
    }
    cJSON_AddItemToObject(js_root, "things", js_things);

    for (index = 0; index < MAX_TODO_SIZE; ++index) {
        js_thing = cJSON_CreateObject();
        if (js_thing == NULL) {
            goto end;
        }
        if (rt_strcmp(calendar_data.todo_list[index].content, "")) {

            js_content = cJSON_CreateString(calendar_data.todo_list[index].content);
            if (js_content == NULL) {
                goto end;
            }
            cJSON_AddItemToObject(js_thing, "content", js_content);

            js_status = cJSON_CreateNumber(calendar_data.todo_list[index].status);
            if (js_status == NULL) {
                goto end;
            }
            cJSON_AddItemToObject(js_thing, "status", js_status);

            cJSON_AddItemToArray(js_things, js_thing);
        }
    }

    json_str = cJSON_PrintUnformatted(js_root);
    if (json_str == NULL) {
        printf("Failed to print js_root.\n");
    }

end:
    cJSON_Delete(js_root);
    return json_str;
}

int parse_json(const char * const json_str) {
    const cJSON *js_things = NULL;
    const cJSON *js_thing  = NULL;
    cJSON *js_content      = NULL;
    cJSON *js_status       = NULL;
    cJSON *js_date         = NULL;
    cJSON *js_festival     = NULL;
    cJSON *js_words        = NULL;
    cJSON *js_todos        = cJSON_Parse(json_str);
    if (js_todos == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error before: %s\n", error_ptr);
        }
        goto parse_json_end;
    }

    rt_uint8_t i, array_size;

    js_date = cJSON_GetObjectItem(js_todos, "date");
    sprintf(calendar_data.date, "%s", js_date->valuestring);

    js_things = cJSON_GetObjectItem(js_todos, "things");
    array_size = cJSON_GetArraySize(js_things);
    for (i = 0; i < MAX_TODO_SIZE; i++) { // 当实际事务小于最大允许数据，需要进行置空处理
        if (i < array_size) {
            js_thing = cJSON_GetArrayItem(js_things, i);
            if (NULL == js_thing) {
                printf("Fail to parse array json\n");
                return -1;
            }
            js_content = cJSON_GetObjectItem(js_thing, "content");
            sprintf(calendar_data.todo_list[i].content, "%s", js_content->valuestring);
            js_status = cJSON_GetObjectItem(js_thing, "status");
            calendar_data.todo_list[i].status = js_status->valueint;
        } else {
            sprintf(calendar_data.todo_list[i].content, "%s", "");
            calendar_data.todo_list[i].status = 0;
        }
    }

    js_words = cJSON_GetObjectItem(js_todos, "words");
    if (js_words == NULL) {
        sprintf(calendar_data.words, "");
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error before: %s\n", error_ptr);
        }
        goto parse_json_end;
    }
    sprintf(calendar_data.words, "%s", js_words->valuestring);

    js_festival = cJSON_GetObjectItem(js_todos, "festival");
    if (js_festival == NULL) {
        sprintf(calendar_data.festival, "");
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error before: %s\n", error_ptr);
        }
        goto parse_json_end;
    }
    sprintf(calendar_data.festival, "%s", js_festival->valuestring);

parse_json_end:
    cJSON_Delete(js_todos);
    rt_sem_release(sem_getdate);
    return 0;
}

int mqtt_get_date(char *msg) {
    if (is_started == 0) {
        LOG_E("mqtt client is not connected.");
        return -1;
    }
    paho_mqtt_publish(&client, QOS1, pub_get_date, msg);
    return 0;
}
int mqtt_update_things(void) {
    if (is_started == 0) {
        LOG_E("mqtt client is not connected.");
        return -1;
    }
    paho_mqtt_publish(&client, QOS1, pub_update_things, create_json_str());
    return 0;
}

static void mqtt_sub_callback(MQTTClient *c, MessageData *msg_data) {
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    LOG_D("mqtt sub callback: %.*s %.*s",
          msg_data->topicName->lenstring.len,
          msg_data->topicName->lenstring.data,
          msg_data->message->payloadlen,
          (char *)msg_data->message->payload);

    parse_json((char *)msg_data->message->payload);
}

static void mqtt_sub_default_callback(MQTTClient *c, MessageData *msg_data) {
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    LOG_D("mqtt sub default callback: %.*s %.*s",
          msg_data->topicName->lenstring.len,
          msg_data->topicName->lenstring.data,
          msg_data->message->payloadlen,
          (char *)msg_data->message->payload);
}

static void mqtt_connect_callback(MQTTClient *c) {
    LOG_D("inter mqtt_connect_callback!");
}

static void mqtt_online_callback(MQTTClient *c) {
    LOG_D("inter mqtt_online_callback!");
    // 当网络连接成功，需要从服务器获取一下当前日期的todolist
    sync_app_time();
    go_today();
}

static void mqtt_offline_callback(MQTTClient *c) {
    LOG_D("inter mqtt_offline_callback!");
}

int mqtt_start(void) {
    /* 获得 MAC 地址 */
    if (rt_wlan_get_mac((rt_uint8_t *)mac) != RT_EOK) {
        LOG_E("get mac addr err!! exit");
        return -RT_ERROR;
    }
    char mac_addr[16];
    LOG_I("MAC:%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    sprintf(mac_addr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    rt_snprintf(pub_get_date, sizeof(pub_get_date), "dev/%s/%s", mac_addr, MQTT_PUBTOPIC_GETDATE);
    rt_snprintf(pub_update_things, sizeof(pub_update_things), "dev/%s/%s", mac_addr, MQTT_PUBTOPIC_UPDATE);
    rt_snprintf(sub_get_things, sizeof(sub_get_things), "dev/%s/%s", mac_addr, MQTT_SUBTOPIC);


    /* init condata param by using MQTTPacket_connectData_initializer */
    MQTTPacket_connectData condata = MQTTPacket_connectData_initializer;
    static char cid[20] = { 0 };

    if (is_started) {
        LOG_E("mqtt client is already connected.");
        return -1;
    }
    /* config MQTT context param */
    {
        client.isconnected = 0;
        client.uri = MQTT_URI;

        /* generate the random client ID */
        rt_snprintf(cid, sizeof(cid), "calendar-%d", rt_tick_get());
        /* config connect param */
        memcpy(&client.condata, &condata, sizeof(condata));
        client.condata.clientID.cstring = cid;
        client.condata.keepAliveInterval = 30;
        client.condata.cleansession = 1;
        client.condata.username.cstring = MQTT_USERNAME;
        client.condata.password.cstring = MQTT_PASSWORD;

        /* config MQTT will param. */
        client.condata.willFlag = 1;
        client.condata.will.qos = 1;
        client.condata.will.retained = 0;
        client.condata.will.topicName.cstring = pub_get_date;
        client.condata.will.message.cstring = MQTT_WILLMSG;

        /* malloc buffer. */
        client.buf_size = client.readbuf_size = 1024;
        client.buf = rt_calloc(1, client.buf_size);
        client.readbuf = rt_calloc(1, client.readbuf_size);
        if (!(client.buf && client.readbuf)) {
            LOG_E("no memory for MQTT client buffer!");
            return -1;
        }

        /* set event callback function */
        client.connect_callback = mqtt_connect_callback;
        client.online_callback = mqtt_online_callback;
        client.offline_callback = mqtt_offline_callback;

        /* set subscribe table and event callback */
        client.messageHandlers[0].topicFilter = rt_strdup(sub_get_things);
        client.messageHandlers[0].callback = mqtt_sub_callback;
        client.messageHandlers[0].qos = QOS1;

        /* set default subscribe event callback */
        client.defaultMessageHandler = mqtt_sub_default_callback;
    }

    /* run mqtt client */
    paho_mqtt_start(&client);
    is_started = 1;

    return 0;
}

static void mqtt_new_sub_callback(MQTTClient *client, MessageData *msg_data) {
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    LOG_D("mqtt new subscribe callback: %.*s %.*s",
          msg_data->topicName->lenstring.len,
          msg_data->topicName->lenstring.data,
          msg_data->message->payloadlen,
          (char *)msg_data->message->payload);
}
