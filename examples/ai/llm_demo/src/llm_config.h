/**
 * @file llm_config.h
 * @brief Configuration for various Large Language Models (LLM) and services.
 *
 * This file contains the configurations necessary for interacting with
 * different Large Language Models (LLM) and services such as aliqwe LLM, kimi
 * LLM, and Baidu's ASR and TTS services. It includes API keys, HTTP URLs,
 * paths, and headers required for making requests to these services.
 * Additionally, it defines constants for device identification, request
 * timeouts, and maximum response lengths to ensure efficient and effective
 * communication with these external services.
 *
 * The configurations are structured to facilitate easy integration and
 * modification for specific project requirements, allowing developers to
 * quickly adapt the codebase for various LLM and speech processing services.
 *
 * @note Ensure that you replace the placeholder tokens and IDs with your actual
 * credentials before deployment.
 *
 * @copyright Copyright (c) 2021-2024 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef LLM_CONFIG_H_
#define LLM_CONFIG_H_

/**
 * @brief aliqwe LLM configure
 *
 * 1，注册阿里云账号,进入控制台
 * 2，开通灵积模型服务: https://dashscope.console.aliyun.com/overview
 * 3，创建API-key: https://dashscope.console.aliyun.com/apiKey
 * 4，使用你的API-key替换LLM_ALIQWEN_TOKEN的内容
 *
 */
#define LLM_ALIQWEN_TOKEN       "sk-*****************************e51"
#define LLM_HTTP_URL_ALIQWEN    "dashscope.aliyuncs.com"
#define LLM_HTTP_PATH_ALIQWEN   "/api/v1/services/aigc/text-generation/generation"
#define LLM_HTTP_HEADER_ALIQWEN "qwen-turbo"

/**
 * @brief kimi LLM configure
 *
 * 1，登录月之暗面官方网站：https://www.moonshot.cn/， 选择moonshot开放平台
 * 2，注册账号，进入用户中心：https://platform.moonshot.cn/console/info
 * 3，创建API-key：https://platform.moonshot.cn/console/api-keys
 * 4，使用你的API-key替换LLM_KIMI_TOKEN的内容
 *
 */
#define LLM_KIMI_TOKEN       "sk-p3xulVNnZ3PZw2TjMO5DPlggsOYQyw6Q021bK3BLzpWuyzxr"//"Y2xz***************************************************************UQ2Q="
#define LLM_HTTP_URL_KIMI    "api.moonshot.cn"
#define LLM_HTTP_PATH_KIMI   "/v1/chat/completions"
#define LLM_HTTP_HEADER_KIMI "moonshot-v1-8k"

/**
 * @brief baidu asr configuration
 *
 * 1，登录百度智能云：，https://cloud.baidu.com/?from=console
 * 2，注册账号，进入控制台
 * 3，搜索语音产品，创建应用：https://console.bce.baidu.com/ai/#/ai/speech/app/list
 * 4，使用你的API-key替换client-id的内容
 * 5，使用你的Securet-key替换client-securet
 * 6，使用设备唯一ID作为CUID
 *
 */
#define ASR_BAIDU_CLIENTID       "SGZCe23peJrAFHFK5AcULBBD"//"YA7******************Siv"
#define ASR_BAIDU_CLIENT_SECURET "5gEXVENSVmDAZL4CFW1lZ3gIGqMzEMye"//"uIi**************************HBE"

/**
 * @brief baidu asr
 *
 */
#define BAIDU_TOKEN_URL      "aip.baidubce.com"
#define ASR_BAIDU_TOKEN_PATH "/oauth/2.0/token"
#define ASR_BAIDU_URL        "vop.baidu.com"
#define ASR_BAIDU_PATH       "/server_api"

/**
 * @brief baiud tts
 *
 */
#define TTS_BAIDU_LONG_URL      "aip.baidubce.com"
#define TTS_BAIDU_LONG_REQ_PATH "/rpc/2.0/tts/v1/create"
#define TTS_BAIDU_LONG_QER_PATH "/rpc/2.0/tts/v1/query"
#define TTS_BAIDU_SHORT_URL     "tsn.baidu.com"
#define TTS_BAIDU_SHORT_PATH    "/text2audio"
#define TTS_MAX_RESPONSE_LENGTH 32 * 1024
/**
 * @brief baidu cuid, a specific device ID, used to identify the device, you can
 * define it yourself
 *
 */
#define BAIDU_CUID "112233aabbccddee112233"

// LLM max context length
#define LLM_MAX_CONTEXT_LENGTH  8 * 1024
#define LLM_MAX_RESPONSE_LENGTH 8 * 1024

// LLM http request config
#define LLM_HTTP_REQUEST_TIMEOUT 30 * 1000

#endif