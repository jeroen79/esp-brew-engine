idf_component_register(SRCS "wifi-connect-lite.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES settings-manager nvs_flash esp_wifi)