#pragma once
#include "Common/Common.h"
#include "freertos/ringbuf.h"
#include "esp_opus_dec.h"
#include "xiaozhi_audio.h"
#include "esp_heap_caps.h"

void xiaozhi_audio_decoder_init(void);
