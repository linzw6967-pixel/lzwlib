#include "xiaozhi_active.h"

#define TAG "xiaozhi_active"
esp_err_t _http_event_handler(esp_http_client_event_t *evt);
uint8_t mac[6] = {0};
char mac_str[18];
char uuid_str[37];
char *requset_body_str;
esp_http_client_handle_t client;

static char *output_buffer; // Buffer to store response of http request from event handler
static int output_len;      // Stores number of bytes read
static size_t buffer_index = 0;
// // 用于接收服务器端返回的数据及长度
// static char *output_buffer; // Buffer to store response of http request from event handler
// static int output_len;      // Stores number of bytes read
// static int current_index = 0;

// char *post_data;

void xiaozhi_active_setheader(void);
void xiaozhi_active_getuuid(char *uuid_str);
void xiaozhi_active_setbody(void);
esp_err_t _http_event_handler(esp_http_client_event_t *evt);
void xiaozhi_parse_json(char *json_data);

// 网络请求
void xiaozhi_http_active(void)
{
    esp_http_client_config_t config = {
        .host = "api.tenclass.net",
        .path = "/xiaozhi/ota/",
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .event_handler = _http_event_handler,
        // .cert_pem = howsmyssl_com_root_cert_pem_start,
        //! 注意下面,选用了另一种方法
        .crt_bundle_attach = esp_crt_bundle_attach,
        .method = HTTP_METHOD_POST,
    };
    client = esp_http_client_init(&config);

    // 设置请求头
    xiaozhi_active_setheader();
    // 设置请求体
    xiaozhi_active_setbody();
    // 发送请求
    while (esp_http_client_perform(client) != ESP_OK)
    {

        xiaozhi_lvgl_showtile("激活网络");

        xiaozhi_lvgl_showlabel("网络不好，重连中。。。");

        xiaozhi_lvgl_showemoji("crying");
        ESP_LOGI(TAG, "激活网络重连中。。。");

        vTaskDelay(20);
    }
    ESP_LOGI(TAG, "连接成功");
    xiaozhi_lvgl_showtile("AI小智");

    xiaozhi_lvgl_showlabel("欢迎");

    xiaozhi_lvgl_showemoji("angry");
}

// 请求头设置方法
void xiaozhi_active_setheader(void)
{
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "User-Agent", "bread-compact-wifi-128x64/1.0.1");

    // 获取mac地址

    esp_wifi_get_mac(WIFI_IF_STA, mac);
    sprintf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    esp_http_client_set_header(client, "Device-Id", mac_str);
    ESP_LOGI(TAG, "MAC:%s", mac_str);
    // 设置UUID
    xiaozhi_active_getuuid(uuid_str);
    esp_http_client_set_header(client, "Client-Id", uuid_str);
    ESP_LOGI(TAG, "uuid:%s", uuid_str);
}
//生成uuid
void xiaozhi_active_getuuid(char *uuid_str)
{
    uint32_t random_data[4];
    // 使用 esp_random() 获取高质量的随机数
    random_data[0] = esp_random();
    random_data[1] = esp_random();
    random_data[2] = esp_random();
    random_data[3] = esp_random();

    // 设置版本位 (4) 和变体位 (8,9,A,B)
    // 第6字节的高4位设置为4 (0100)
    random_data[1] = (random_data[1] & 0xFFFF0FFF) | 0x00004000;
    // 第8字节的高4位设置为8-11 (1000-1011)
    random_data[2] = (random_data[2] & 0x3FFFFFFF) | 0x80000000;

    // 格式化为 UUID 字符串
    sprintf(uuid_str, "%08lx-%04lx-%04lx-%04lx-%04lx%08lx",
            random_data[0],
            random_data[1] >> 16,
            random_data[1] & 0xFFFF,
            random_data[2] >> 16,
            random_data[2] & 0xFFFF,
            random_data[3]);
}
//设置请求体
void xiaozhi_active_setbody(void)
{
    // 创建json对象
    cJSON *root = cJSON_CreateObject();
    cJSON *application = cJSON_CreateObject();
    cJSON *board = cJSON_CreateObject();

    cJSON_AddItemToObject(root, "application", application);
    cJSON_AddItemToObject(root, "board", board);

    cJSON_AddStringToObject(application, "version", "1.0.1");
    cJSON_AddStringToObject(application, "elf_sha256", esp_app_get_elf_sha256_str());

    cJSON_AddStringToObject(board, "type", "bread-compact-wifi");
    cJSON_AddStringToObject(board, "name", "bread-compact-wifi-128x64");
    cJSON_AddStringToObject(board, "ssid", "卧室");
    cJSON_AddNumberToObject(board, "rssi", -55);
    cJSON_AddNumberToObject(board, "channel", 1);
    cJSON_AddStringToObject(board, "ip", "192.168.1.11");
    cJSON_AddStringToObject(board, "mac", mac_str);

    requset_body_str = cJSON_PrintUnformatted(root);
    ESP_LOGI(TAG, "requset_body_str:%s", requset_body_str);

    // 添加请求体内容
    esp_http_client_set_post_field(client, requset_body_str, strlen(requset_body_str));

    cJSON_Delete(root);
}

