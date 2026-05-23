#include "xiaozhi_audio_encoder.h"

#define TAG "xiaozhi_audio_encoder"

void xiaozhi_audio_encoder_transformdata(void *parameter);

esp_audio_enc_handle_t encoder_handle = NULL;

uint8_t *pcm_data = NULL;
uint8_t *raw_data = NULL;

// 定义输入输出缓存区大小
int opus_output_size = 0;
int pcm_input_size = 0;

void xiaozhi_audio_encoder_init(void)
{
    esp_opus_enc_config_t opus_cfg = {
        .sample_rate = ESP_AUDIO_SAMPLE_RATE_16K,
        .channel = ESP_AUDIO_MONO,
        .bits_per_sample = ESP_AUDIO_BIT16,
        .bitrate = 32000,
        .frame_duration = ESP_OPUS_ENC_FRAME_DURATION_60_MS,
        .application_mode = ESP_OPUS_ENC_APPLICATION_AUDIO,
        .complexity = 5,

        .enable_fec = false,
        .enable_dtx = false,
        .enable_vbr = false,
    };

    // 开启编码器
    esp_opus_enc_open(&opus_cfg, sizeof(esp_opus_enc_config_t), &encoder_handle);

    // 对输入输出帧大小赋值
    esp_opus_enc_get_frame_size(encoder_handle, &pcm_input_size, &opus_output_size);
    ESP_LOGI(TAG, "pcm_input_size:%d, opus_output_size:%d", pcm_input_size, opus_output_size);

    // 开辟输入输出帧内存大小
    pcm_data = heap_caps_malloc(pcm_input_size, MALLOC_CAP_SPIRAM);
    raw_data = heap_caps_malloc(opus_output_size, MALLOC_CAP_SPIRAM);

    // 创建加工环形缓冲区数据并把数据传出方法的FreeRTOS任务
    xTaskCreatePinnedToCoreWithCaps(xiaozhi_audio_encoder_transformdata, "xiaozhi_audio_encoder_transformdata", 1024 * 32, NULL, 5, NULL, 1, MALLOC_CAP_SPIRAM);
}

// 加工环形缓冲区数据并把数据传出方法
void xiaozhi_audio_encoder_transformdata(void *parameter)
{

    // Do encoding
    esp_audio_enc_in_frame_t in_frame = {
        .buffer = pcm_data,
        .len = pcm_input_size,
    };
    esp_audio_enc_out_frame_t out_frame = {
        .buffer = raw_data,
        .len = opus_output_size,
    };

    while (1)
    {

        // 需要接收的数据长度
        size_t remain_size = pcm_input_size;
        // 将接收数据位置的起始地址指针赋值给中间缓冲地址
        uint8_t *p_start_addr = in_frame.buffer;

        while (remain_size > 0)
        {
            size_t item_size = 0;
            void *item = xRingbufferReceiveUpTo(xiaozhi_handle.srToencoder_buff_handle, &item_size, portMAX_DELAY, remain_size);
            memcpy(p_start_addr, item, item_size);
            p_start_addr += item_size;
            remain_size -= item_size;
            // 输出当前数据
            // ESP_LOGI(TAG, "item pointer=%p, item_size=%d", item, item_size);

            // 清除环形缓冲区数据
            vRingbufferReturnItem(xiaozhi_handle.srToencoder_buff_handle, item);
        }

        // 处理数据
        esp_opus_enc_process(encoder_handle, &in_frame, &out_frame);

        // 将数据给下一个环形缓冲区
        xRingbufferSend(xiaozhi_handle.encoderToWs_buff_handle, out_frame.buffer, out_frame.encoded_bytes, portMAX_DELAY);
    }
}