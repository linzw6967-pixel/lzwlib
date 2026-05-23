#include "xiaozhi_lvgl.h"

#define TAG "xiaozhi_lvgl"
void xiaozhi_lvgl_Init_Show(void);
lv_obj_t *screen;
lv_obj_t *qr = NULL;

extern esp_lcd_panel_handle_t panel_handle;
extern esp_lcd_panel_io_handle_t io_handle;

static lv_display_t *lvgl_disp = NULL;
#define LCD_DRAW_BUFF_HEIGHT 16

lv_obj_t *label_obj;
lv_obj_t *txet_label;
lv_obj_t *emoji_label;

// 定义emoji结构体
typedef struct
{
    char *name;
    char *emoji;
} emoji_t;

// 定义以emoji结构体为元素的emoji数组
const emoji_t emoji_match[21] = {
    {.name = "neutral", .emoji = "😶"},     // 1. 😶 - neutral
    {.name = "happy", .emoji = "🙂"},       // 2. 🙂 - happy
    {.name = "laughing", .emoji = "😆"},    // 3. 😆 - laughing
    {.name = "funny", .emoji = "😂"},       // 4. 😂 - funny
    {.name = "sad", .emoji = "😔"},         // 5. 😔 - sad
    {.name = "angry", .emoji = "😠"},       // 6. 😠 - angry
    {.name = "crying", .emoji = "😭"},      // 7. 😭 - crying
    {.name = "loving", .emoji = "😍"},      // 8. 😍 - loving
    {.name = "embarrassed", .emoji = "😳"}, // 9. 😳 - embarrassed
    {.name = "surprised", .emoji = "😲"},   // 10. 😲 - surprised
    {.name = "shocked", .emoji = "😱"},     // 11. 😱 - shocked
    {.name = "thinking", .emoji = "🤔"},    // 12. 🤔 - thinking
    {.name = "winking", .emoji = "😉"},     // 13. 😉 - winking
    {.name = "cool", .emoji = "😎"},        // 14. 😎 - cool
    {.name = "relaxed", .emoji = "😌"},     // 15. 😌 - relaxed
    {.name = "delicious", .emoji = "🤤"},   // 16. 🤤 - delicious
    {.name = "kissy", .emoji = "😘"},       // 17. 😘 - kissy
    {.name = "confident", .emoji = "😏"},   // 18. 😏 - confident
    {.name = "sleepy", .emoji = "😴"},      // 19. 😴 - sleepy
    {.name = "silly", .emoji = "😜"},       // 20. 😜 - silly
    {.name = "confused", .emoji = "🙄"}     // 21. 🙄 - confused
};

void xiaozhi_lvgl_init(void)
{
    xiaozhi_lcd_init();
    // xiaozhi_lcd_showtest();
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,       /* LVGL task priority */
        .task_stack = 6144 * 2,   /* LVGL task stack size */
        .task_affinity = -1,      /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 500, /* Maximum sleep in LVGL task */
        .timer_period_ms = 5      /* LVGL timer tick period in ms */
    };
    lvgl_port_init(&lvgl_cfg);

    uint32_t buff_size = LCD_H_RES * LCD_V_RES;

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * LCD_DRAW_BUFF_HEIGHT,
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = false,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy = true,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
            .swap_bytes = true,
        }};
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);

    /* 设置整个屏幕的公共样式 */
    lvgl_port_lock(0);
    screen = lv_screen_active();
    // 设置屏幕对象设置背景颜色
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xf3f7f5), LV_PART_MAIN);
    // 设置文本颜色
    lv_obj_set_style_text_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    lvgl_port_unlock();

    xiaozhi_lvgl_Init_Show();
}

// void xiaozhi_lvgl_helloworld(void)
// {
//     lvgl_port_lock(0);

//     lv_obj_t *screen=lv_screen_active();

//     lv_obj_set_style_bg_color(screen, lv_color_hex(0x063c41), LV_PART_MAIN);

//     lv_obj_t * label = lv_label_create(screen);

//     //设置文本
//     lv_label_set_text(label, "Hello World!");

//     //设置文本颜色
//     lv_obj_set_style_text_color(screen, lv_color_hex(0xffffff), LV_PART_MAIN);

//     //设置对齐方式
//     lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

//     lvgl_port_unlock();
// }

