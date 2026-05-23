#include "xiaozhi_lcd.h"

#define TAG "xiaozhi_lcd"
#define IMAGE_W 320
#define IMAGE_H 240

// extern const uint8_t image_jpg_start[] asm("_binary_image_jpg_start");
// extern const uint8_t image_jpg_end[] asm("_binary_image_jpg_end");
// void xiaozhi_lcd_showpicture(uint8_t * image_data);

// LCD panel配置
esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_panel_io_handle_t io_handle = NULL;

static uint16_t *s_lines[2];
void xiaozhi_lcd_init(void)
{
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT};
    // Initialize the GPIO of backlight
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_PIN_NUM_PCLK,
        .mosi_io_num = EXAMPLE_PIN_NUM_DATA0,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = PARALLEL_LINES * EXAMPLE_LCD_H_RES * 2 + 8};

    // Initialize the SPI bus
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_PIN_NUM_DC,
        .cs_gpio_num = EXAMPLE_PIN_NUM_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };

    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    // esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    // Initialize the LCD configuration
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    // Turn off backlight to avoid unpredictable display on the LCD screen while initializing
    // the LCD panel driver. (Different LCD screens may need different levels)
    ESP_ERROR_CHECK(gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL));

    // Reset the display
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));

    // Initialize LCD panel
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // Turn on the screen
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    // ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

    // Swap x and y axis (Different LCD screens may need different options)
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));

    // Turn on backlight (Different LCD screens may need different levels)
    ESP_ERROR_CHECK(gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL));

    // Allocate memory for the pixel buffers
    for (int i = 0; i < 2; i++)
    {
        s_lines[i] = heap_caps_malloc(EXAMPLE_LCD_H_RES * PARALLEL_LINES * sizeof(uint16_t), MALLOC_CAP_DMA);
        assert(s_lines[i] != NULL);
    }

    // 翻转
    esp_lcd_panel_mirror(panel_handle, true, false);
}

// void xiaozhi_lcd_showtest(void)
// {

//     uint8_t *pixels=heap_caps_malloc(IMAGE_H * IMAGE_W*sizeof(uint16_t),MALLOC_CAP_SPIRAM);

//     //JPEG decode config
//     esp_jpeg_image_cfg_t jpeg_cfg = {
//         .indata = (uint8_t *)image_jpg_start,
//         .indata_size = image_jpg_end - image_jpg_start,
//         .outbuf = pixels,
//         .outbuf_size = IMAGE_W * IMAGE_H * sizeof(uint16_t),
//         .out_format = JPEG_IMAGE_FORMAT_RGB565,
//         .out_scale = JPEG_IMAGE_SCALE_0,
//         .flags = {
//             .swap_color_bytes = 1,
//         }
//     };

//     //JPEG decode
//     esp_jpeg_image_output_t outimg;
//     esp_jpeg_decode(&jpeg_cfg, &outimg);
//     xiaozhi_lcd_showpicture(pixels);
// }

// void xiaozhi_lcd_showpicture(uint8_t * image_data)
// {
//     int sending_line = 0;
//     int calc_line = 0;
//     uint32_t cpy_size=EXAMPLE_LCD_H_RES * PARALLEL_LINES * sizeof(uint16_t);
//         for (int y = 0; y < EXAMPLE_LCD_V_RES; y += PARALLEL_LINES) {
//             memcpy(s_lines[calc_line], image_data, cpy_size);
//             image_data+=cpy_size;
//             sending_line = calc_line;
//             calc_line = !calc_line;
//         esp_lcd_panel_draw_bitmap(panel_handle, 0, y, 0 + EXAMPLE_LCD_H_RES, y + PARALLEL_LINES, s_lines[sending_line]);

//         }
// }
