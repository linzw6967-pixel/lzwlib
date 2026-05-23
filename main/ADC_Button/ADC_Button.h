#pragma once

#include "button_adc.h"
#include "iot_button.h"
#include "esp_log.h"

void ADC_Button_Init(void);

void ADC_sw2button_Register_callback(button_event_t event, button_cb_t cb,void *usr_data);

void ADC_sw3button_Register_callback(button_event_t event, button_cb_t cb,void *usr_data);

