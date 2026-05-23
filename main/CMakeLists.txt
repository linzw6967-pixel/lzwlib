idf_component_register(SRCS 
                        "main.c"
                        "ADC_Button/ADC_Button.c"
                        "Internet/xiaozhi_wifi.c"
                        "Internet/xiaozhi_active.c"
                        "Display/xiaozhi_lcd.c"
                        "Display/xiaozhi_lvgl.c"
                        "audio/xiaozhi_audio.c"
                        "audio/xiaozhi_sr.c"
                        "audio/xiaozhi_audio_decoder.c"
                        "audio/xiaozhi_audio_encoder.c"
                        "Internet/xiaozhi_websocket.c"
                    INCLUDE_DIRS
                         "."
                         "./Internet"
                         "./Display"
                         "./audio"
                         "./volume_text"
                         "./ADC_Button"
                         EMBED_FILES image.jpg)


