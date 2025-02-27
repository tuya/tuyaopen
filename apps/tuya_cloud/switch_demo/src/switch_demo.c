/**
 * @file switch_demo.c
 *
 * @brief a simple switch demo show how to use tuya-open-sdk-for-device to
 * develop a simple switch. 1, download, compile, run in ubuntu according the
 * README.md 2, binding the device use tuya APP accoring scan QRCode 3, on/off
 * the switch in tuya APP 4, "switch on/off" use cli
 * @copyright Copyright (c) 2021-2024 Tuya Inc. All Rights Reserved.
 */

#include "cJSON.h"
#include "netmgr.h"
#include "tal_api.h"
#include "tkl_output.h"
#include "tuya_config.h"
#include "tuya_iot.h"
#include "tuya_iot_dp.h"
#include <assert.h>
#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
#include "netconn_wifi.h"
#endif
#if defined(ENABLE_WIRED) && (ENABLE_WIRED == 1)
#include "netconn_wired.h"
#endif
#if defined(ENABLE_LIBLWIP) && (ENABLE_LIBLWIP == 1)
#include "lwip_init.h"
#endif

#ifndef PROJECT_VERSION
#define PROJECT_VERSION "1.0.0"
#endif

#include "tkl_gpio.h"
#include "tkl_pwm.h"
#include "c_source.h"
extern const unsigned char dataa[];
#include "tkl_uart.h"

/* for string to QRCode translate and print */
extern void example_qrcode_string(const char *string, void (*fputs)(const char *str), int invert);

/* for cli command register */
extern void tuya_app_cli_init(void);

/* Tuya device handle */
tuya_iot_client_t client;

/**
 * @brief user defined log output api, in this demo, it will use uart0 as log-tx
 *
 * @param str log string
 * @return void
 */
void user_log_output_cb(const char *str)
{
    tkl_log_output(str);
}

/**
 * @brief user defined upgrade notify callback, it will notify device a OTA
 * request received
 *
 * @param client device info
 * @param upgrade the upgrade request info
 * @return void
 */
void user_upgrade_notify_on(tuya_iot_client_t *client, cJSON *upgrade)
{
    PR_INFO("----- Upgrade information -----");
    PR_INFO("OTA Channel: %d", cJSON_GetObjectItem(upgrade, "type")->valueint);
    PR_INFO("Version: %s", cJSON_GetObjectItem(upgrade, "version")->valuestring);
    PR_INFO("Size: %s", cJSON_GetObjectItem(upgrade, "size")->valuestring);
    PR_INFO("MD5: %s", cJSON_GetObjectItem(upgrade, "md5")->valuestring);
    PR_INFO("HMAC: %s", cJSON_GetObjectItem(upgrade, "hmac")->valuestring);
    PR_INFO("URL: %s", cJSON_GetObjectItem(upgrade, "url")->valuestring);
    PR_INFO("HTTPS URL: %s", cJSON_GetObjectItem(upgrade, "httpsUrl")->valuestring);
}

/**
 * @brief user defined event handler
 *
 * @param client device info
 * @param event the event info
 * @return void
 */
