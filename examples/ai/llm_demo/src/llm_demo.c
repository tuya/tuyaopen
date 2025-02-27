/**
 * @file LLM_DEMO.C
 * @brief This file contains the implementation of a lightweight logging
 * mechanism (LLM) for managing and processing conversation histories. It
 * includes functionalities for adding conversations to history, retrieving
 * conversation history, sending HTTP requests to AI models, and setting the
 * current AI model type.
 *
 * The LLM system is designed to interface with different AI models and manage
 * conversation histories efficiently. It provides a mechanism to log
 * conversations, manage them in a list, and use them in HTTP requests to AI
 * models for generating responses. The system supports different models by
 * configuring model-specific parameters and handling their responses
 * accordingly.
 *
 * Key functionalities include:
 * - Logging output through a user-defined callback.
 * - Adding conversations to a history list.
 * - Retrieving and managing conversation history.
 * - Sending HTTP requests to AI models based on the current configuration.
 * - Setting and resetting the current AI model type.
 *
 * This implementation demonstrates the use of various C standard library
 * functions, Tuya SDK functions, and cJSON for JSON parsing and formatting. It
 * showcases a practical application of linked lists, dynamic memory management,
 * and HTTP client interactions in embedded systems programming.
 *
 * @copyright Copyright (c) 2021-2024 Tuya Inc. All Rights Reserved.
 *
 */

#include "llm_demo.h"
#include "audio_asr.h"
#include "audio_tts.h"
#include "cJSON.h"
#include "http_client_interface.h"
#include "iotdns.h"
#include "llm_config.h"
#include "netmgr.h"
#include "tal_api.h"
#include "tkl_output.h"
#include "tuya_error_code.h"
#include "tuya_register_center.h"
#include <stdio.h>
#include <stdlib.h>
#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
#include "netconn_wifi.h"
#endif
#if defined(ENABLE_WIRED) && (ENABLE_WIRED == 1)
#include "netconn_wired.h"
#endif
#if defined(ENABLE_LIBLWIP) && (ENABLE_LIBLWIP == 1)
#include "lwip_init.h"
#endif

//#include "tkl_i2s.h"////

extern void tuya_app_cli_init(void);
#define MAX_SIZE_OF_DEBUG_BUF 4096
static char s_output_buf[MAX_SIZE_OF_DEBUG_BUF] = {0};

/**
 * @brief large language model configure
 *
 */
static LLM_t *sg_llm = NULL;
static LLM_config_t sg_config[] = {
    {LLM_ALIQWEN_TOKEN, LLM_HTTP_URL_ALIQWEN, LLM_HTTP_PATH_ALIQWEN, LLM_HTTP_HEADER_ALIQWEN},
    {LLM_KIMI_TOKEN, LLM_HTTP_URL_KIMI, LLM_HTTP_PATH_KIMI, LLM_HTTP_HEADER_KIMI},
    //{LLM_KIMI_TOKEN, LLM_HTTP_URL_KIMI, LLM_HTTP_PATH_KIMI, LLM_HTTP_HEADER_KIMI}
};

/**
 * @brief user defined log output api, in this demo, it will use uart0 as log-tx
 *
 * @param str log string
 * @return void
 */
void user_log_output_cb(const char *format, ...)
{
    if (format == NULL) {
        return;
    }

    va_list ap;
    va_start(ap, format);
    vsnprintf(s_output_buf, MAX_SIZE_OF_DEBUG_BUF, format, ap);
    va_end(ap);

    tal_uart_write(TUYA_UART_NUM_0, (const uint8_t *)s_output_buf, strlen(s_output_buf));
}

/**
 * @brief save the conversation as history
 *
 * @param conversation
 * @return int
 */
int __LLM_add_conversation(LLM_conversation_t *conversation)
{
    PR_DEBUG("save conversation %p, q: %p, a%p", conversation, conversation->q, conversation->a);
    sg_llm->his_cnt += conversation->q_size + conversation->a_size;
    tuya_list_add_tail(&conversation->node, &sg_llm->history);
    PR_DEBUG("token size is %d", sg_llm->his_cnt);

    return OPRT_OK;
}

