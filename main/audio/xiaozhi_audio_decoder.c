#include "xiaozhi_audio_decoder.h"
#define TAG "xiaozhi_audio_decoder"
esp_audio_dec_handle_t decoder_handle;

void xiaozhi_audio_decoder_transformdata_task(void *pvParameters);

void xiaozhi_audio_decoder_init(void)
{
    // 解码器基础配置
    esp_opus_dec_cfg_t opus_cfg = {
        .sample_rate = ESP_AUDIO_SAMPLE_RATE_16K,
        .channel = ESP_AUDIO_MONO,
        .frame_duration = ESP_OPUS_DEC_FRAME_DURATION_60_MS,
        .self_delimited = false,
    };

    // Open OPUS encoder
    esp_opus_dec_open(&opus_cfg, sizeof(opus_cfg), &decoder_handle);

    xTaskCreatePinnedToCoreWithCaps(xiaozhi_audio_decoder_transformdata_task, "xiaozhi_audio_decoder_transformdata_task", 1024 * 16, NULL, 8, NULL, 0, MALLOC_CAP_SPIRAM);
}

void xiaozhi_audio_decoder_transformdata_task(void *pvParameters)
{
    esp_audio_dec_in_raw_t opus_in_frame;
    esp_audio_dec_out_frame_t pcm_out_frame =
        {
            .buffer = heap_caps_malloc(8 * 1024, MALLOC_CAP_SPIRAM),
            .len = 8 * 1024,
        };

    esp_audio_dec_info_t aud_info;

    while (1)
    {
        void *rev_buf = xRingbufferReceive(xiaozhi_handle.wsToDecoder_buff_handle, &opus_in_frame.len, portMAX_DELAY);
        opus_in_frame.buffer = rev_buf;

        while (opus_in_frame.len > 0)
        {
            esp_opus_dec_decode(decoder_handle, &opus_in_frame, &pcm_out_frame, &aud_info);
            opus_in_frame.len -= opus_in_frame.consumed;
            opus_in_frame.buffer += opus_in_frame.consumed;
            
            //ESP_LOGI(TAG, "%s", (char *)pcm_out_frame.buffer);
            // ESP_LOGI()
            // 将数据发给audio播放
            xiaozhi_audio_play(pcm_out_frame.buffer, pcm_out_frame.decoded_size);
        }

        // 释放ringbuffer内存
        vRingbufferReturnItem(xiaozhi_handle.wsToDecoder_buff_handle, rev_buf);
    }
}