void user_event_handler_on(tuya_iot_client_t *client, tuya_event_msg_t *event)
{
    PR_DEBUG("Tuya Event ID:%d(%s)", event->id, EVENT_ID2STR(event->id));
    PR_INFO("Device Free heap %d", tal_system_get_free_heap_size());
    switch (event->id) {
    case TUYA_EVENT_BIND_START:
        PR_INFO("Device Bind Start!");
        break;

    /* Print the QRCode for Tuya APP bind */
    case TUYA_EVENT_DIRECT_MQTT_CONNECTED: {
        char buffer[255];
        sprintf(buffer, "https://smartapp.tuya.com/s/p?p=%s&uuid=%s&v=2.0", TUYA_PRODUCT_KEY, TUYA_OPENSDK_UUID);
        example_qrcode_string(buffer, user_log_output_cb, 0);
    } break;

    /* MQTT with tuya cloud is connected, device online */
    case TUYA_EVENT_MQTT_CONNECTED:
        PR_INFO("Device MQTT Connected!");
        break;

    /* RECV upgrade request */
    case TUYA_EVENT_UPGRADE_NOTIFY:
        user_upgrade_notify_on(client, event->value.asJSON);
        break;

    /* Sync time with tuya Cloud */
    case TUYA_EVENT_TIMESTAMP_SYNC:
        PR_INFO("Sync timestamp:%d", event->value.asInteger);
        tal_time_set_posix(event->value.asInteger, 1);
        break;
    case TUYA_EVENT_RESET:
        PR_INFO("Device Reset:%d", event->value.asInteger);
        break;

    /* RECV OBJ DP */
    case TUYA_EVENT_DP_RECEIVE_OBJ: {
        dp_obj_recv_t *dpobj = event->value.dpobj;
        PR_DEBUG("SOC Rev DP Cmd t1:%d t2:%d CNT:%u", dpobj->cmd_tp, dpobj->dtt_tp, dpobj->dpscnt);
        if (dpobj->devid != NULL) {
            PR_DEBUG("devid.%s", dpobj->devid);
        }

        uint32_t index = 0;
        for (index = 0; index < dpobj->dpscnt; index++) {
            dp_obj_t *dp = dpobj->dps + index;
            PR_DEBUG("idx:%d dpid:%d type:%d ts:%u", index, dp->id, dp->type, dp->time_stamp);
            switch (dp->type) {
            case PROP_BOOL: {
                PR_DEBUG("bool value:%d", dp->value.dp_bool);
                break;
            }
            case PROP_VALUE: {
                PR_DEBUG("int value:%d", dp->value.dp_value);
                break;
            }
            case PROP_STR: {
                PR_DEBUG("str value:%s", dp->value.dp_str);
                break;
            }
            case PROP_ENUM: {
                PR_DEBUG("enum value:%u", dp->value.dp_enum);
                break;
            }
            case PROP_BITMAP: {
                PR_DEBUG("bits value:0x%X", dp->value.dp_bitmap);
                break;
            }
            default: {
                PR_ERR("idx:%d dpid:%d type:%d ts:%u is invalid", index, dp->id, dp->type, dp->time_stamp);
                break;
            }
            } // end of switch
        }

        tuya_iot_dp_obj_report(client, dpobj->devid, dpobj->dps, dpobj->dpscnt, 0);

    } break;

    /* RECV RAW DP */
    case TUYA_EVENT_DP_RECEIVE_RAW: {
        dp_raw_recv_t *dpraw = event->value.dpraw;
        PR_DEBUG("SOC Rev DP Cmd t1:%d t2:%d", dpraw->cmd_tp, dpraw->dtt_tp);
        if (dpraw->devid != NULL) {
            PR_DEBUG("devid.%s", dpraw->devid);
        }

        uint32_t index = 0;
        dp_raw_t *dp = &dpraw->dp;
        PR_DEBUG("dpid:%d type:RAW len:%d data:", dp->id, dp->len);
        for (index = 0; index < dp->len; index++) {
            PR_DEBUG_RAW("%02x", dp->data[index]);
        }

        tuya_iot_dp_raw_report(client, dpraw->devid, &dpraw->dp, 3);

    } break;

        /* TBD.. add other event if necessary */

    default:
        break;
    }
}

/**
 * @brief user defined network check callback, it will check the network every
 * 1sec, in this demo it alwasy return ture due to it's a wired demo
 *
 * @return true
 * @return false
 */
bool user_network_check(void)
{
    netmgr_status_e status = NETMGR_LINK_DOWN;
    netmgr_conn_get(NETCONN_AUTO, NETCONN_CMD_STATUS, &status);
    return status == NETMGR_LINK_DOWN ? false : true;
}



TUYA_UART_BASE_CFG_T *cfg;
cfg.baudrate = 115200;
cfg.databits = TUYA_UART_DATA_LEN_8BIT;
cfg.stopbits = TUYA_UART_STOP_LEN_1BIT;
cfg.parity = TUYA_UART_PARITY_TYPE_NONE;
cfg.flowctrl = TUYA_UART_FLOWCTRL_NONE;
tkl_uart_init(TUYA_UART_NUM_2, &cfg);

char *buff="hello12312312312312";
tkl_uart_write(TUYA_UART_NUM_2, buff, strlen(buff));

//tkl_uart_read(TUYA_UART_NUM_2, void *buff, uint16_t len)



//
// typedef __UINT8_TYPE__ __uint8_t;
// typedef __uint8_t uint8_t ;
// // #define size_t long unsigned int
// //typedef __SIZE_TYPE__ size_t;

// typedef struct http_client_response {
//     /**
//      * @brief Buffer for both the raw HTTP header and body.
//      *
//      * This buffer is supplied by the application.
//      */
//     uint8_t *buffer;
//     size_t buffer_length; /**< The length of the response buffer in bytes. */

