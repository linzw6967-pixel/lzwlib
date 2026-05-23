#include <stdio.h>
#include "esp_log.h"
#include "ADC_Button/ADC_Button.h"
#include "Internet/xiaozhi_wifi.h"
#include "Internet/xiaozhi_active.h"
#include "Display/xiaozhi_lcd.h"
#include "Display/xiaozhi_lvgl.h"
#include "Common/Common.h"
#include "audio/xiaozhi_audio.h"
#include "es8311_codec.h"
#include "audio/xiaozhi_audio_decoder.h"
#include "audio/xiaozhi_audio_encoder.h"
#include "Internet/xiaozhi_websocket.h"
#include "freertos/event_groups.h"

#define TAG "main"
// 函数声明
void audio_sr_create_all_ringsbuf(void);
void xiaozhi_wakeup_callback(void);
void xiaozhi_websocket_process_frame(char *data, size_t len);
void xiaozhi_websocket_process_speechframe(char *data, size_t len);
void xiaozhi_sr_register_callback(void);
void xiaozhi_ws_process_audio_task(void *pvParameters);

// extern es8311_codec_cfg_t es8311_cfg;
xiaozhi_handle_t xiaozhi_handle = {0};

// 定义按钮按下状态枚举
typedef enum
{
    SW2_SINGLE_CLICK,
    SW3_SINGLE_CLICK,
    SW3_LONG_CLICK,
} Button_event;

static void button_event_cb(void *arg, void *usr_data);
void app_main(void)
{
    // 创建环形缓冲区
    audio_sr_create_all_ringsbuf();
    // 创建事件标志组
    xiaozhi_handle.xiaozhi_event_group = xEventGroupCreate();
    // 注册唤醒回调函数
    xiaozhi_handle.wakeup_callback = xiaozhi_wakeup_callback;
    // 注册返回信息帧处理的回调函数
    xiaozhi_handle.websocket_process_farme = xiaozhi_websocket_process_frame;
    // 注册语音接收函数指针
    xiaozhi_handle.websocket_process_audio = xiaozhi_websocket_process_speechframe;
    // 注册小智状态服务函数
    xiaozhi_handle.voice_change_callback = xiaozhi_sr_register_callback;
    ADC_Button_Init();
    // sw2单击回调注册
    ADC_sw2button_Register_callback(BUTTON_SINGLE_CLICK, button_event_cb, (void *)SW2_SINGLE_CLICK);
    // sw3单击回调注册
    ADC_sw3button_Register_callback(BUTTON_SINGLE_CLICK, button_event_cb, (void *)SW3_SINGLE_CLICK);
    // sw3长按回调注册
    ADC_sw3button_Register_callback(BUTTON_LONG_PRESS_START, button_event_cb, (void *)SW3_LONG_CLICK);

    xiaozhi_lvgl_init();

    xiaozhi_wifi_init();

    xiaozhi_http_active();
    // ! 初始化小智的状态 刚开始为空闲状态
    xiaozhi_handle.xiaozhi_state = XIAOZHI_STATE_IDLE;

    xiaozhi_sr_init();
    xiaozhi_audio_encoder_init();
    xiaozhi_audio_decoder_init();

    xiaozhi_websocket_init();

    // 注册发送给服务器语音数据的回调函数
    xTaskCreatePinnedToCoreWithCaps(xiaozhi_ws_process_audio_task, "xiaozhi_ws_process_audio_task", 1024 * 16, NULL, 5, NULL, 0, MALLOC_CAP_SPIRAM);
}
// 按钮事件回调函数
static void button_event_cb(void *arg, void *usr_data)
{
    uint8_t event = (uint8_t)usr_data;
    switch (event)
    {
    case SW2_SINGLE_CLICK:
        ESP_LOGI(TAG, "SW2_SINGLE_CLICK");
        break;
    case SW3_SINGLE_CLICK:
        ESP_LOGI(TAG, "SW3_SINGLE_CLICK");
        break;
    case SW3_LONG_CLICK: // SW3_LONG_CLICK:
        ESP_LOGI(TAG, "SW3_LONG_CLICK");
        xiaozhi_wifi_clear();
        break;
    default:
        break;
    }
}