// 回调函数
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{

    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        if (strcmp(evt->header_key, "Content-Length") == 0)
        {
            output_len = atoi(evt->header_value);
            output_buffer = heap_caps_calloc(output_len + 1, sizeof(char), MALLOC_CAP_SPIRAM);
        }
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        // Clean the buffer in case of a new request
        memcpy(output_buffer + buffer_index, evt->data, evt->data_len);
        buffer_index += evt->data_len;
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        ESP_LOGI(TAG, "output_buffer: %s, output_len: %d", output_buffer, output_len);

        xiaozhi_parse_json(output_buffer);

        cJSON_free(requset_body_str);

        if (output_buffer != NULL)
        {

            // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
            // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        buffer_index = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        if (output_buffer != NULL)
        {
            free(output_buffer);
            output_buffer = NULL;
        }
        buffer_index = 0;
        output_len = 0;
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        esp_http_client_set_header(evt->client, "From", "user@example.com");
        esp_http_client_set_header(evt->client, "Accept", "text/html");
        esp_http_client_set_redirection(evt->client);
        break;
    }
    return ESP_OK;
}

// 对服务器返回的json数据进行解析
void xiaozhi_parse_json(char *json_data)
{
    // 创建一个根节点
    cJSON *root = cJSON_Parse(json_data);
    if (root == NULL)
    {
        ESP_LOGI(TAG, "json_data error");
        return;
    }
    // 从root对象中获取websocket对象
    cJSON *websocket = cJSON_GetObjectItem(root, "websocket");
    if (websocket == NULL)
    {
        ESP_LOGI(TAG, "404 NOT Found");
        return;
    }
    // 从websocket对象中获取url对象
    char *websocket_url = cJSON_GetObjectItem(websocket, "url")->valuestring;
    xiaozhi_handle.websocket_url = heap_caps_calloc(strlen(websocket_url) + 1, sizeof(char), MALLOC_CAP_SPIRAM);
    memcpy(xiaozhi_handle.websocket_url, websocket_url, strlen(websocket_url));

    // 从websocket对象中获取token对象
    char *websocket_token = cJSON_GetObjectItem(websocket, "token")->valuestring;

    xiaozhi_handle.websocket_token = heap_caps_calloc(strlen(websocket_token) + 1, sizeof(char), MALLOC_CAP_SPIRAM);
    memcpy(xiaozhi_handle.websocket_token, websocket_token, strlen(websocket_token));

    // 从root对象中获取activation对象
    cJSON *activation = cJSON_GetObjectItem(root, "activation");
    if (activation == NULL)
    {
        xiaozhi_handle.is_active = false;
        ESP_LOGI(TAG, "设备已经被激活");
        xiaozhi_lvgl_showtile("设备激活");
        xiaozhi_lvgl_showemoji("cool");
        xiaozhi_lvgl_showlabel("设备已经被激活");
    }
    else
    {
        ESP_LOGI(TAG, "设备未被激活");
        // 标题显示
        xiaozhi_lvgl_showtile("设备激活");
        xiaozhi_handle.is_active = true;
        // 从activation对象中获取code
        char *code = cJSON_GetObjectItem(activation, "code")->valuestring;
        xiaozhi_handle.active_code = heap_caps_calloc(strlen(code) + 1, sizeof(char), MALLOC_CAP_SPIRAM);
        memcpy(xiaozhi_handle.active_code, code, strlen(code));
        // 文本显示
        char show_code[100];
        sprintf(show_code, "激活码为:%s", xiaozhi_handle.active_code);
        xiaozhi_lvgl_showlabel("设备未被激活/n请输入激活码激活设备:/n");
        xiaozhi_lvgl_showlabel(show_code);
        xiaozhi_lvgl_showemoji("loving");
        ESP_LOGI(TAG, "激活码为:%s", show_code);
    }

    // 打印获取数据
    ESP_LOGI(TAG, "URL为:%s", xiaozhi_handle.websocket_url);
    ESP_LOGI(TAG, "TOKEN为:%s", xiaozhi_handle.websocket_token);

    cJSON_Delete(root);
}