//     /**
//      * @brief The starting location of the response headers in buffer.
//      */
//     const uint8_t *headers;
//     size_t headers_length;

//     /**
//      * @brief The starting location of the response body in buffer.
//      */
//     const uint8_t *body;
//     size_t body_length;

//     /**
//      * @brief The HTTP response Status-Code.
//      */
//     //uint16_t status_code;
// } http_client_response_t;

static void delay(void)
{
    //for (int i = 0 ; i < 1000 ; i++) {//end 62.5ms
        for (int i = 0 ; i < 20000 ; i++) {//end ms
            for (int i = 0 ; i < 1000 ; i++) {//end 38.46us
                __asm__ __volatile__("nop");
            }
        }
    //}
}

void user_main()
{
    int ret = OPRT_OK;

    //! open iot development kit runtim init
    cJSON_InitHooks(&(cJSON_Hooks){.malloc_fn = tal_malloc, .free_fn = tal_free});
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 1024, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    tal_kv_init(&(tal_kv_cfg_t){
        .seed = "vmlkasdh93dlvlcy",
        .key = "dflfuap134ddlduq",
    });
    tal_sw_timer_init();
    tal_workq_init();
    tal_cli_init();
    tuya_app_cli_init();

    tuya_iot_license_t license;

    if (OPRT_OK != tuya_iot_license_read(&license)) {
        license.uuid = TUYA_OPENSDK_UUID;
        license.authkey = TUYA_OPENSDK_AUTHKEY;
        PR_WARN("Replace the TUYA_OPENSDK_UUID and TUYA_OPENSDK_AUTHKEY contents, otherwise the demo cannot work.\n \
                Visit https://platform.tuya.com/purchase/index?type=6 to get the open-sdk uuid and authkey.");
    }
    // PR_DEBUG("uuid %s, authkey %s", license.uuid, license.authkey);
    /* Initialize Tuya device configuration */
    ret = tuya_iot_init(&client, &(const tuya_iot_config_t){
                                     .software_ver = PROJECT_VERSION,
                                     .productkey = TUYA_PRODUCT_KEY,
                                     .uuid = license.uuid,
                                     .authkey = license.authkey,
                                     .event_handler = user_event_handler_on,
                                     .network_check = user_network_check,
                                 });
    assert(ret == OPRT_OK);

#if defined(ENABLE_LIBLWIP) && (ENABLE_LIBLWIP == 1)
    TUYA_LwIP_Init();
#endif

    // network init
    netmgr_type_e type = 0;
#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
    type |= NETCONN_WIFI;
#endif
#if defined(ENABLE_WIRED) && (ENABLE_WIRED == 1)
    type |= NETCONN_WIRED;
#endif
    netmgr_init(type);

#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
    netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_NETCFG, &(netcfg_args_t){.type = NETCFG_TUYA_BLE | NETCFG_TUYA_WIFI_AP});
#endif

    PR_DEBUG("tuya_iot_init success");
    /* Start tuya iot task */
    tuya_iot_start(&client);

    http_client_response_t http_response = {0};
    //uint8_t body_content = 1;
    uint32_t body_buf[5] = {90,180,250,180,190};//char body_buf[8] = {0,1,1};
    http_response.body = (uint8_t *)body_buf;
    http_response.body_length = sizeof(body_buf);//sizeof

    //pwm
    TUYA_PWM_BASE_CFG_T pwm_cfg;
    // 设置PWM极性，假设使用正极性
    pwm_cfg.polarity = TUYA_PWM_POSITIVE; 
    // 设置计数模式，假设使用向上计数模式
    pwm_cfg.count_mode = TUYA_PWM_CNT_UP; 
    // 设置占空比，假设为 ?0%
    pwm_cfg.duty = 130;//50; 0-270
    // 设置周期
    //pwm_cfg.cycle = 10000;//100; 
    // 设置频率，假设为10000Hz
    pwm_cfg.frequency = 16000;//1396;//65533;//10000; //must
    tkl_pwm_init(TUYA_PWM_NUM_1, &pwm_cfg);
    OPERATE_RET E_RET = tkl_pwm_start(TUYA_PWM_NUM_0);
