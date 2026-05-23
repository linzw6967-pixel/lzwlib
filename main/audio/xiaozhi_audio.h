#pragma once

#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "driver/i2c_master.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "Common/Common.h"
#include "audio/xiaozhi_sr.h"


void xiaozhi_audio_init();
void xiaozhi_audio_play(uint8_t *data,uint32_t data_len);
void xiaozhi_audio_record(uint8_t *data,uint32_t data_len);


/* 控制音量 */
void xiaozhi_audio_volume_set(int volume);

/* 设置是否静音 */
void xiaozhi_audio_mute_set(bool mute);