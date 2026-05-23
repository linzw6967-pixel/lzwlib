#pragma once
#include "esp_websocket_client.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "Common/Common.h"
#include "esp_crt_bundle.h"

void xiaozhi_websocket_init(void);


void xiaozhi_websocket_sendtext(char *text,int data_len);
void xiaozhi_websocket_sendbinary(char *binary,int len);
void xiaozhi_websocket_sendhello(void);

bool xiaozhi_websocket_startconnect(void);
void xiaozhi_websocket_sendwakeupword(void);

/* 发送开始监听 */
void xiaozhi_ws_send_start_listen(void);

/* 发送停止监听 */
void xiaozhi_ws_send_stop_listen(void);

/* 发送终止消息 */
void xiaozhi_ws_send_abort(void);

void xiaozhi_ws_send_volume_command(void);

/* 发送音量初始化状态的数据 */
void xiaozhi_ws_send_volume_init(void);
