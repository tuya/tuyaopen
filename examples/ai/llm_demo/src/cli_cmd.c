/**
 * @file cli_cmd.c
 * @brief Command Line Interface (CLI) commands for Tuya IoT applications.
 *
 * This file contains the implementation of CLI commands for interacting with
 * various services such as WiFi connection management, chatting with a large
 * language model, switching between different language models, testing
 * Automatic Speech Recognition (ASR), and Text-to-Speech (TTS) functionalities.
 * It demonstrates the integration of Tuya's IoT SDK functionalities, including
 * network management, language model interaction, and audio processing
 * capabilities, providing a comprehensive example of how to build IoT
 * applications with Tuya's technology stack.
 *
 * @copyright Copyright (c) 2021-2024 Tuya Inc. All Rights Reserved.
 *
 */

#include "audio_asr.h"
#include "audio_tts.h"
#include "llm_demo.h"
#include "netmgr.h"
#include "tal_api.h"
#include "tal_cli.h"
#include "tal_workq_service.h"
#include <stdlib.h>
#include <sys/stat.h>
#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
#include "netconn_wifi.h"
#endif
/**
 * @brief send chat information to big language model
 *
 * @param argc
 * @param argv
 */
void connect_cmd(int argc, char *argv[])
{
    if (argc == 3) {
#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
        netconn_wifi_info_t wifi_info = {0};
        strcpy(wifi_info.ssid, argv[1]);
        strcpy(wifi_info.pswd, argv[2]);
        netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_SSID_PSWD, &wifi_info);
#endif
    } else {
        PR_NOTICE("usage: connect <ssid> <password>");
    }

    return;
}

void __chat(void *data)
{
    char *context = (char *)data;
    char *response = tal_malloc(8192);
    memset(response, 0, 8192);
    int rt = LLM_conversation(context, response);
    if (OPRT_OK == rt) {
        PR_NOTICE("******************************************************");
        PR_NOTICE("q: %s", context);
        PR_NOTICE("------------------------------------------------------");
        PR_NOTICE("a: %s", response);
        PR_NOTICE("******************************************************");
    }

    tal_free(response);

    return;
}
/**
 * @brief send chat information to big language model
 *
 * @param argc
 * @param argv
 */
void chat_cmd(int argc, char *argv[])
{
    netmgr_status_e status = NETMGR_LINK_DOWN;
    netmgr_conn_get(NETCONN_AUTO, NETCONN_CMD_STATUS, &status);
    if (status != NETMGR_LINK_UP) {
        PR_ERR("please connect to the router first!");
        return;
    }

    int index = 1;
    int offset = 0;
    char *context = tal_malloc(128);
    memset(context, 0, 128);
    for (index = 1; index < argc; index++) {
        if (offset + strlen(argv[index]) >= 128) {
            PR_ERR("context is too long!");
            return;
        }
        offset += sprintf(context + offset, "%s ", argv[index]);
    }

    tal_workq_schedule(WORKQ_SYSTEM, __chat, context);
    return;
}

/**
 * @brief switch the llm
 *
 * @param[in/out] argc
 * @param[in/out] argv
 */
void switch_cmd(int argc, char *argv[])
{
    if (argc == 2) {
        if (0 == strcmp(argv[1], "ali-qwen")) {
            LLM_set_model(MODEL_ALI_QWEN);
            return;
        } else if (0 == strcmp(argv[1], "moonshot")) {
            LLM_set_model(MODEL_MOONSHOT_AI);
            return;
        }
    }

    PR_NOTICE("usage: switch ali-qwen/moonshot");
    return;
}

void __asr(void *data)
{
    int fsize = 0;
    uint8_t *buffer = NULL;
    char output_text[128];
    int output_len = 128;
    char *file_name = "../../../examples/llm_demo/src/localrec1.wav";
    FILE *fd = fopen(file_name, "rb");
    if (fd) {
        struct stat file_stat;
        if (stat(file_name, &file_stat) == -1) {
            perror("Error getting file status");
            return;
        }
        fsize = file_stat.st_size;
        PR_DEBUG("input %p, size is %d", fd, fsize);
        buffer = tal_malloc(fsize + 10);
        fsize = fread(buffer, sizeof(uint8_t), fsize, fd);
        fclose(fd);

        asr_request_baidu(ASR_FORMAT_WAV, 8000, 1, buffer, fsize, output_text, &output_len);
        PR_DEBUG("output is %s, size is %d", output_text, output_len);
    } else {
        PR_DEBUG("audio file not found!");
    }
}
/**
 * @brief test asr
 *
 * @param[in/out] argc
 * @param[in/out] argv
 * @return void
 */
void asr_cmd(int argc, char *argv[])
{
    netmgr_status_e status = NETMGR_LINK_DOWN;
    netmgr_conn_get(NETCONN_AUTO, NETCONN_CMD_STATUS, &status);
    if (status != NETMGR_LINK_UP) {
        PR_ERR("please connect to the router first!");
        return;
    }

    tal_workq_schedule(WORKQ_SYSTEM, __asr, NULL);
    return;
}

void __tts(void *data)
{
    char *context = (char *)data;
    //tts_request_baidu(TTS_FORMAT_MP3, context, 0, "zh", 5, 5, 5);
    tts_request_baidu(TTS_FORMAT_PCM16K, context, 0, "zh", 5, 5, 5);
    tal_free(context);
    return;
}
/**
 * @brief test tts
 *
 * @param argc
 * @param argv
 */
void tts_cmd(int argc, char *argv[])
{
    netmgr_status_e status = NETMGR_LINK_DOWN;
    netmgr_conn_get(NETCONN_AUTO, NETCONN_CMD_STATUS, &status);
    if (status != NETMGR_LINK_UP) {
        PR_ERR("please connect to the router first!");
        return;
    }

    int index = 1;
    int offset = 0;
    char *context = tal_malloc(128);
    memset(context, 0, 128);
    if (argc < 2) {
        PR_ERR("usage: tts <text>");
    } else {
        for (index = 1; index < argc; index++) {
            if (offset + strlen(argv[index]) >= 128) {
                PR_ERR("context is too long!");
                return;
            }
            offset += sprintf(context + offset, "%s ", argv[index]);
        }

        tal_workq_schedule(WORKQ_SYSTEM, __tts, context);
    }

    return;
}

/**
 * @brief
 *
 */
void tuya_app_cli_init(void)
{
    LLM_set_model(MODEL_ALI_QWEN);

    static cli_cmd_t cmd[5] = {
        {.name = "connect", .help = "Connect to the router.", .func = connect_cmd},
        {.name = "chat", .help = "Chat with large language model.", .func = chat_cmd},
        {.name = "switch", .help = "Switch large language model.", .func = switch_cmd},
        {.name = "asr", .help = "Test ASR.", .func = asr_cmd},
        {.name = "tts", .help = "Test TTS.", .func = tts_cmd},
    };
    tal_cli_cmd_register(cmd, 5);
    PR_NOTICE("******************************************************************"
              "************************************************");
    PR_NOTICE("now you can chat with large language model, default is ali-qwen, "
              "you can change the model according cli command!");
    PR_NOTICE("");
    PR_NOTICE("TAB for help!");
    PR_NOTICE("******************************************************************"
              "************************************************");
    return;
}