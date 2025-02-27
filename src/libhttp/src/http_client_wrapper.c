#include "tuya_error_code.h"
#include "http_client_interface.h"
#include "transport_interface.h"
#include "core_http_client.h"
#include "tuya_tls.h"
#include "tal_log.h"

int i=0;
#define log_debug PR_DEBUG
#define log_error PR_ERR

#define HEADER_BUFFER_LENGTH (255)
#define DEFAULT_HTTP_PORT    (80)
#define DEFAULT_HTTPS_PORT   (443)
static http_client_status_t core_http_request_send(const TransportInterface_t *pTransportInterface,
                                                   const HTTPRequestInfo_t *requestInfo, http_client_header_t *headers,
                                                   uint8_t headers_count, const uint8_t *pRequestBodyBuf,
                                                   size_t reqBodyBufLen, HTTPResponse_t *response)
{
    /* Represents header data that will be sent in an HTTP request. */
    HTTPRequestHeaders_t requestHeaders;

    /* Return value of all methods from the HTTP Client library API. */
    HTTPStatus_t httpStatus = HTTPSuccess;

    if (NULL == requestInfo || NULL == response) {
        return HTTP_CLIENT_SERIALIZE_FAULT;
    }
    /* Initialize all HTTP Client library API structs to 0. */
    (void)memset(&requestHeaders, 0, sizeof(requestHeaders));
    /* Set the buffer used for storing request headers. */
    requestHeaders.bufferLen = HEADER_BUFFER_LENGTH + headers_count * 32;
    requestHeaders.pBuffer = tal_malloc(requestHeaders.bufferLen);
    if (requestHeaders.pBuffer == NULL) {
        return HTTP_CLIENT_MALLOC_FAULT;
    }

    httpStatus = HTTPClient_InitializeRequestHeaders(&requestHeaders, requestInfo);
    int i;
    for (i = 0; i < headers_count; i++) {
        log_debug("HTTP header add key:value\r\nkey=%s : value=%s", headers[i].key, headers[i].value);
        httpStatus |= HTTPClient_AddHeader(&requestHeaders, headers[i].key, strlen(headers[i].key), headers[i].value,
                                           strlen(headers[i].value));
    }

    if (httpStatus != HTTPSuccess) {
        log_error("HTTP header error:%d", httpStatus);
        tal_free(requestHeaders.pBuffer);
        return HTTP_CLIENT_SERIALIZE_FAULT;
    }

    log_debug("Sending HTTP %.*s request to %.*s%.*s", (int32_t)requestInfo->methodLen, requestInfo->pMethod,
              (int32_t)requestInfo->hostLen, requestInfo->pHost, (int32_t)requestInfo->pathLen, requestInfo->pPath);

    /* Send the request and receive the response. */
    httpStatus = HTTPClient_Request(pTransportInterface, &requestHeaders, (uint8_t *)pRequestBodyBuf, reqBodyBufLen,
                                    response, 0);

    /* Release headers buffer */
    tal_free(requestHeaders.pBuffer);

    if (httpStatus != HTTPSuccess) {
        log_error("Failed to send HTTP %.*s request to %.*s%.*s: Error=%s.", (int32_t)requestInfo->methodLen,
                  requestInfo->pMethod, (int32_t)requestInfo->hostLen, requestInfo->pHost,
                  (int32_t)requestInfo->pathLen, requestInfo->pPath, HTTPClient_strerror(httpStatus));
        return HTTP_CLIENT_SEND_FAULT;
    }

    log_debug("Response Headers:\r\n%.*s\r\n"
              "Response Status:\r\n%u\r\n"
              "Response Body:\r\n%.*s\r\n",
              (int32_t)response->headersLen, response->pHeaders, response->statusCode, (int32_t)response->bodyLen,
              response->pBody);


    // FILE *fp = fopen("/home/ubuntu/Desktop/tuyaopen/examples/ai/llm_demo/audio_file.mp3", "wb");
    // if (fp) {
    //     i++;if (i==3)
    //     {
    //     fwrite(response->pBody, 1, (int32_t)response->bodyLen, fp);
    //     fclose(fp);i=0;
    //     PR_DEBUG("Audio file saved successfully.");
    //     }else{
    //         fclose(fp);
    //     }  
    // } else {
    //     PR_ERR("Failed to open file for writing.");
    //     // rt = OPRT_FILE_OPEN_FAILED;
    //     // goto err_exit;
    // }

    return HTTP_CLIENT_SUCCESS;
}