/*
    // //for (int i = 0; i < http_response.body_length; i++) {
     for (int i = 0; i < 57280; i++) {
        // 将音频数据映射到PWM占空比
        //tkl_pwm_duty_set(TUYA_PWM_NUM_0, http_response.body[i]);
        //pwm_cfg.duty=http_response.body[i];
        pwm_cfg.duty= data0[i];
        // 输出PWM信号
        //analogWrite(pwmPin, pwm_duty);
        // 延时，控制音频播放速度
        //delay(); //delay(ms);
     }
*/
// for (int i = 0; i < 5; i++) {
// tkl_pwm_start(TUYA_PWM_NUM_0);
// pwm_cfg.duty= body_buf[i];
// delay(); 
// }
    //tkl_pwm_stop(TUYA_PWM_NUM_0);

//pwm_cfg.duty = 50;tkl_pwm_start(TUYA_PWM_NUM_0);delay(); delay(); 
// pwm_cfg.duty = 250;tkl_pwm_start(TUYA_PWM_NUM_0);
//delay(); delay(); 
//pwm_cfg.duty = 50;delay(); delay(); 
// tkl_pwm_duty_set(TUYA_PWM_NUM_0,50);
// delay(); 
// tkl_pwm_duty_set(TUYA_PWM_NUM_0,100);
// delay(); 
// tkl_pwm_duty_set(TUYA_PWM_NUM_0,150);
// delay(); 
// tkl_pwm_duty_set(TUYA_PWM_NUM_0,200);
// delay(); 
// tkl_pwm_duty_set(TUYA_PWM_NUM_0,250);
// delay(); 
// tkl_pwm_duty_set(TUYA_PWM_NUM_0,200);
// delay(); 
// tkl_pwm_duty_set(TUYA_PWM_NUM_0,150);
// delay(); 
// tkl_pwm_duty_set(TUYA_PWM_NUM_0,100);
// delay(); 
// tkl_pwm_duty_set(TUYA_PWM_NUM_0,50);
// delay(); 

    if (E_RET != OPRT_OK) {
        printf("PWM initialization failed! Error code: %d\n", E_RET);
    } else {
        printf("PWM initialization succeeded!\n");
    }


    for (;;) {
        /* Loop to receive packets, and handles client keepalive */
        tuya_iot_yield(&client);
        // tkl_pwm_stop(TUYA_PWM_NUM_0);
        // pwm_cfg.duty = 250;
        // tkl_pwm_init(TUYA_PWM_NUM_0, &pwm_cfg);tkl_pwm_start(TUYA_PWM_NUM_0);

        // tkl_pwm_duty_set(250);delay(); 
        // tkl_pwm_duty_set(150);delay(); 
        // tkl_pwm_duty_set(50);delay();
        // tkl_pwm_duty_set(150);delay(); 
        // tkl_pwm_duty_set(250);delay(); 

        // pwm_cfg.duty = 250;tkl_pwm_init(TUYA_PWM_NUM_0, &pwm_cfg);delay(); 
        // pwm_cfg.duty = 150;tkl_pwm_init(TUYA_PWM_NUM_0, &pwm_cfg);delay(); 
        // pwm_cfg.duty = 50;tkl_pwm_init(TUYA_PWM_NUM_0, &pwm_cfg);delay(); 
        // pwm_cfg.duty = 150;tkl_pwm_init(TUYA_PWM_NUM_0, &pwm_cfg);delay(); 
        // pwm_cfg.duty = 250;tkl_pwm_init(TUYA_PWM_NUM_0, &pwm_cfg);delay(); 

     for (int i = 0; i < 57280; i++) {
        // 将音频数据映射到PWM占空比
        //tkl_pwm_duty_set(TUYA_PWM_NUM_0, http_response.body[i]);
        //pwm_cfg.duty=http_response.body[i];
        pwm_cfg.duty= dataa[i];tkl_pwm_init(TUYA_PWM_NUM_0, &pwm_cfg);//delay(); 
        // 延时，控制音频播放速度
        //delay(); //delay(ms);
     }


    }
}

/**
 * @brief main
 *
 * @param argc
 * @param argv
 * @return void
 */
#if OPERATING_SYSTEM == SYSTEM_LINUX
void main(int argc, char *argv[])
{
    user_main();
}
#else

/* Tuya thread handle */
static THREAD_HANDLE ty_app_thread = NULL;

/**
 * @brief  task thread
 *
 * @param[in] arg:Parameters when creating a task
 * @return none
 */
static void tuya_app_thread(void *arg)
{
    user_main();

    tal_thread_delete(ty_app_thread);
    ty_app_thread = NULL;
}

void tuya_app_main(void)
{
    THREAD_CFG_T thrd_param = {4096, 4, "tuya_app_main"};
    tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
}
#endif
