idf_component_register(SRCS "station_example_main.c"
                            "tcp_server.c"
                            "uart_handler.c"
                            "audio_handler.c"
                            "mdns_service.c"
                    REQUIRES max98357 esp_driver_uart
                    PRIV_REQUIRES esp_wifi nvs_flash lwip mdns
                    INCLUDE_DIRS ".")
