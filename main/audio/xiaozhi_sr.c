#include "audio/xiaozhi_sr.h"

#define TAG "xiaozhi_sr"

const esp_afe_sr_iface_t *afe_handle = NULL;
esp_afe_sr_data_t *afe_data;

void xiaozhi_sr_feeddata_Task(void *pvParameters);
void xiaozhi_sr_detectdata_Task(void *pvParameters);

// //! 编写一个待注册函数测试效果
// void xiaozhi_sr_register_callback(void)
// {
//     ESP_LOGI(TAG, "%s", xiaozhi_handle.xiaozhi_vad_current_status ? "声音" : "静默");
// }

void xiaozhi_sr_init(void)
{
    xiaozhi_audio_init();
    // 注册回调函数
    //xiaozhi_handle.voice_change_callback = xiaozhi_sr_register_callback;

    // 初始化配置
    srmodel_list_t *models = esp_srmodel_init("model");
    afe_config_t *afe_config = afe_config_init("M", models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);

    // 判断噪声或者静默的最小延时时间
    afe_config->vad_min_noise_ms = 1000;
    // 判断连续人声的最小延时时间间隔
    afe_config->vad_min_speech_ms = 128;
    // 设置语音活动检测的标准，值越大，要求越严格越难检测到
    afe_config->vad_mode = VAD_MODE_1;
    // 设置环形缓冲区大小
    afe_config->afe_ringbuf_size = 1024 * 6;

    // 禁用人声增强
    afe_config->se_init = false;
    // 禁用回声消除
    afe_config->aec_init = false;
    // 禁用噪声消除
    afe_config->ns_init = false;

    // 提升了唤醒的灵敏度
    afe_config->wakenet_mode = DET_MODE_90;
    // 更多的内存分配到外部的sram里去
    afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;

    //
    afe_handle = esp_afe_handle_from_config(afe_config);
    afe_data = afe_handle->create_from_config(afe_config);

    // 创建任务
    xTaskCreatePinnedToCoreWithCaps(xiaozhi_sr_feeddata_Task, "feed", 8 * 1024, NULL, 5, NULL, 0, MALLOC_CAP_SPIRAM);
    xTaskCreatePinnedToCoreWithCaps(xiaozhi_sr_detectdata_Task, "detect", 10 * 1024, NULL, 5, NULL, 1, MALLOC_CAP_SPIRAM);
}


//将数据发送给编码器
void xiaozhi_sr_feeddata_Task(void *pvParameters)
{

    // get feed chunksize
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);

    // get feed channel num
    int feed_channel = afe_handle->get_feed_channel_num(afe_data);

    // 为喂数据开空间
    int16_t *i2s_buff = heap_caps_calloc(audio_chunksize * sizeof(int16_t) * feed_channel, sizeof(int16_t), MALLOC_CAP_SPIRAM);

    // TickType_t xLastWakeTime;
    // const TickType_t xFrequency = pdMS_TO_TICKS(1);
    // // 必须初始化 xLastWakeTime!
    // xLastWakeTime = xTaskGetTickCount();

    // 向ES8311要数据
    while (1)
    {
        xiaozhi_audio_record((uint8_t *)i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);
        afe_handle->feed(afe_data, i2s_buff);
        // vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void xiaozhi_sr_detectdata_Task(void *pvParameters)
{

    //  TickType_t xLastWakeTime = xTaskGetTickCount(); // 必须初始化
    //  const TickType_t xFrequency = pdMS_TO_TICKS(1);

    while (1)
    {
        // 通过fetch获取数据
        afe_fetch_result_t *result = afe_handle->fetch(afe_data);
        // 得到最终结果
        int16_t *processed_audio = result->data;

        //! 获取数据大小

        // 获取vad状态 "Voice Activity Detection"（语音活动检测）
        vad_state_t vad_state = result->vad_state;
        // 获取唤醒状态
        wakenet_state_t wakeup_state = result->wakeup_state;

        if (wakeup_state == WAKENET_DETECTED)
        {
            ESP_LOGI(TAG, "检测到唤醒");
            xiaozhi_handle.wkup_flag = true;
            if (xiaozhi_handle.wakeup_callback != NULL)
            {
                xiaozhi_handle.wakeup_callback();
            }
        }

        if (xiaozhi_handle.wkup_flag)
        {
            // 记录当前状态
            xiaozhi_handle.xiaozhi_vad_current_status = vad_state;
            // 判断有无状态切换
            if (xiaozhi_handle.xiaozhi_vad_current_status != xiaozhi_handle.xiaozhi_vad_last_status)
                // 调用回调函数
                if (xiaozhi_handle.voice_change_callback != NULL)
                {
                    xiaozhi_handle.voice_change_callback();
                }
            // 将当前状态赋值给上一次
            xiaozhi_handle.xiaozhi_vad_last_status = xiaozhi_handle.xiaozhi_vad_current_status;
        }

        if (xiaozhi_handle.wkup_flag && result->vad_state == VAD_SPEECH)
        {
            if (result->vad_cache_size > 0)
            {
                // 识别人声
                xRingbufferSend(xiaozhi_handle.srToencoder_buff_handle, result->vad_cache, result->vad_cache_size, portMAX_DELAY);
            }
            xRingbufferSend(xiaozhi_handle.srToencoder_buff_handle, result->data, result->data_size, portMAX_DELAY);
        }
        // vTaskDelayUntil(&xLastWakeTime, xFrequency);
        // pdTICKS_TO_MS(5000)
    }
}