/**
 * @brief get the conversatoion history
 *
 * @return char*
 */
char *__get_LLM_conversation()
{
    if (sg_llm->his_cnt >= DEFAULT_MAX_HISTORY_CNT) {
        struct tuya_list_head *p = NULL;
        struct tuya_list_head *n = NULL;
        LLM_conversation_t *first_entry = NULL;

        tuya_list_for_each_safe(p, n, &sg_llm->history)
        {
            first_entry = tuya_list_entry(p, LLM_conversation_t, node);
            if (first_entry) {
                tuya_list_del(&first_entry->node);
                PR_DEBUG("token is %d, drop \"%s\", length %d", sg_llm->his_cnt, first_entry->a,
                         first_entry->q_size + first_entry->a_size);
                sg_llm->his_cnt -= (first_entry->q_size + first_entry->a_size);
                if (first_entry->a)
                    tal_free(first_entry->a);
                if (first_entry->q)
                    tal_free(first_entry->q);
                tal_free(first_entry);
                first_entry = NULL;
            }

            if (sg_llm->his_cnt < DEFAULT_MAX_HISTORY_CNT) {
                break;
            }
        }
    }

    char *hist_buffer = NULL;
    int offset = 0;
    size_t hist_buffer_length = DEFAULT_MAX_HISTORY_CNT;
    if (sg_llm->his_cnt != 0) {
        hist_buffer = tal_malloc(hist_buffer_length);
        struct tuya_list_head *p = NULL;
        LLM_conversation_t *entry = NULL;
        tuya_list_for_each(p, &sg_llm->history)
        {
            entry = tuya_list_entry(p, LLM_conversation_t, node);
            if (offset != 0)
                offset += sprintf(hist_buffer + offset, ",");
            if (sg_llm->current == MODEL_ALI_QWEN) {
                offset += sprintf(hist_buffer + offset,
                                  "{\"role\":\"user\", \"content\":\"%s\"},{\"role\":\"assistant\", "
                                  "\"content\":\"%s\"}",
                                  entry->q, entry->a);
            } else if (sg_llm->current == MODEL_MOONSHOT_AI) {
                offset += sprintf(hist_buffer + offset,
                                  "{\"role\":\"user\", \"content\":\"%s\"},{\"role\":\"system\", "
                                  "\"content\":\"%s\"}",
                                  entry->q, entry->a);
            }
            if (offset >= hist_buffer_length) {
                PR_ERR("history buffer malloc fail");
                tal_free(hist_buffer);
                return NULL;
            }
        }
    }

    return hist_buffer;
}

/**
 * @brief send a llm http request
 *
 * @param q the question
 * @param a the answer
 * @return int OPRT_OK: success; other: fail
 */
