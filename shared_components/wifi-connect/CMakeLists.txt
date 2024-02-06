idf_component_register(SRCS "wifi-connect.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES settings-manager nvs_flash esp_wifi)