http_client_status_t http_client_request(const http_client_request_t *request, http_client_response_t *response)
{
    http_client_status_t rt = HTTP_CLIENT_SUCCESS;
    int ret = OPRT_OK;

    /* TLS pre init */
    NetworkContext_t network;
    TUYA_TRANSPORT_TYPE_E transport_type = (request->cacert == NULL) ? TRANSPORT_TYPE_TCP : TRANSPORT_TYPE_TLS;
    network = tuya_transporter_create(transport_type, NULL);
    if (NULL == network) {
        return HTTP_CLIENT_MALLOC_FAULT;
    }

    if (transport_type == TRANSPORT_TYPE_TLS) {
        tuya_tls_config_t tls_config = {
            .ca_cert = (char *)request->cacert,
            .ca_cert_size = request->cacert_len,
            .hostname = (char *)request->host,
            .port = (request->port == 0) ? DEFAULT_HTTPS_PORT : request->port,
            .timeout = request->timeout_ms,
            .mode = TUYA_TLS_SERVER_CERT_MODE,
            .verify = true,
        };

        ret = tuya_transporter_ctrl(network, TUYA_TRANSPORTER_SET_TLS_CONFIG, &tls_config);
        if (OPRT_OK != ret) {
            log_error("network_tls_init fail:%d", ret);
            return ret;
        }

        ret = tuya_transporter_connect(network, tls_config.hostname, tls_config.port, tls_config.timeout);
        if (OPRT_OK != ret) {
            tuya_transporter_close(network);
            tuya_transporter_destroy(network);
            return HTTP_CLIENT_SEND_FAULT;
        }

        log_debug("tls connencted!");
    } else {
        ret = tuya_transporter_connect(network, request->host, (request->port == 0) ? DEFAULT_HTTP_PORT : request->port,
                                       request->timeout_ms);
        if (OPRT_OK != ret) {
            tuya_transporter_close(network);
            tuya_transporter_destroy(network);
            return HTTP_CLIENT_SEND_FAULT;
        }
    }
    /* http client TransportInterface */
    TransportInterface_t pTransportInterface = {.pNetworkContext = (NetworkContext_t *)&network,
                                                .recv = (TransportRecv_t)NetworkTransportRecv,
                                                .send = (TransportSend_t)NetworkTransportSend};

    /* http client request object make */
    HTTPRequestInfo_t requestInfo = {
        .pMethod = request->method,
        .methodLen = strlen(request->method),
        .pHost = request->host,
        .hostLen = strlen(request->host),
        .pPath = request->path,
        .pathLen = strlen(request->path),
    };

    HTTPResponse_t http_response = {0};

    /* HTTP request send */
    log_debug("http request send!");
    rt = core_http_request_send((const TransportInterface_t *)&pTransportInterface,
                                (const HTTPRequestInfo_t *)&requestInfo, request->headers, request->headers_count,
                                (const uint8_t *)request->body, request->body_length, &http_response);

    /* tls disconnect */
    tuya_transporter_close(network);
    tuya_transporter_destroy(network);

    if (OPRT_OK != rt) {
        log_error("http_request_send error:%d", rt);
        return rt;
    }

    /* Response copy out */
    response->status_code = http_response.statusCode;
    response->body = http_response.pBody;
    response->body_length = http_response.bodyLen;
    response->headers = http_response.pHeaders;
    response->headers_length = http_response.headersLen;
    response->buffer = http_response.pBuffer;
    response->buffer_length = http_response.bufferLen;

    return HTTP_CLIENT_SUCCESS;
}

int http_client_free(http_client_response_t *response)
{
    if (NULL == response) {
        return OPRT_INVALID_PARM;
    }

    if (response->buffer) {
        tal_free(response->buffer);
    }

    if (response->body) {
        tal_free(response->body);
    }

    return OPRT_OK;
}
