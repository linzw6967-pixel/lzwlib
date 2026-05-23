#pragma once
#include "esp_lvgl_port.h"
#include "xiaozhi_lcd.h"
#include "esp_log.h"
#include "font_emoji.h"
#include "font_puhui.h"


extern const lv_font_t font_puhui_14_1;
extern const lv_font_t font_puhui_16_4;
extern const lv_font_t font_puhui_20_4;


#define LCD_H_RES   (320)
#define LCD_V_RES   (240)


void xiaozhi_lvgl_init(void);

//void xiaozhi_lvgl_helloworld(void);

void xiaozhi_lvgl_showQrcode(char *qrcode);

void xiaozhi_lvgl_del_qrcode(void);

void xiaozhi_lvgl_showtile(char *tile);

void xiaozhi_lvgl_showlabel(char *label);

void xiaozhi_lvgl_showemoji(char *emoji);
