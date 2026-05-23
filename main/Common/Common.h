#pragma once
#include "stdbool.h"
#include "esp_vad.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "freertos/event_groups.h"
//创建小智状态机枚举类型
typedef enum
{
    XIAOZHI_STATE_IDLE = 0,
    XIAOZHI_STATE_LISTEN,
    XIAOZHI_STATE_SPEAK,
} xiaozhi_state_t;


typedef struct
{
    char *websocket_url;
    char *websocket_token;
    char *active_code;
    bool is_active;

    //创建事件标志组
    EventGroupHandle_t xiaozhi_event_group;
    // 上一次的状态
    vad_state_t xiaozhi_vad_last_status;
    // 当前的状态
    vad_state_t xiaozhi_vad_current_status;

    /* 激活的标志 */
    bool is_actived;
    /* 唤醒的标志 */
    bool wkup_flag;

    // 唤醒后执行的CALLBACK
    void (*wakeup_callback)(void);
    // 人声切换后执行的CALLBACK
    void (*voice_change_callback)(void);
    //定义文本处理函数指针
    void (*websocket_process_farme)(char *data, size_t len);
    //定义语音接收函数指针
    void (*websocket_process_audio)(char *data, size_t len);
    //创建SR到编码器的环形缓冲区
    RingbufHandle_t srToencoder_buff_handle;
    //创建编码器到WS的环形缓冲区
    RingbufHandle_t encoderToWs_buff_handle;
    //创建WS到解码器的环形缓冲区
    RingbufHandle_t wsToDecoder_buff_handle;

    //定义小智状态机枚举变量
    xiaozhi_state_t xiaozhi_state;
} xiaozhi_handle_t;

extern xiaozhi_handle_t xiaozhi_handle;
extern uint8_t is_power_amplifier;

//定义事件连接标志位
#define WEBSOCKET_Event_Connected BIT0
//定义hello事件标志位
#define WEBSOCKET_Event_Hello_Receive BIT1

