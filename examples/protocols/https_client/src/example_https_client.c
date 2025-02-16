/**
 * @file example_https_client.c
 * @brief Demonstrates HTTPS client usage in Tuya SDK applications.
 *
 * This file provides an example of how to use the HTTPS client interface provided by the Tuya SDK to send HTTPS requests
 * and handle responses. It includes initializing the SDK, setting up network connections (both WiFi and wired,
 * depending on the configuration), sending a GET request to a specified URL, and handling the response. The example
 * also demonstrates how to handle network link status changes and perform cleanups.
 *
 * Key operations demonstrated in this file:
 * - Initialization of the Tuya SDK and network manager.
 * - Sending an HTTPS GET request and receiving a response.
 * - Handling network link status changes.
 * - Cleanup and resource management.
 *
 * This example is intended for developers looking to integrate HTTPS communication into their Tuya SDK-based IoT
 * applications, providing a foundation for building applications that interact with web services.
 *
 * @copyright Copyright (c) 2021-2024 Tuya Inc. All Rights Reserved.
 *
 */

 #include "tuya_cloud_types.h"
 #include "http_client_interface.h"
 
 #include "tal_api.h"
 #include "tkl_output.h"
 #include "netmgr.h"
 #if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
 #include "netconn_wifi.h"
 #include "tal_wifi.h"
 #endif
 #if defined(ENABLE_WIRED) && (ENABLE_WIRED == 1)
 #include "netconn_wired.h"
 #endif
 
 /***********************************************************
 *********************** macro define ***********************
 ***********************************************************/
 #define URL  "httpbin.org"
 #define PATH "/get"
 
 #ifdef ENABLE_WIFI
 #define DEFAULT_WIFI_SSID "your-ssid-****"
 #define DEFAULT_WIFI_PSWD "your-pswd-****"
 #endif
 /***********************************************************
 ********************** typedef define **********************
 ***********************************************************/
 #define HTTP_REQUEST_TIMEOUT 10 * 1000
 
 /***********************************************************
 ********************** variable define *********************
 ***********************************************************/
enum {
    WIFI_STATUS_IDLE = 0,
    WIFI_STATUS_DISCONNECTED,
    WIFI_STATUS_CONNECT_FAIL,
    WIFI_STATUS_CONNECTED,
};

 static int wifi_connect_status = WIFI_STATUS_IDLE;

 /***********************************************************
 ********************** function define *********************
 ***********************************************************/
 
 /**
  * @brief  http_get_example
  *
  * @param[in] param: None
  * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
  */
 OPERATE_RET https_get_example(void)
 {
    int rt = OPRT_OK;
    uint16_t cacert_len = 0;
    uint8_t *cacert = NULL;
        
    /* HTTP Response */
    http_client_response_t http_response = {0};

    /* HTTP headers */
    http_client_header_t headers[] = {{.key = "Content-Type", .value = "application/json"}};

    /* HTTPS cert */
    TUYA_CALL_ERR_GOTO(tuya_iotdns_query_domain_certs(URL, &cacert, &cacert_len), err_exit);

    /* HTTP Request send */
    PR_DEBUG("http request send!");
    http_client_status_t http_status = http_client_request(
        &(const http_client_request_t){.cacert = cacert,
                                       .cacert_len = cacert_len,
                                       .host = URL,
                                       .port = 443,
                                       .method = "GET",
                                       .path = PATH,
                                       .headers = headers,
                                       .headers_count = sizeof(headers) / sizeof(http_client_header_t),
                                       .body = "",
                                       .body_length = 0,
                                       .timeout_ms = HTTP_REQUEST_TIMEOUT},
        &http_response);


    if (HTTP_CLIENT_SUCCESS != http_status) {
        PR_ERR("http_request_send error:%d", http_status);
        rt = OPRT_LINK_CORE_HTTP_CLIENT_SEND_ERROR;
        goto err_exit;
    }

    PR_DEBUG("http_get_example body: \n%s", (char *)http_response.body);
err_exit:
    http_client_free(&http_response);

    return rt;
 }
 
 #if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
 /**
  * @brief wifi Related event callback function
  *
  * @param[in] event:event
  * @param[in] arg:parameter
  * @return none
  */
 static void wifi_event_callback(WF_EVENT_E event, void *arg)
 {
     OPERATE_RET op_ret = OPRT_OK;
     NW_IP_S sta_info;
     memset(&sta_info, 0, sizeof(NW_IP_S));
 
     PR_DEBUG("-------------event callback-------------");
     switch (event) {
        case WFE_CONNECTED: {
            PR_DEBUG("connection succeeded!");
    
            /* output ip information */
            op_ret = tal_wifi_get_ip(WF_STATION, &sta_info);
            if (OPRT_OK != op_ret) {
                PR_ERR("get station ip error");
                return;
            }
            PR_NOTICE("gw: %s", sta_info.gw);
            PR_NOTICE("ip: %s", sta_info.ip);
            PR_NOTICE("mask: %s", sta_info.mask);
    
            wifi_connect_status = WIFI_STATUS_CONNECTED;
            break;
        }
    
        case WFE_CONNECT_FAILED: {
            PR_DEBUG("connection fail!");
            wifi_connect_status = WIFI_STATUS_CONNECT_FAIL;
            break;
        }
    
        case WFE_DISCONNECTED: {
            PR_DEBUG("disconnect!");
            wifi_connect_status = WIFI_STATUS_DISCONNECTED;
            break;
        }
     }
 }
 #endif
 
 /**
  * @brief user_main
  *
  * @return void
  */
 void user_main()
 {
     OPERATE_RET rt = OPRT_OK;
 
     /* basic init */
     tal_log_init(TAL_LOG_LEVEL_DEBUG, 1024, (TAL_LOG_OUTPUT_CB)tkl_log_output);
     tal_kv_init(&(tal_kv_cfg_t){
         .seed = "vmlkasdh93dlvlcy",
         .key = "dflfuap134ddlduq",
     });
     tal_sw_timer_init();
     tal_workq_init();
     tuya_tls_init();
     tuya_register_center_init();
 
     // 初始化LWIP
 #if defined(ENABLE_LIBLWIP) && (ENABLE_LIBLWIP == 1)
     TUYA_LwIP_Init();
 #endif
 
 #if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
     /*WiFi init*/
     TUYA_CALL_ERR_GOTO(tal_wifi_init(wifi_event_callback), __EXIT);
 
     /*Set WiFi mode to station*/
     TUYA_CALL_ERR_GOTO(tal_wifi_set_work_mode(WWM_STATION), __EXIT);
 
     TUYA_CALL_ERR_LOG(tal_wifi_station_connect((int8_t *)DEFAULT_WIFI_SSID, (int8_t *)DEFAULT_WIFI_PSWD));
 #endif
 
 #if defined(ENABLE_WIRED) && (ENABLE_WIRED == 1)
     https_get_example();
#elif defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)

     while (1) {
        if (wifi_connect_status == WIFI_STATUS_CONNECTED) {
            https_get_example();
            break;
        } else
            tkl_system_sleep(500);
     }
 #endif
 
 __EXIT:
     return;
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
     while (1) {
         tal_system_sleep(500);
     }
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
     THREAD_CFG_T thrd_param = {1024 * 6, 4, "tuya_app_main"};
     tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
 }
 #endif