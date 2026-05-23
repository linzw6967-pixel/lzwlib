#include "xiaozhi_websocket.h"
#define TAG "xiaozhi_websocket"
extern char mac_str[18];
extern char uuid_str[37];
static char *volume_str=
#include "xiaozhi_command.json"
;
static char *volume_init_str=
#include "xiaozhi_init_state.json"
;
esp_websocket_client_handle_t wb_client;

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void xiaozhi_websocket_init(void)
{
    esp_websocket_client_config_t websocket_cfg = {0};

    websocket_cfg.uri = xiaozhi_handle.websocket_url;
    websocket_cfg.transport = WEBSOCKET_TRANSPORT_OVER_SSL;
    websocket_cfg.crt_bundle_attach = esp_crt_bundle_attach;
    websocket_cfg.reconnect_timeout_ms = 2000;
    websocket_cfg.network_timeout_ms = 2000;

    // 配置初始化
    wb_client = esp_websocket_client_init(&websocket_cfg);

    // 设置请求头
    size_t auth_len = strlen("Bearer ") + strlen(xiaozhi_handle.websocket_token) + 1;
    char *auth = heap_caps_malloc(auth_len, MALLOC_CAP_SPIRAM);
    sprintf(auth, "Bearer %s", xiaozhi_handle.websocket_token);
    esp_websocket_client_append_header(wb_client, "Authorization", auth);
    esp_websocket_client_append_header(wb_client, "Protocol-Version", "1");
    esp_websocket_client_append_header(wb_client, "Device-Id", mac_str);
    esp_websocket_client_append_header(wb_client, "Client-Id", uuid_str);

    // 注册回调函数
    esp_websocket_register_events(wb_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)wb_client);
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id)
    {
    case WEBSOCKET_EVENT_BEGIN:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_BEGIN");
        break;
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        xEventGroupSetBits(xiaozhi_handle.xiaozhi_event_group, WEBSOCKET_Event_Connected);
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
        // 判断是文本帧
        if (data->op_code == WS_TRANSPORT_OPCODES_TEXT)
        {
            ESP_LOGI(TAG, "WS_TRANSPORT_OPCODES_TEXT: %s", data->data_ptr);
            xiaozhi_handle.websocket_process_farme(data->data_ptr, data->data_len);
        }
        // 判断是二进制帧
        if (data->op_code == WS_TRANSPORT_OPCODES_BINARY)
        {
            ESP_LOGI(TAG, "WS_TRANSPORT_OPCODES_BINARY: %d", data->data_len);
            xiaozhi_handle.websocket_process_audio(data->data_ptr, data->data_len);
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    case WEBSOCKET_EVENT_FINISH:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_FINISH");
        break;
    }
}

void xiaozhi_websocket_sendtext(char *text, int data_len)
{
    esp_websocket_client_send_text(wb_client, text, data_len, portMAX_DELAY);
}
void xiaozhi_websocket_sendbinary(char *binary,int len)
{
    esp_websocket_client_send_bin(wb_client, binary,len,portMAX_DELAY);
}
void xiaozhi_websocket_sendhello(void)
{
    char *send_buff = "{\"type\":\"hello\",\"version\":1,\"transport\":\"websocket\",\"features\":{\"mcp\":true},\"audio_params\":{\"format\":\"opus\",\"sample_rate\":16000,\"channels\":1,\"frame_duration\":60}}";
    xiaozhi_websocket_sendtext(send_buff, strlen(send_buff));
}

bool xiaozhi_websocket_startconnect(void)
{
    if (esp_websocket_client_is_connected(wb_client))
    {
        return true;
    }

    // 发起链接请求
    esp_websocket_client_start(wb_client);
    EventBits_t ret = xEventGroupWaitBits(xiaozhi_handle.xiaozhi_event_group, WEBSOCKET_Event_Connected, true, false, portMAX_DELAY);

    if (!(ret & WEBSOCKET_Event_Connected))
    {
        ESP_LOGI(TAG, "连接失败");
        return false;
    }

    return true;
}

void xiaozhi_websocket_sendwakeupword(void)
{
    char *send_buff = "{\"type\":\"listen\",\"state\":\"detect\",\"text\":\"你好，小智\"}";
    xiaozhi_websocket_sendtext(send_buff, strlen(send_buff));
}

/* 发送开始监听 */
void xiaozhi_ws_send_start_listen(void)
{
  char *start_listen_str = "{\"type\":\"listen\",\"state\":\"start\",\"mode\":\"realtime\"}";
  xiaozhi_websocket_sendtext(start_listen_str, strlen(start_listen_str));
}

/* 发送停止监听 */
void xiaozhi_ws_send_stop_listen(void)
{
  char *stop_listen_str = "{\"type\":\"listen\",\"state\":\"stop\"}";
  xiaozhi_websocket_sendtext(stop_listen_str, strlen(stop_listen_str));
}

/* 发送终止消息 */
void xiaozhi_ws_send_abort(void)
{
  char *abort_str = "{\"type\":\"abort\",\"reason\":\"wake_word_detected\"}";
  xiaozhi_websocket_sendtext(abort_str, strlen(abort_str));
}


/* 发送控制音量的命令格式 */
void xiaozhi_ws_send_volume_command(void)
{

  xiaozhi_websocket_sendtext(volume_str, strlen(volume_str));
}
/* 发送音量初始化状态的数据 */
void xiaozhi_ws_send_volume_init(void)
{
  xiaozhi_websocket_sendtext(volume_init_str, strlen(volume_init_str));
}
