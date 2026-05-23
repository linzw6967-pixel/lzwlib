#pragma once


#include "stddef.h"
#include "esp_opus_enc.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/ringbuf.h"
#include "Common/Common.h"
#include "string.h"

void xiaozhi_audio_encoder_init(void);