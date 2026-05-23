#include "xiaozhi_wifi.h"

void xiaozhi_wifi_nvs_Init(void);
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void get_device_service_name(char *service_name, size_t max);
static void wifi_prov_print_qr(const char *name, const char *username, const char *pop, const char *transport);

#define WIFI_CONNECTED_BIT BIT0
#define TAG "xiaozhi_wifi"
#define PROV_TRANSPORT_BLE      "ble"
#define QRCODE_BASE_URL         "https://espressif.github.io/esp-jumpstart/qrcode.html"
#define PROV_QR_VERSION         "v1"

static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;

void xiaozhi_wifi_init(void)
{

    //!提交测试
    //fenzhi1
    // 存储器初始化
    xiaozhi_wifi_nvs_Init();
    // 创建事件标志组
    s_wifi_event_group = xEventGroupCreate();
    //网络接口初始化
    ESP_ERROR_CHECK(esp_netif_init());
    //创建事件轮询机制的任务
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // 创建wifi的station
    esp_netif_create_default_wifi_sta();
    //wifi初始化
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    


    //注册回调函数
    //监听 Wi-Fi 相关事件：通过注册 WIFI_EVENT 的回调，捕获所有与 Wi-Fi 相关的事件。 
    //监听 IP 地址分配事件：通过注册 IP_EVENT_STA_GOT_IP 的回调，捕获站点模式下成功获取 IP 地址的事件。
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_TRANSPORT_BLE_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

 
    //并网管理配置
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_ble,                                      // 蓝牙模式
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM // 一旦wifi配网结束就释放蓝牙所有资源
    };
    // 初始化wifi信息
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
    // 判断是否已经配网
    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    if (!provisioned)
    {
        ESP_LOGI(TAG, "Starting provisioning");
        //用户名
        char service_name[12];
        get_device_service_name(service_name, sizeof(service_name));
        //并网安全等级
        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
        //密钥定义并传入
        const char *pop = "abcd1234";
        wifi_prov_security1_params_t *sec_params = pop;

        const char *username = NULL;

        const char *service_key = NULL;

        uint8_t custom_service_uuid[] = {
            /* LSB <---------------------------------------
             * ---------------------------------------> MSB */
            0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
            0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
        };
        //设置uuid
        wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);
        //开始并网
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, (const void *) sec_params, service_name, service_key));
        //生成二维码
        wifi_prov_print_qr(service_name, username, pop, PROV_TRANSPORT_BLE);

        xiaozhi_lvgl_showtile("请扫码连接WiFi");
    }
    else
    {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");
        /* We don't need the manager as device is already provisioned,
         * so let's release it's resources */
        wifi_prov_mgr_deinit();
        /* Start Wi-Fi station */
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    }
    //!指定信息联网
    // wifi_config_t wifi_config = {
    //     .sta = {
    //         .ssid = "“lin19”的iPhone",
    //         .password = "12345678",
    //         /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
    //          * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
    //          * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
    //          * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
    //          */
    //         .threshold.authmode = WIFI_AUTH_WPA2_PSK,
    //         .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
    //         .sae_h2e_identifier = "",
    //     },
    // };

    // ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    // ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT,
                                           pdTRUE,
                                           pdFALSE,
                                           portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "wifi conntect success");
        // //!配网成功删除二维码
        xiaozhi_lvgl_del_qrcode();
    }
}

void xiaozhi_wifi_nvs_Init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < 10)
        {
            xiaozhi_lvgl_showtile("自动连接记忆WIFI");
            xiaozhi_lvgl_showemoji("sunglasses");
            xiaozhi_lvgl_showlabel("正在连接中.....");
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        if(s_retry_num >= 10)
        {
            xiaozhi_lvgl_showemoji("crying");
            xiaozhi_lvgl_showlabel("自动连接失败，长按WIFI信息清除键重新扫码吧");
        }

    }
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        //获取IP后就放行
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }


    if (event_base == PROTOCOMM_TRANSPORT_BLE_EVENT)
    {
        switch (event_id)
        {
        case PROTOCOMM_TRANSPORT_BLE_CONNECTED:
            ESP_LOGI(TAG, "BLE transport: Connected!");
            break;
        case PROTOCOMM_TRANSPORT_BLE_DISCONNECTED:
            ESP_LOGI(TAG, "BLE transport: Disconnected!");
            break;
        default:
            break;
        }

    if (event_base == WIFI_PROV_EVENT)
    {
        switch (event_id)
        {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;
        case WIFI_PROV_CRED_RECV:
        {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Received Wi-Fi credentials"
                          "\n\tSSID     : %s\n\tPassword : %s",
                     (const char *)wifi_sta_cfg->ssid,
                     (const char *)wifi_sta_cfg->password);
            break;
        }
        case WIFI_PROV_CRED_FAIL:
        {
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                          "\n\tPlease reset to factory and retry provisioning",
                     (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
#ifdef CONFIG_EXAMPLE_RESET_PROV_MGR_ON_FAILURE
            retries++;
            if (retries >= CONFIG_EXAMPLE_PROV_MGR_MAX_RETRY_CNT)
            {
                ESP_LOGI(TAG, "Failed to connect with provisioned AP, resetting provisioned credentials");
                wifi_prov_mgr_reset_sm_state_on_failure();
                retries = 0;
            }
#endif
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
        //!配网成功删除二维码
        xiaozhi_lvgl_del_qrcode();
        ESP_LOGI(TAG, "Provisioning successful");
            
            
#ifdef CONFIG_EXAMPLE_RESET_PROV_MGR_ON_FAILURE
            retries = 0;
#endif
            break;
        case WIFI_PROV_END:
            /* De-initialize manager once provisioning is finished */
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
        }
    }
}
}

static void get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X",
             ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

static void wifi_prov_print_qr(const char *name, const char *username, const char *pop, const char *transport)
{
    if (!name || !transport) {
        ESP_LOGW(TAG, "Cannot generate QR code payload. Data missing.");
        return;
    }
    char payload[150] = {0};
    if (pop) {

        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                    ",\"pop\":\"%s\",\"transport\":\"%s\"}",
                    PROV_QR_VERSION, name, pop, transport);
    } else {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                    ",\"transport\":\"%s\"}",
                    PROV_QR_VERSION, name, transport);
    }

    ESP_LOGI(TAG, "Scan this QR code from the provisioning application for Provisioning.");
    // esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
    // esp_qrcode_generate(&cfg, payload);

    xiaozhi_lvgl_showQrcode(payload);
    //ESP_LOGI(TAG, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s", QRCODE_BASE_URL, payload);
}


//清除配网信息
void xiaozhi_wifi_clear(void)
{
    //清楚配网信息
    esp_wifi_restore();
    //重启芯片
    esp_restart();
}

