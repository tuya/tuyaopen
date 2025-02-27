/**
 * @file audio_tts.c
 * @brief Text-to-Speech (TTS) interface for Baidu TTS service.
 *
 * This file provides the implementation of functions to interact with Baidu's
 * TTS service, including obtaining access tokens, converting text to speech,
 * and handling the TTS response. It utilizes HTTP requests for communication
 * with Baidu's TTS service and cJSON for parsing the responses. The
 * implementation demonstrates how to use the Tuya IoT SDK's HTTP client
 * interface and memory management functions for TTS feature integration.
 *
 * @copyright Copyright (c) 2021-2024 Tuya Inc. All Rights Reserved.
 *
 */

#include "audio_tts.h"
#include "audio_asr.h"
#include "cJSON.h"
#include "http_client_interface.h"
#include "iotdns.h"
#include "llm_config.h"
#include "mix_method.h"
#include "tal_log.h"
#include "tal_memory.h"
#include "tal_system.h"
#include "tuya_error_code.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "tkl_pwm.h"////////
#include "tts_data.h"////////

extern unsigned char data0[];

/**
 * @brief get token
 *
 * @param[in/out] token
 * @return int
 */
int __tts_baidu_get_token(char *token)
{
    int rt = OPRT_OK;
    cJSON *response = NULL;
    uint16_t cacert_len = 0;
    uint8_t *cacert = NULL;
    char *path_buf = NULL;
    size_t path_buf_length = 256;

    /* HTTP Response */
    http_client_response_t http_response = {0};

    /* HTTP path */
    path_buf = tal_malloc(path_buf_length);
    TUYA_CHECK_NULL_GOTO(path_buf, err_exit);
    memset(path_buf, 0, path_buf_length);
    snprintf(path_buf, 256, "%s?client_id=%s&client_secret=%s&grant_type=client_credentials", ASR_BAIDU_TOKEN_PATH,
             ASR_BAIDU_CLIENTID, ASR_BAIDU_CLIENT_SECURET);

    /* make HTTP body */
    char body_buf[8] = {0};
    snprintf(body_buf, 8, "{}");

    /* HTTP headers */
    http_client_header_t headers[] = {{.key = "Content-Type", .value = "application/json"}};

    /* HTTPS cert */
    TUYA_CALL_ERR_GOTO(tuya_iotdns_query_domain_certs(BAIDU_TOKEN_URL, &cacert, &cacert_len), err_exit);

    /* HTTP Request send */
    PR_DEBUG("http request send!");
    http_client_status_t http_status = http_client_request(
        &(const http_client_request_t){.cacert = cacert,
                                       .cacert_len = cacert_len,
                                       .host = BAIDU_TOKEN_URL,
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
    if (response) {
        PR_DEBUG("response: %s", cJSON_PrintUnformatted(response));
        strcpy(token, cJSON_GetObjectItem(response, "access_token")->valuestring);
        cJSON_Delete(response);
    }

err_exit:
    http_client_free(&http_response);

    if (path_buf)
        tal_free(path_buf);
    if (cacert)
        tal_free(cacert);
    return rt;
}

/**
 * @brief
 *
 * @param format the format of audio file
 * @param text the text to be translated
 * @param voice the voice of tts:0-xiaomei,1-xiaoyu,3-xiaoyao,4-yaya
 * @param lang default is "zh"
 * @param speed the speed of tts:0-15,default is 5
 * @param pitch the pitch of tts:0-15,default is 5
 * @param volume the volume of tts:0-15,default is 5
 * @return int OPRT_OK:success;other:fail
 */
int tts_request_baidu(TTS_format_e format, char *text, int voice, char *lang, int speed, int pitch, int volume)
{
    int rt = OPRT_OK;
    uint16_t cacert_len = 0;
    uint8_t *cacert = NULL;
    char *path_buf = NULL;
    char *body_buf = NULL;
    size_t path_buf_length = 128;

    /* HTTP Response */
    http_client_response_t http_response = {0};

    /* HTTP path */
    path_buf = tal_malloc(path_buf_length);
    TUYA_CHECK_NULL_GOTO(path_buf, err_exit);
    memset(path_buf, 0, path_buf_length);
    snprintf(path_buf, 128, "%s", TTS_BAIDU_SHORT_PATH);

    /* get token */
    char token[128] = {};
    TUYA_CALL_ERR_GOTO(__tts_baidu_get_token(token), err_exit);

    /* make HTTP body */
    size_t body_buf_length = strlen(text) + 512;
    body_buf = tal_malloc(body_buf_length);
    TUYA_CHECK_NULL_GOTO(body_buf, err_exit);
    memset(body_buf, 0, body_buf_length);
    // body_buf_length = sprintf(body_buf, "{\"text\":\"%s\", \"format\":\"%s\",
    // \"voice\":%d, \"speed\":%d, \"pitch\":%d, \"volume\":%d, \"lang\":\"%s\"}",
    // text, format, voice, speed, pitch, volume, lang);
    body_buf_length = sprintf(body_buf, "tex=%s&tok=%s&aue=%d&per=%d&spd=%d&pit=%d&vol=%d&lan=%s&ctp=1&cuid=%s", text,
                              token, format, voice, speed, pitch, volume, lang, BAIDU_CUID);
    PR_DEBUG("https body: %s", body_buf);

    /* HTTP headers */
    http_client_header_t headers[] = {
        {.key = "Content-Type", .value = "application/x-www-form-urlencoded"},
        {.key = "Accept", .value = "*/*"},
    };

    /* HTTPS cert */
    TUYA_CALL_ERR_GOTO(tuya_iotdns_query_domain_certs(TTS_BAIDU_SHORT_URL, &cacert, &cacert_len), err_exit);

    /* HTTP Request send */
    PR_DEBUG("http request send!");
    http_client_status_t http_status = http_client_request(
        &(const http_client_request_t){.cacert = cacert,
                                       .cacert_len = cacert_len,
                                       .host = TTS_BAIDU_SHORT_URL,
                                       .port = 443,
                                       .method = "POST",
                                       .path = path_buf,
                                       .headers = headers,
                                       .headers_count = sizeof(headers) / sizeof(http_client_header_t),
                                       .body = (uint8_t *)body_buf,
                                       .body_length = body_buf_length,
                                       .timeout_ms = LLM_HTTP_REQUEST_TIMEOUT},
        &http_response);

    if (HTTP_CLIENT_SUCCESS != http_status) {
        PR_ERR("http_request_send error:%d", http_status);
        rt = OPRT_LINK_CORE_HTTP_CLIENT_SEND_ERROR;
        goto err_exit;
    }


    TUYA_PWM_BASE_CFG_T pwm_cfg;
    pwm_cfg.polarity = TUYA_PWM_POSITIVE; 
    pwm_cfg.count_mode = TUYA_PWM_CNT_UP; 
    pwm_cfg.duty = 130;// 0-270
    pwm_cfg.frequency = 16000;
    tkl_pwm_init(TUYA_PWM_NUM_0, &pwm_cfg);
    tkl_pwm_start(TUYA_PWM_NUM_0);

    // for (int i = 0; i < http_response.body_length; i++) {
    //     pwm_cfg.duty= data0[i];tkl_pwm_init(TUYA_PWM_NUM_0, &pwm_cfg);
    // }

    ////add    
   // 保存语音文件
    FILE *fp = fopen("/home/ubuntu/Desktop/tuyaopen/examples/ai/llm_demo/file_audio.pcm", "wb");
    if (fp) {
        // if (http_response.body == NULL || http_response.body_length == 0) {
        //     PR_DEBUG("数据为空或无效");
        //     return;
        // }else{fwrite(http_response.body, 1, http_response.body_length, fp);

        PR_DEBUG("二进制数据（十六进制）:");
        //PR_DEBUG("%02x ", http_response.body[i]); // 输出形如 "1a ff 00 3b"
        for (size_t i = 0; i < http_response.body_length; i++) {    
            pwm_cfg.duty= http_response.body[i];tkl_pwm_init(TUYA_PWM_NUM_0, &pwm_cfg);       
        }
        fwrite(http_response.body, 1, http_response.body_length, fp);

        PR_DEBUG("http_response.body_length:%zu",http_response.body_length);  
        fclose(fp);PR_DEBUG("Audio file saved successfully.");        
    } else PR_DEBUG("Failed to open file for writing.");
        //PR_ERR("Failed to open file for writing.");
        // rt = OPRT_FILE_OPEN_FAILED;
        //goto err_exit;
    

/*
    //pwm
    TUYA_PWM_BASE_CFG_T pwm_cfg;
    // 设置PWM极性，假设使用正极性
    pwm_cfg.polarity = TUYA_PWM_POSITIVE; 
    // 设置计数模式，假设使用向上计数模式
    pwm_cfg.count_mode = TUYA_PWM_CNT_UP; 
    // 设置占空比，假设为 ?0%
    pwm_cfg.duty = 10; 
    // 设置周期
    pwm_cfg.cycle = 100; 
    // 设置频率，假设为10000Hz
    pwm_cfg.frequency = 10000; 
    tkl_pwm_init(TUYA_PWM_NUM_0, &pwm_cfg);
    OPERATE_RET E_RET = tkl_pwm_start(TUYA_PWM_NUM_0);

    for (int i = 0; i < http_response.body_length; i++) {
        // 将音频数据映射到PWM占空比
        tkl_pwm_duty_set(TUYA_PWM_NUM_0, http_response.body[i]);
        // 输出PWM信号
        //analogWrite(pwmPin, pwm_duty);
        // 延时，控制音频播放速度
        delay(100); //delay(ms);
    }

    tkl_pwm_stop(TUYA_PWM_NUM_0);

    if (E_RET != OPRT_OK) {
        printf("PWM initialization failed! Error code: %d\n", E_RET);
    } else {
        printf("PWM initialization succeeded!\n");
    }
*/

    // response.body
    //PR_DEBUG("response: %x", http_response.body);
    PR_DEBUG("response: %.*ss",(int32_t)http_response.body_length, http_response.body);
    // 保存语音文件
    // FILE *fp = fopen("/home/ubuntu/Desktop/tuyaopen/examples/ai/llm_demo/audio_file.bin", "wb");
    // if (fp) {
    //     fwrite(http_response.body, 1, http_response.body_length, fp);
    //     fclose(fp);
    //     PR_DEBUG("Audio file saved successfully.");
    // } else {
    //     PR_ERR("Failed to open file for writing.");
    //     //rt = OPRT_FILE_OPEN_FAILED;
    //     //goto err_exit;
    // }
/*
FILE *fp = fopen("/home/ubuntu/Desktop/tuyaopen/examples/ai/llm_demo/audio_file.bin", "wb");
    // if (fp) {
    if (fp == NULL) {
        PR_DEBUG("Failed to open file???");
        //return 1;
    }

    // 检查 body 是否为空
    if (http_response.body != NULL && http_response.body_length > 0) {
        // 将 body 内容写入文件
        size_t written = fwrite(http_response.body, 1, http_response.body_length, fp);
        if (written != http_response.body_length) {
            PR_DEBUG("Failed to write the entire response body to the file");
        } else {
            PR_DEBUG("Response body written to file successfully.\n");
        }

        // 关闭文件
        fclose(fp);

    } else {
        PR_DEBUG("Response body is empty.\n");
    }
*/


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