// 显示二维码
void xiaozhi_lvgl_showQrcode(char *qrcode)
{
    lvgl_port_lock(0);
    qr = lv_qrcode_create(lv_screen_active());
    lv_qrcode_set_size(qr, 150);
    lv_qrcode_set_dark_color(qr, lv_color_hex(0x000000));
    lv_qrcode_set_light_color(qr, lv_color_hex(0xffffff));

    /*Set data*/
    const char *data = qrcode;
    lv_qrcode_update(qr, data, strlen(data));
    lv_obj_center(qr);

    /*Add a border with bg_color*/
    // lv_obj_set_style_border_color(qr, lv_color_hex(0x000000), 0);
    // lv_obj_set_style_border_width(qr, 5, 0);

    lvgl_port_unlock();
}
// 二维码删除方法
void xiaozhi_lvgl_del_qrcode(void)
{
    if (qr == NULL)
    {
        return;
    }

    lvgl_port_lock(0);
    lv_obj_del(qr);
    qr = NULL;
    lvgl_port_unlock();
}

void xiaozhi_lvgl_Init_Show(void)
{
    lvgl_port_lock(0);
    // 创建容器对象
    lv_obj_t *title_obj = lv_obj_create(screen);
    // 设置容器大小
    lv_obj_set_size(title_obj, 320, 30);
    // 设置容器背景颜色
    lv_obj_set_style_bg_color(title_obj, lv_color_hex(0xc4d8ce), LV_PART_MAIN);
    // 消除滚动条
    lv_obj_set_scrollbar_mode(title_obj, LV_SCROLLBAR_MODE_OFF);
    // 设置title_obj边框圆角
    lv_obj_set_style_radius(title_obj, 10, LV_PART_MAIN);
    // 创建文本标签
    label_obj = lv_label_create(title_obj);
    // 设置文本库
    lv_obj_set_style_text_font(title_obj, &font_puhui_16_4, LV_PART_MAIN);
    // 设置文本内容
    // lv_label_set_text(label_obj,"你好小智");

    // 设置Title_obj成为flex容器
    lv_obj_set_layout(title_obj, LV_LAYOUT_FLEX);
    // 设置flex主轴方向
    lv_obj_set_flex_flow(title_obj, LV_FLEX_FLOW_ROW);
    // 设置label基于容器水平垂直居中
    lv_obj_set_flex_align(title_obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // emoji

    // 创建emoji标签
    emoji_label = lv_label_create(screen);
    // 设置文本水平居中
    lv_obj_set_style_text_align(emoji_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    // 设置标签的宽度
    lv_obj_set_width(emoji_label, 300);
    // 设置垂直方向的位置
    lv_obj_align_to(emoji_label, title_obj, LV_ALIGN_OUT_BOTTOM_MID, 0, 40);
    // 设置文本相对屏幕居中
    // lv_obj_align(emoji_label, LV_ALIGN_CENTER, 0, -20);
    // 设置emoji_label表情库
    lv_obj_set_style_text_font(emoji_label, font_emoji_64_init(), LV_PART_MAIN);
    // 设置文本内容
    // lv_label_set_text(emoji_label,"😍");

    // text

    // 创建text标签
    txet_label = lv_label_create(screen);
    // 设置文本水平居中
    lv_obj_set_style_text_align(txet_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    // 设置标签的宽度
    lv_obj_set_width(txet_label, 300);
    // 设置垂直方向的位置
    lv_obj_align_to(txet_label, emoji_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 40);
    // 设置文本相对屏幕居中
    // lv_obj_align(txet_label, LV_ALIGN_CENTER, 0, 20);
    // 设置txet_label文本库
    lv_obj_set_style_text_font(txet_label, &font_puhui_14_1, LV_PART_MAIN);
    // 设置文本内容
    // lv_label_set_text(txet_label,"林忠伟");

    lvgl_port_unlock();
}

void xiaozhi_lvgl_showtile(char *tile)
{
    lvgl_port_lock(0);
    lv_label_set_text(label_obj, tile);
    lvgl_port_unlock();
}

void xiaozhi_lvgl_showlabel(char *label)
{
    lvgl_port_lock(0);
    lv_label_set_text(txet_label, label);
    lvgl_port_unlock();
}

void xiaozhi_lvgl_showemoji(char *emoji)
{
    lvgl_port_lock(0);
    bool isMatch = false;
    for (uint8_t i = 0; i < 21; i++)
    {
        if (strcmp(emoji, emoji_match[i].name) == 0)
        {
            isMatch = true;
            lv_label_set_text(emoji_label, emoji_match[i].emoji);
            break;
        }
    }

    if (!isMatch)
    {
        lv_label_set_text(emoji_label, "😶");
    }

    lvgl_port_unlock();
}
