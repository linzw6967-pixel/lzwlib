#include "audio/xiaozhi_audio.h"

i2s_chan_handle_t i2s_tx_handle;
i2s_chan_handle_t i2s_rx_handle;
static i2c_master_bus_handle_t i2c_bus_handle;
esp_codec_dev_handle_t codec_dev;
// es8311_codec_cfg_t es8311_cfg;

void xiaozhi_audio_I2C_Init(void);
void xiaozhi_audio_I2S_Init(void);

void xiaozhi_audio_init()
{

    // IIC与IIS通信初始化s
    xiaozhi_audio_I2C_Init();
    xiaozhi_audio_I2S_Init();

    // 2. 为编解码器设备实现控制接口，数据接口和 GPIO 接口 (使用默认提供的接口实现)
    audio_codec_i2s_cfg_t i2s_cfg = {
        .rx_handle = i2s_rx_handle,
        .tx_handle = i2s_tx_handle,
    };
    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_cfg);

    audio_codec_i2c_cfg_t i2c_cfg = {
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_bus_handle};
    const audio_codec_ctrl_if_t *out_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();

    // 3. 基于控制接口和 ES8311 特有的配置实现 `audio_codec_if_t` 接口
    es8311_codec_cfg_t es8311_cfg = {
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH,
        .ctrl_if = out_ctrl_if,
        .gpio_if = gpio_if,
        .pa_pin = GPIO_NUM_7, // 功放的使能
        .use_mclk = true,     // 使用mclk作为参考时钟
    };
    const audio_codec_if_t *out_codec_if = es8311_codec_new(&es8311_cfg);

    // 4. 通过 API `esp_codec_dev_new` 获取 `esp_codec_dev_handle_t` 句柄
    esp_codec_dev_cfg_t dev_cfg = {
        .codec_if = out_codec_if,              // es8311_codec_new 获取到的接口实现
        .data_if = data_if,                    // audio_codec_new_i2s_data 获取到的数据接口实现
        .dev_type = ESP_CODEC_DEV_TYPE_IN_OUT, // 设备同时支持录制和播放
    };
    codec_dev = esp_codec_dev_new(&dev_cfg);
    // 设置音量
    esp_codec_dev_set_out_vol(codec_dev, 80.0);
    // 设置增益
    esp_codec_dev_set_in_gain(codec_dev, 15.0);

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = 16000,  // 采样率
        .channel = 1,          // 声道数
        .bits_per_sample = 16, // 采样深度
    };

    // 开启ES8311
    esp_codec_dev_open(codec_dev, &fs);
}

void xiaozhi_audio_I2C_Init(void)
{
    i2c_master_bus_config_t i2c_bus_config = {0};
    i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_bus_config.i2c_port = I2C_NUM_0;
    i2c_bus_config.scl_io_num = GPIO_NUM_1;
    i2c_bus_config.sda_io_num = GPIO_NUM_0;
    i2c_bus_config.glitch_ignore_cnt = 7;
    i2c_bus_config.flags.enable_internal_pullup = true;
    i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
}

void xiaozhi_audio_I2S_Init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(16, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = 3,
            .bclk = 2,
            .ws = 5,
            .dout = 6,
            .din = 4,
        },
    };

    // 创建通道
    i2s_new_channel(&chan_cfg, &i2s_tx_handle, &i2s_rx_handle);
    // 初始化I2S
    i2s_channel_init_std_mode(i2s_tx_handle, &std_cfg);
    i2s_channel_init_std_mode(i2s_rx_handle, &std_cfg);
    // 启动I2S
    i2s_channel_enable(i2s_tx_handle);
    i2s_channel_enable(i2s_rx_handle);
}

/**
 * @brief 播放音频
 *
 * @param data 音频数据
 */
void xiaozhi_audio_play(uint8_t *data, uint32_t data_len)
{
    esp_codec_dev_write(codec_dev, data, data_len);
}
/**
 * @brief 录音
 *
 * @param data
 */
void xiaozhi_audio_record(uint8_t *data, uint32_t data_len)
{
    esp_codec_dev_read(codec_dev, data, data_len);
}

/* 控制音量 */
int last_volume = 0;
void xiaozhi_audio_volume_set(int volume)
{
  last_volume = volume;
  esp_codec_dev_set_out_vol(codec_dev, volume);
}

/* 设置是否静音 */
void xiaozhi_audio_mute_set(bool mute)
{
  esp_codec_dev_set_out_vol(codec_dev, mute == true ? 0 : last_volume);
}