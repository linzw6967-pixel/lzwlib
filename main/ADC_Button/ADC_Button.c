#include "ADC_Button.h"
#define TAG "ADC_Button"
button_handle_t adc_sw2_btn = NULL;
button_handle_t adc_sw3_btn = NULL;


void ADC_Button_Init(void) {

// create sw2_adc button
const button_config_t btn_sw2_cfg = {0};
button_adc_config_t btn_sw2_adc_cfg = {
    .unit_id = ADC_UNIT_1,
    .adc_channel = 7,
    .button_index = 0,
    .min = 0,
    .max = 400,
};


esp_err_t ret = iot_button_new_adc_device(&btn_sw2_cfg, &btn_sw2_adc_cfg, &adc_sw2_btn);
if(NULL == adc_sw2_btn) {
    ESP_LOGE(TAG, "Button create failed");
}

// create sw3_adc button
const button_config_t btn_sw3_cfg = {0};
button_adc_config_t btn_sw3_adc_cfg = {
    .unit_id = ADC_UNIT_1,
    .adc_channel = 7,
    .button_index = 1,
    .min = 1300,
    .max = 1700,
};


ret = iot_button_new_adc_device(&btn_sw3_cfg, &btn_sw3_adc_cfg, &adc_sw3_btn);
if(NULL == adc_sw3_btn) {
    ESP_LOGE(TAG, "Button create failed");
}


}


// sw2 Register callback function
void ADC_sw2button_Register_callback(button_event_t event, button_cb_t cb,void *usr_data)
{ 
    iot_button_register_cb(adc_sw2_btn, event, NULL, cb,usr_data);
}

// sw3 Register callback function
void ADC_sw3button_Register_callback(button_event_t event, button_cb_t cb,void *usr_data)
{ 
    iot_button_register_cb(adc_sw3_btn, event, NULL, cb,usr_data);

}