int __LLM_http_request(char *q, char **a)
{
    PR_DEBUG("sg_llm->config[%d].model %s", sg_llm->current, sg_llm->config[sg_llm->current].model);
    // int offset = 0;
    int rt = OPRT_OK;
    cJSON *response = NULL;
    char *result = NULL;
    char *path_buf = NULL;
    char *body_buf = NULL;
    size_t path_buf_length = 128;

    /* HTTP Response */
    http_client_response_t http_response = {0};

    /* HTTP path */
    path_buf = tal_malloc(path_buf_length);
    TUYA_CHECK_NULL_GOTO(path_buf, err_exit);
    memset(path_buf, 0, path_buf_length);
    snprintf(path_buf, 128, "%s", sg_llm->config[sg_llm->current].path);

    /* make HTTP body */
    char *hist_buffer = __get_LLM_conversation();
    size_t body_buf_length = DEFAULT_BODY_BUFFER_LEN;
    body_buf = tal_malloc(body_buf_length);
    TUYA_CHECK_NULL_GOTO(body_buf, err_exit);
    memset(body_buf, 0, body_buf_length);
    if (sg_llm->current == MODEL_ALI_QWEN) {
        if (hist_buffer) {
            body_buf_length = sprintf(body_buf,
                                      "{\"model\":\"%s\", \"input\":{\"messages\":[%s,{\"role\":\"user\", "
                                      "\"content\":\"%s\"}]}}",
                                      sg_llm->config[sg_llm->current].model, hist_buffer, q);
        } else {
            body_buf_length = sprintf(body_buf, "{\"model\":\"%s\", \"input\":{\"prompt\":\"%s\"}}",
                                      sg_llm->config[sg_llm->current].model, q);
        }
    } else {
        if (hist_buffer) {
            body_buf_length = sprintf(body_buf,
                                      "{\"model\":\"%s\", \"messages\":[%s, {\"role\":\"user\", "
                                      "\"content\":\"%s\"}], \"temperature\": 0.3}",
                                      sg_llm->config[sg_llm->current].model, hist_buffer, q);
        } else {
            body_buf_length = sprintf(body_buf,
                                      "{\"model\":\"%s\", \"messages\":[{\"role\":\"user\", "
                                      "\"content\":\"%s\"}], \"temperature\": 0.3}",
                                      sg_llm->config[sg_llm->current].model, q);
        }
    }
    PR_DEBUG("https body: %s", body_buf);

    /* HTTP headers */
    http_client_header_t headers[] = {{.key = "Content-Type", .value = "application/json"},
                                      {.key = "Authorization", .value = sg_llm->config[sg_llm->current].token}};

    /* HTTPS cert */
    uint16_t cacert_len = 0;
    uint8_t *cacert = NULL;
    TUYA_CALL_ERR_GOTO(tuya_iotdns_query_domain_certs(sg_llm->config[sg_llm->current].host, &cacert, &cacert_len),
                       err_exit);

    /* HTTP Request send */
    PR_DEBUG("http request send!");
    http_client_status_t http_status = http_client_request(
        &(const http_client_request_t){.cacert = cacert,
                                       .cacert_len = cacert_len,
                                       .host = sg_llm->config[sg_llm->current].host,
                                       .port = 443,
                                       .method = "POST",
                                       .path = path_buf,
                                       .headers = headers,
                                       .headers_count = sizeof(headers) / sizeof(http_client_header_t),
                                       .body = (uint8_t *)body_buf,
                                       .body_length = strlen(body_buf),
                                       .timeout_ms = LLM_HTTP_REQUEST_TIMEOUT},
        &http_response);

    if (HTTP_CLIENT_SUCCESS != http_status) {
        PR_ERR("http_request_send error:%d", http_status);
        rt = OPRT_LINK_CORE_HTTP_CLIENT_SEND_ERROR;
        goto err_exit;
    }

    response = cJSON_Parse((char *)http_response.body);
    PR_DEBUG("response: %s", cJSON_PrintUnformatted(response));
    if (response) {
        if (sg_llm->current == MODEL_ALI_QWEN) {
            cJSON *output = cJSON_GetObjectItem(response, "output");
            if (output) {
                result = (char *)tal_malloc(strlen(cJSON_GetObjectItem(output, "text")->valuestring) + 1);
                strcpy(result, cJSON_GetObjectItem(output, "text")->valuestring);
                *a = result;
            }
        } else if (sg_llm->current == MODEL_MOONSHOT_AI) {
            cJSON *output = cJSON_GetObjectItem(response, "choices");
            if (output) {
                cJSON *first_item = cJSON_GetArrayItem(output, 0);
                cJSON *message = cJSON_GetObjectItem(first_item, "message");
                result = (char *)tal_malloc(strlen(cJSON_GetObjectItem(message, "content")->valuestring) + 1);
                strcpy(result, cJSON_GetObjectItem(message, "content")->valuestring);
                *a = result;
            }
        } else {
        }
        cJSON_Delete(response);
    }

err_exit:
    http_client_free(&http_response);

    if (path_buf)
        tal_free(path_buf);
    if (body_buf)
        tal_free(body_buf);
    if (cacert)
        tal_free(cacert);

    return rt;
}

/**
 * @brief set current ai model type
 *
 * @param type
 * @return int
 */