// 唤醒后所调用回调函数
void xiaozhi_wakeup_callback(void)
{

    if (xiaozhi_handle.xiaozhi_state == XIAOZHI_STATE_IDLE)
    {
        if (xiaozhi_websocket_startconnect() == true)
        {
            // ! 发送控制音量相关的数据
            xiaozhi_ws_send_volume_command();
            xiaozhi_ws_send_volume_init();

            xiaozhi_websocket_sendhello();
        }

        EventBits_t hello_wait = xEventGroupWaitBits(xiaozhi_handle.xiaozhi_event_group, WEBSOCKET_Event_Hello_Receive, pdTRUE, pdFALSE, portMAX_DELAY);
        if (!(hello_wait & WEBSOCKET_Event_Hello_Receive))
        {
            ESP_LOGI(TAG, "hello_wait fail");
            //return;
        }
        else
        {
            ESP_LOGI(TAG, "hello_wait success");
            xiaozhi_websocket_sendwakeupword();
        }
    }
    else if (xiaozhi_handle.xiaozhi_state == XIAOZHI_STATE_SPEAK)
    {
        xiaozhi_ws_send_abort();
        // 再次发送唤醒词
        xiaozhi_websocket_sendwakeupword();
    }
}
// 处理文本帧回调函数
void xiaozhi_websocket_process_frame(char *data, size_t len)
{
    //根据返回文本获取返回内容
    cJSON *root = cJSON_Parse(data);
    if (root == NULL)
    {
        ESP_LOGI(TAG, "cJSON_Parse fail");
        return;
    }
    // 获取文本中type内容
    cJSON *type = cJSON_GetObjectItem(root, "type");
    if (type == NULL)
    {
        ESP_LOGI(TAG, "type is null");
        return;
    }
    // 判断hello是否发发送成功
    if (strcmp(type->valuestring, "hello") == 0)
    {
        ESP_LOGI(TAG, "hello is receive");
        xEventGroupSetBits(xiaozhi_handle.xiaozhi_event_group, WEBSOCKET_Event_Hello_Receive);
    }
    // 判断type中有无“llm”
    if (strcmp(type->valuestring, "llm") == 0)
    {
        char *emoji = cJSON_GetObjectItem(root, "emotion")->valuestring;
        xiaozhi_lvgl_showemoji(emoji);
    }
    // 判断type中有无“tts”
    if (strcmp(type->valuestring, "tts") == 0)
    {
        // 获取文本中state内容
        cJSON *state = cJSON_GetObjectItem(root, "state");
        if (strcmp(type->valuestring, "sentence_start") == 0)
        {
            char *text = cJSON_GetObjectItem(root, "text")->valuestring;
            xiaozhi_lvgl_showlabel(text);
        }
    }


    //根据返回调整音量方法设置音量
    if (strcmp(type->valuestring, "iot") == 0)
    {
        cJSON *cmdsItem = cJSON_GetObjectItem(root, "commands");
        cJSON *cmdItem = cJSON_GetArrayItem(cmdsItem, 0);
        cJSON *parametersItem = cJSON_GetObjectItem(cmdItem, "parameters");

        char *method = cJSON_GetObjectItem(cmdItem, "method")->valuestring;

        if (strcmp(method, "SetVolume") == 0)
        {
            int volumne = cJSON_GetObjectItem(parametersItem, "volume")->valueint;

            xiaozhi_audio_volume_set(volumne);
        }
        else if (strcmp(method, "SetMute") == 0)
        {
            cJSON *muteItem = cJSON_GetObjectItem(parametersItem, "mute");
            bool is_mute = false;
            if (cJSON_IsBool(muteItem))
            {
                is_mute = cJSON_IsTrue(muteItem);
            }
            xiaozhi_audio_mute_set(is_mute);
        }
    }

    cJSON_Delete(root);
}
// 处理音频帧回调函数
void xiaozhi_websocket_process_speechframe(char *data, size_t len)
{
    xRingbufferSend(xiaozhi_handle.wsToDecoder_buff_handle, data, len, pdMS_TO_TICKS(5000));
}
// 小智服务器状态设置回调函数
void xiaozhi_sr_register_callback(void)
{
    ESP_LOGI(TAG, "%s", xiaozhi_handle.xiaozhi_vad_current_status ? "声音" : "静默");
    // 说话时判断一下小智状态是否空闲，若小智正在说话那么会影响识别结果
    // 等到空闲状态就将小智状态置为聆听
    if (xiaozhi_handle.xiaozhi_vad_current_status == VAD_SPEECH)
    {
        if (xiaozhi_handle.xiaozhi_state == XIAOZHI_STATE_IDLE)
        {
            xiaozhi_handle.xiaozhi_state = XIAOZHI_STATE_LISTEN;
            xiaozhi_ws_send_start_listen();
        }
    }
    // 如果说话结束处于静默状态，且小智没有在说话就将小智状态置为空闲
    if (xiaozhi_handle.xiaozhi_vad_current_status == VAD_SILENCE)
    {
        if (xiaozhi_handle.xiaozhi_state != XIAOZHI_STATE_SPEAK)
        {
            xiaozhi_handle.xiaozhi_state = XIAOZHI_STATE_IDLE;
            xiaozhi_ws_send_stop_listen();
        }
    }
}

// 定义不断发送音频数据给服务器的任务
void xiaozhi_ws_process_audio_task(void *pvParameters)
{

    size_t data_len = 0;
    while (1)
    {
        void *rev_buff = xRingbufferReceive(xiaozhi_handle.encoderToWs_buff_handle, &data_len, portMAX_DELAY);
        if (xiaozhi_handle.xiaozhi_state == XIAOZHI_STATE_LISTEN)
        {
            xiaozhi_websocket_sendbinary((char *)rev_buff, data_len);
        }
        vRingbufferReturnItem(xiaozhi_handle.encoderToWs_buff_handle, rev_buff);
    }
}

// 创建所需的所有环形缓冲区
void audio_sr_create_all_ringsbuf(void)
{
    xiaozhi_handle.srToencoder_buff_handle = xRingbufferCreateWithCaps(1024 * 32, RINGBUF_TYPE_BYTEBUF, MALLOC_CAP_SPIRAM);
    xiaozhi_handle.encoderToWs_buff_handle = xRingbufferCreateWithCaps(1024 * 32, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
    xiaozhi_handle.wsToDecoder_buff_handle = xRingbufferCreateWithCaps(1024 * 128, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
}