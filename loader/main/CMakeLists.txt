idf_component_register(SRCS "main.cpp"
                        INCLUDE_DIRS "."
                        REQUIRES settings-manager wifi-connect esp_http_server app_update
                        EMBED_FILES "index.html")