int LLM_set_model(LLM_type_e type)
{
    if (NULL == sg_llm) {
        PR_DEBUG("init llm");
        sg_llm = (LLM_t *)tal_malloc(sizeof(LLM_t));
        TUYA_CHECK_NULL_RETURN(sg_llm, OPRT_MALLOC_FAILED);

        sg_llm->his_cnt = 0;
        sg_llm->config = sg_config;
        INIT_LIST_HEAD(&sg_llm->history);
    } else {
        LLM_reset_model();
    }

    sg_llm->current = type;
    PR_DEBUG("set llm to %d, host %s path %s", type, sg_llm->config[type].host, sg_llm->config[type].path);

    // tuya_iot_get_third_cloud_ca(sg_llm->config[type].host);
    // http_recv_timeout_set(HTTP_TIMEOUT_MS_DEFAULT/1000);
    return OPRT_OK;
}

/**
 * @brief get current ai model type
 *
 * @param type
 * @return int
 */
int LLM_get_model(LLM_type_e *type)
{
    int rt = OPRT_OK;

    if (NULL == sg_llm) {
        //TUYA_CALL_ERR_RETURN(LLM_set_model(MODEL_ALI_QWEN));
        TUYA_CALL_ERR_RETURN(LLM_set_model(MODEL_MOONSHOT_AI));
    }
    ////sg_llm->current = MODEL_MOONSHOT_AI;
    *type = sg_llm->current;
    return OPRT_OK;
}

/**
 * @brief
 *
 * @param model_config
 * @param cur_context
 * @param his_context
 * @param result
 * @return int
 */
int LLM_conversation(char *q, char *a)
{
    int rt = OPRT_OK;
    char *response = NULL;

    __LLM_http_request(q, &response);
    if (response) {
        LLM_conversation_t *conversation = (LLM_conversation_t *)tal_malloc(sizeof(LLM_conversation_t));
        conversation->q = (char *)tal_malloc(strlen(q) + 1);
        strcpy(conversation->q, q);
        conversation->a = response;
        conversation->q_size = strlen(q) + 1;
        conversation->a_size = strlen(response) + 1;
        __LLM_add_conversation(conversation);

        strcpy(a, response);
    } else {
        strcpy(a, "please say that again!");
        LLM_reset_model();
    }

    return rt;
}

/**
 * @brief
 *
 * @return int
 */
int LLM_reset_model()
{
    struct tuya_list_head *p = NULL;
    struct tuya_list_head *n = NULL;
    LLM_conversation_t *first_entry = NULL;

    tuya_list_for_each_safe(p, n, &sg_llm->history)
    {
        first_entry = tuya_list_entry(p, LLM_conversation_t, node);
        if (first_entry) {
            tuya_list_del(&first_entry->node);
            PR_DEBUG("free conversation %p, q: %p, a%p", first_entry, first_entry->q, first_entry->a);
            if (first_entry->a)
                tal_free(first_entry->a);
            if (first_entry->q)
                tal_free(first_entry->q);
            tal_free(first_entry);
            first_entry = NULL;
        }
    }

    sg_llm->his_cnt = 0;
    PR_NOTICE("history cnt %d", sg_llm->his_cnt);
    return OPRT_OK;
}

void user_main()
{
    //! open iot development kit runtim init
    cJSON_InitHooks(&(cJSON_Hooks){.malloc_fn = tal_malloc, .free_fn = tal_free});
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 1024, (TAL_LOG_OUTPUT_CB)user_log_output_cb);
    tal_kv_init(&(tal_kv_cfg_t){
        .seed = "vmlkasdh93dlvlcy",
        .key = "dflfuap134ddlduq",
    });
    tal_sw_timer_init();
    tal_workq_init();
    tal_cli_init();
    tuya_tls_init();
    tuya_register_center_init();
    tuya_app_cli_init();

    // 初始化LWIP
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

    while (1) {
        tal_system_sleep(5000);
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
    //THREAD_CFG_T thrd_param = {16384 , 4, "tuya_app_main"};
    tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
}
#endif